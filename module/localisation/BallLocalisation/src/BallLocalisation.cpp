#include "BallLocalisation.hpp"

#include <chrono>

#include "extension/Configuration.hpp"

#include "message/input/Sensors.hpp"
#include "message/localisation/Ball.hpp"
#include "message/support/FieldDescription.hpp"
#include "message/vision/Ball.hpp"

#include "utility/input/ServoID.hpp"
#include "utility/math/coordinates.hpp"
#include "utility/math/matrix/Transform3D.hpp"
#include "utility/nusight/NUhelpers.hpp"
#include "utility/support/eigen_armadillo.hpp"
#include "utility/support/yaml_expression.hpp"

namespace module {
namespace localisation {

    using extension::Configuration;
    using message::input::Sensors;
    using message::localisation::Ball;
    using message::support::FieldDescription;

    using utility::math::coordinates::cartesianToSpherical;
    using utility::math::matrix::Transform3D;
    using ServoID = utility::input::ServoID;
    using utility::nusight::graph;
    using utility::support::Expression;

    BallLocalisation::BallLocalisation(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment)), filter() {

        on<Startup>().then([this] {
            last_measurement_update_time = NUClear::clock::now();
            last_time_update_time        = NUClear::clock::now();
        });

        on<Configuration, Sync<BallLocalisation>>("BallLocalisation.yaml").then([this](const Configuration& config) {
            auto message = std::make_unique<std::vector<Ball>>();
            emit(message);
            emit(std::make_unique<Ball>());

            ball_pos_log = config["ball_pos_log"].as<bool>();
            // Use configuration here from file RobotParticleLocalisation.yaml
            filter.model.processNoiseDiagonal = arma::vec2(config["process_noise_diagonal"].as<Expression>());
            filter.model.n_rogues             = config["n_rogues"].as<int>();
            filter.model.resetRange           = arma::vec2(config["reset_range"].as<Expression>());
            int n_particles                   = config["n_particles"].as<int>();

            arma::vec2 start_state    = config["start_state"].as<Expression>();
            arma::vec2 start_variance = config["start_variance"].as<Expression>();

            filter.reset(start_state, arma::diagmat(start_variance), n_particles);

            // Use configuration here from file BallLocalisation.yaml
        });

        /* Run Time Update */
        on<Every<15, Per<std::chrono::seconds>>, Sync<BallLocalisation>>().then("BallLocalisation Time", [this] {
            /* Perform time update */
            using namespace std::chrono;
            auto curr_time        = NUClear::clock::now();
            double seconds        = duration_cast<duration<double>>(curr_time - last_time_update_time).count();
            last_time_update_time = curr_time;
            filter.timeUpdate(seconds);

            /* Creating ball state vector and covariance matrix for emission */
            auto ball        = std::make_unique<Ball>();
            ball->position   = convert(filter.get());
            ball->covariance = convert(filter.getCovariance());

            if (ball_pos_log) {
                emit(graph("localisation ball pos", filter.get()[0], filter.get()[1]));
                log("localisation ball pos = ", filter.get()[0], filter.get()[1]);
                log("localisation seconds elapsed = ", seconds);
            }
            emit(ball);
        });

        /* To run whenever a ball has been detected */
        on<Trigger<message::vision::Balls>, With<FieldDescription>>().then(
            [this](const message::vision::Balls& balls, const FieldDescription& field) {
                if (balls.balls.size() > 0) {
                    /* Call Time Update first */
                    using namespace std::chrono;
                    auto curr_time        = NUClear::clock::now();
                    double seconds        = duration_cast<duration<double>>(curr_time - last_time_update_time).count();
                    last_time_update_time = curr_time;
                    filter.timeUpdate(seconds);

                    /* Now call Measurement Update. Supports multiple measurement methods
                     * and will treat them as
                     * separate measurements */
                    for (auto& measurement : balls.balls[0].measurements) {
                        filter.measurementUpdate(arma::conv_to<arma::vec>::from(convert(measurement.rBCc)),
                                                 arma::conv_to<arma::mat>::from(convert(measurement.covariance)),
                                                 field,
                                                 convert(balls.Hcw));
                    }
                    last_measurement_update_time = curr_time;
                }
            });
    }
}  // namespace localisation
}  // namespace module

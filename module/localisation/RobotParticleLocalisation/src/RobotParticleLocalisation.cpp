#include "RobotParticleLocalisation.h"

#include "extension/Configuration.h"
#include "message/input/Sensors.h"
#include "message/localisation/Field.h"
#include "message/localisation/ResetRobotHypotheses.h"
#include "utility/localisation/transform.h"

#include "utility/math/geometry/Circle.h"
#include "utility/nubugger/NUhelpers.h"
#include "utility/support/eigen_armadillo.h"
#include "utility/support/yaml_armadillo.h"
#include "utility/time/time.h"

namespace module {
namespace localisation {

    using extension::Configuration;

    using message::input::Sensors;
    using message::localisation::Field;

    using utility::math::matrix::Transform2D;
    using utility::math::matrix::Transform3D;
    using utility::nubugger::graph;
    using utility::nubugger::drawCircle;
    using utility::math::geometry::Circle;
    using utility::time::TimeDifferenceSeconds;

    using message::support::FieldDescription;
    using message::vision::Goal;
    using message::localisation::ResetRobotHypotheses;
    using utility::localisation::transform3DToFieldState;

    RobotParticleLocalisation::RobotParticleLocalisation(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment)) {

        last_measurement_update_time = NUClear::clock::now();
        last_time_update_time        = NUClear::clock::now();

        on<Configuration>("RobotParticleLocalisation.yaml").then([this](const Configuration& config) {
            // Use configuration here from file RobotParticleLocalisation.yaml
            filter.model.processNoiseDiagonal = config["process_noise_diagonal"].as<arma::vec>();
            filter.model.n_rogues             = config["n_rogues"].as<int>();
            filter.model.resetRange           = config["reset_range"].as<arma::vec>();
            n_particles                       = config["n_particles"].as<int>();
            draw_particles                    = config["draw_particles"].as<int>();

            arma::vec3 start_state    = config["start_state"].as<arma::vec>();
            arma::vec3 start_variance = config["start_variance"].as<arma::vec>();

            std::vector<arma::vec3> possible_states;
            std::vector<arma::mat33> possible_var;

            possible_states.push_back(start_state);
            // Reflected position
            possible_states.push_back(arma::vec3{start_state[0], -start_state[1], -start_state[2]});

            possible_var.push_back(arma::diagmat(start_variance));
            possible_var.push_back(arma::diagmat(start_variance));

            filter.resetAmbiguous(possible_states, possible_var, n_particles);

        });

        on<Every<PARTICLE_UPDATE_FREQUENCY, Per<std::chrono::seconds>>, Sync<RobotParticleLocalisation>>().then(
            "Particle Debug", [this]() {
                arma::mat particles = filter.getParticles();

                for (int i = 0; i < std::min(draw_particles, int(particles.n_cols)); i++) {
                    emit(drawCircle("particle" + std::to_string(i),
                                    Circle(0.01, particles.submat(0, i, 1, i)),
                                    0.05,
                                    {0, 0, 0},
                                    PARTICLE_UPDATE_FREQUENCY));
                }
            });

        on<Every<TIME_UPDATE_FREQUENCY, Per<std::chrono::seconds>>, Sync<RobotParticleLocalisation>>().then(
            "Time Update", [this]() {
                /* Perform time update */
                auto curr_time        = NUClear::clock::now();
                double seconds        = TimeDifferenceSeconds(curr_time, last_time_update_time);
                last_time_update_time = curr_time;

                filter.timeUpdate(seconds);

                // Get filter state and transform
                arma::vec3 state = filter.get();
                emit(graph("robot filter state = ", state[0], state[1], state[2]));

                // Emit state
                auto field = std::make_unique<Field>();
                field->position =
                    Eigen::Vector3d(state[RobotModel::kX], state[RobotModel::kY], state[RobotModel::kAngle]);
                field->covariance = convert<double, 3, 3>(filter.getCovariance());

                emit(std::make_unique<std::vector<Field>>(1, *field));
                emit(field);
            });

        on<Trigger<std::vector<Goal>>, With<FieldDescription>, Sync<RobotParticleLocalisation>>().then(
            "Measurement Update", [this](const std::vector<Goal>& goals, const FieldDescription& fd) {

                if (!goals.empty()) {
                    /* Perform time update */
                    auto curr_time        = NUClear::clock::now();
                    double seconds        = TimeDifferenceSeconds(curr_time, last_time_update_time);
                    last_time_update_time = curr_time;

                    filter.timeUpdate(seconds);

                    for (auto goal : goals) {

                        // Check side and team
                        std::vector<arma::vec> poss = getPossibleFieldPositions(goal, fd);

                        for (auto& m : goal.measurement) {
                            if (m.type == Goal::MeasurementType::CENTRE) {
                                filter.ambiguousMeasurementUpdate(
                                    convert<double, 3>(m.position),
                                    convert<double, 3, 3>(m.covariance),
                                    poss,
                                    convert<double, 4, 4>(goals[0].visObject.classifiedImage->image->Hcw),
                                    m.type,
                                    fd);
                            }
                        }
                    }
                }
            });

        on<Trigger<ResetRobotHypotheses>, With<Sensors>, Sync<RobotParticleLocalisation>>().then(
            "Reset Robot Hypotheses", [this](const ResetRobotHypotheses& locReset, const Sensors& sensors) {

                Transform3D Hft;
                Transform3D Hfw;
                const Transform3D& Htw = convert<double, 4, 4>(sensors.world);
                arma::vec3 rTFf;
                arma::mat22 Hfw_xy;
                arma::mat22 pos_cov;
                arma::mat33 state_cov;
                std::vector<arma::vec3> states;
                std::vector<arma::mat33> cov;

                for (auto& s : locReset.hypotheses) {
                    rTFf              = {s.position[0], s.position[1], 0};
                    Hft.translation() = -rTFf;
                    Hft.rotateZ(s.heading);
                    Hfw = Hft * Htw;
                    states.push_back(transform3DToFieldState(Hfw));

                    Hfw_xy    = Hfw.submat(0, 0, 1, 1);
                    pos_cov   = Hfw_xy * convert<double, 2, 2>(s.position_cov) * Hfw_xy.t();
                    state_cov = arma::zeros(3, 3);
                    state_cov.submat(0, 0, 1, 1) = pos_cov;
                    state_cov(2, 2) = s.heading_var;
                    cov.push_back(state_cov);
                }
                filter.resetAmbiguous(states, cov, n_particles);
            });
    }

    std::vector<arma::vec> RobotParticleLocalisation::getPossibleFieldPositions(
        const message::vision::Goal& goal,
        const message::support::FieldDescription& fd) const {
        std::vector<arma::vec> possibilities;

        bool left  = (goal.side != Goal::Side::RIGHT);
        bool right = (goal.side != Goal::Side::LEFT);
        bool own   = (goal.team != Goal::Team::OPPONENT);
        bool opp   = (goal.team != Goal::Team::OWN);

        if (own && left) {
            possibilities.push_back(arma::vec3({fd.goalpost_own_l[0], fd.goalpost_own_l[1], 0}));
        }
        if (own && right) {
            possibilities.push_back(arma::vec3({fd.goalpost_own_r[0], fd.goalpost_own_r[1], 0}));
        }
        if (opp && left) {
            possibilities.push_back(arma::vec3({fd.goalpost_opp_l[0], fd.goalpost_opp_l[1], 0}));
        }
        if (opp && right) {
            possibilities.push_back(arma::vec3({fd.goalpost_opp_r[0], fd.goalpost_opp_r[1], 0}));
        }

        return possibilities;
    }
}  // namespace localisation
}  // namespace module

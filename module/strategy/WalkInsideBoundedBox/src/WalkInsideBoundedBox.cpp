#include "WalkInsideBoundedBox.hpp"

#include "extension/Behaviour.hpp"
#include "extension/Configuration.hpp"

#include "message/input/GameState.hpp"
#include "message/input/Sensors.hpp"
#include "message/localisation/Ball.hpp"
#include "message/localisation/Field.hpp"
#include "message/strategy/WalkInsideBoundedBox.hpp"
#include "message/strategy/WalkToFieldPosition.hpp"

#include "utility/support/yaml_expression.hpp"

namespace module::strategy {

    using extension::Configuration;
    using DefendTask = message::strategy::WalkInsideBoundedBox;
    using utility::support::Expression;
    using Ball = message::localisation::Ball;
    using message::input::Sensors;
    using message::localisation::Field;
    using message::strategy::WalkToFieldPosition;

    WalkInsideBoundedBox::WalkInsideBoundedBox(std::unique_ptr<NUClear::Environment> environment)
        : BehaviourReactor(std::move(environment)) {
        on<Configuration>("WalkInsideBoundedBox.yaml").then([this](const Configuration& config) {
            // Use configuration here from file Defend.yaml
            this->log_level          = config["log_level"].as<NUClear::LogLevel>();
            cfg.bounded_region_x_min = config["bounded_region_x_min"].as<Expression>();
            cfg.bounded_region_x_max = config["bounded_region_x_max"].as<Expression>();
            cfg.bounded_region_y_min = config["bounded_region_y_min"].as<Expression>();
            cfg.bounded_region_y_max = config["bounded_region_y_max"].as<Expression>();
        });
        on<Provide<DefendTask>, Trigger<Ball>, With<Field>, With<Sensors>>().then(
            [this](const DefendTask& defend_task, const Ball& ball, const Field& field, const Sensors& sensor) {
                Eigen::Isometry3d Hfw = Eigen::Isometry3d(field.Hfw.cast<double>());
                Eigen::Isometry3d Hrw = Eigen::Isometry3d(sensor.Hrw.cast<double>());
                Eigen::Isometry3d Hfr = Hfw * Hrw.inverse();
                Eigen::Vector3d rBFf  = Hfw * ball.rBWw;
                Eigen::Vector3d rRFf  = Hfr.translation();

                // The distance of the robot from the ball
                double robot_distance_to_ball =
                    sqrt(abs(pow(rBFf.x() - rRFf.x(), 2.0) - pow(rBFf.y() - rRFf.y(), 2.0)));

                log<NUClear::DEBUG>("rRFf: ", rRFf.transpose());
                log<NUClear::DEBUG>("rBFf: ", rBFf.transpose());
                log<NUClear::DEBUG>("Dist: ", robot_distance_to_ball);

                // Check if the ball is in the defending region
                if (rBFf.x() > cfg.bounded_region_x_min && rBFf.x() < cfg.bounded_region_x_max
                    && rBFf.y() > cfg.bounded_region_y_max && rBFf.y() < cfg.bounded_region_y_min) {

                    log<NUClear::DEBUG>("Ball inside of defending region");

                    // Do nothing, play normally
                }
                else {  // Determines robots as ball has left bounded region
                    // Robot - Defending position on field
                    Eigen::Vector3d rDFf = Eigen::Vector3d::Zero();

                    log<NUClear::DEBUG>("rDFf x: ", rDFf.x());
                    log<NUClear::DEBUG>("rDFf y: ", rDFf.y());

                    // If ball is in a region parallel and outside own bounding box of robot we clamp in the y
                    // direction and move to 1m behind ball
                    if (rBFf.x() >= 0 && rBFf.y() > cfg.bounded_region_y_min) {
                        log<NUClear::DEBUG>("Ball is in own half and in other region");

                        // Calculate the defender position
                        // Clamps to x direction of the ball and bounding box an 1 metre behind the ball
                        rDFf.x() = std::clamp(rBFf.x(), cfg.bounded_region_x_min, cfg.bounded_region_x_max);
                        rDFf.y() = std::clamp(rBFf.y(), cfg.bounded_region_y_max, cfg.bounded_region_y_min);
                        rDFf.x() += 1.0;  // For positioning of robot 1 metre behind ball when ball is in own half but
                                          // not inside robots bounded box
                    }
                    else {
                        // Calculate the defender position
                        // Robot clamped to defending bounding box
                        rDFf.x() = std::clamp(rBFf.x(), cfg.bounded_region_x_min, cfg.bounded_region_x_max);
                        rDFf.y() = std::clamp(rBFf.y(), cfg.bounded_region_y_max, cfg.bounded_region_y_min);
                    }

                    // Walk to determined position given by vector rDFf
                    emit<Task>(std::make_unique<WalkToFieldPosition>(Eigen::Vector3f(rDFf.x(), rDFf.y(), 0), -M_PI));
                    log<NUClear::DEBUG>("Ball is outside of defending region");
                }
            });
    }

}  // namespace module::strategy

#include "FallingRelaxPlanner.hpp"

#include "extension/Behaviour.hpp"
#include "extension/Configuration.hpp"
#include "extension/behaviour/Script.hpp"

#include "message/actuation/Limbs.hpp"
#include "message/behaviour/state/Stability.hpp"
#include "message/input/Sensors.hpp"
#include "message/planning/PlanFallingRelax.hpp"
#include "message/skill/GetUp.hpp"

#include "utility/support/yaml_expression.hpp"

namespace module::planning {

    using extension::Configuration;
    using extension::behaviour::Script;
    using extension::behaviour::ScriptRequest;
    using message::actuation::BodySequence;
    using message::behaviour::state::Stability;
    using message::input::Sensors;
    using message::planning::PlanFallingRelax;
    using utility::support::Expression;

    double smooth(double value, double new_value, double alpha) {
        return alpha * value + (1.0 - alpha) * new_value;
    }

    /// @brief A state to categorise each of the properties we are monitoring
    enum class State {
        /// @brief This sensor believes the robot is stable
        STABLE,
        /// @brief This sensor believes the robot is unstable but is unsure if it is falling
        UNSTABLE,
        /// @brief This sensor believes the robot is falling
        FALLING
    };

    FallingRelaxPlanner::FallingRelaxPlanner(std::unique_ptr<NUClear::Environment> environment)
        : BehaviourReactor(std::move(environment)) {

        on<Configuration>("FallingRelaxPlanner.yaml").then([this](const Configuration& config) {
            this->log_level = config["log_level"].as<NUClear::LogLevel>();

            cfg.gyro_mag.mean       = config["gyro_mag"]["mean"].as<Expression>();
            cfg.gyro_mag.unstable   = config["gyro_mag"]["unstable"].as<Expression>();
            cfg.gyro_mag.falling    = config["gyro_mag"]["falling"].as<Expression>();
            cfg.gyro_mag.smoothing  = config["gyro_mag"]["smoothing"].as<Expression>();
            cfg.acc_mag.mean        = config["acc_mag"]["mean"].as<Expression>();
            cfg.acc_mag.unstable    = config["acc_mag"]["unstable"].as<Expression>();
            cfg.acc_mag.falling     = config["acc_mag"]["falling"].as<Expression>();
            cfg.acc_mag.smoothing   = config["acc_mag"]["smoothing"].as<Expression>();
            cfg.acc_angle.mean      = config["acc_angle"]["mean"].as<Expression>();
            cfg.acc_angle.unstable  = config["acc_angle"]["unstable"].as<Expression>();
            cfg.acc_angle.falling   = config["acc_angle"]["falling"].as<Expression>();
            cfg.acc_angle.smoothing = config["acc_angle"]["smoothing"].as<Expression>();
        });

        on<Provide<PlanFallingRelax>, Trigger<Sensors>>().then([this](const RunInfo& info, const Sensors& sensors) {
            // OTHER_TRIGGER means we ran because of a sensors update
            if (info.run_reason == RunInfo::OTHER_TRIGGER) {
                auto& a = sensors.accelerometer;
                auto& g = sensors.gyroscope;

                // Smooth the values we use to determine if we are falling
                gyro_mag  = smooth(gyro_mag,
                                  std::abs(std::abs(g.x()) + std::abs(g.y()) + std::abs(g.z()) - cfg.gyro_mag.mean),
                                  cfg.gyro_mag.smoothing);
                acc_mag   = smooth(acc_mag,  //
                                 std::abs(a.norm() - cfg.acc_mag.mean),
                                 cfg.acc_mag.smoothing);
                acc_angle = smooth(acc_angle,
                                   std::acos(std::min(1.0, std::abs(a.normalized().z())) - cfg.acc_angle.mean),
                                   cfg.acc_angle.smoothing);

                // Check if we are stable according to each sensor
                State gyro_mag_state  = gyro_mag < cfg.gyro_mag.unstable  ? State::STABLE
                                        : gyro_mag < cfg.gyro_mag.falling ? State::UNSTABLE
                                                                          : State::FALLING;
                State acc_mag_state   = acc_mag < cfg.acc_mag.unstable  ? State::STABLE
                                        : acc_mag < cfg.acc_mag.falling ? State::UNSTABLE
                                                                        : State::FALLING;
                State acc_angle_state = acc_angle < cfg.acc_angle.unstable  ? State::STABLE
                                        : acc_angle < cfg.acc_angle.falling ? State::UNSTABLE
                                                                            : State::FALLING;

                // Falling if at least two of the three checks are unstable or if any one of them is falling
                bool falling = (gyro_mag_state == State::FALLING || acc_mag_state == State::FALLING
                                || acc_angle_state == State::FALLING)
                               || (gyro_mag_state == State::UNSTABLE && acc_mag_state == State::UNSTABLE)
                               || (gyro_mag_state == State::UNSTABLE && acc_angle_state == State::UNSTABLE)
                               || (acc_mag_state == State::UNSTABLE && acc_angle_state == State::UNSTABLE);

                // We are falling
                if (falling) {
                    // We are falling! Relax the limbs!
                    emit(std::make_unique<Stability>(Stability::FALLING));
                    emit<Script>(std::make_unique<BodySequence>(), ScriptRequest{"Relax.yaml"});
                }
            }
            else {
                // Emit an idle task for every other reason
                emit(std::make_unique<Idle>());
            }
        });
    }

}  // namespace module::planning

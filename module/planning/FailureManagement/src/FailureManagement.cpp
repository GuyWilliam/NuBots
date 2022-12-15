#include "FailureManagement.hpp"

#include "extension/Behaviour.hpp"
#include "extension/Configuration.hpp"
#include "extension/behaviour/Script.hpp"

#include "message/behaviour/state/Stability.hpp"
#include "message/input/Sensors.hpp"
#include "message/motion/Limbs.hpp"
#include "message/planning/Recover.hpp"

#include "utility/support/yaml_expression.hpp"

namespace module::planning {

    using extension::Configuration;
    using extension::behaviour::Script;
    using extension::behaviour::ScriptRequest;
    using message::behaviour::state::Stability;
    using message::input::Sensors;
    using message::motion::BodySequence;
    using message::planning::Recover;
    using utility::support::Expression;

    enum class State { STABLE, UNSTABLE, FALLING };

    double smooth(double value, double new_value, double alpha) {
        return alpha * value + (1.0 - alpha) * new_value;
    }

    FailureManagement::FailureManagement(std::unique_ptr<NUClear::Environment> environment)
        : BehaviourReactor(std::move(environment)) {

        on<Configuration>("FailureManagement.yaml").then([this](const Configuration& config) {
            // Use configuration here from file FailureManagement.yaml
            this->log_level = config["log_level"].as<NUClear::LogLevel>();

            cfg.falling.gyro.mag.mean       = config["falling"]["gyro"]["mag"]["mean"].as<Expression>();
            cfg.falling.gyro.mag.unstable   = config["falling"]["gyro"]["mag"]["unstable"].as<Expression>();
            cfg.falling.gyro.mag.falling    = config["falling"]["gyro"]["mag"]["falling"].as<Expression>();
            cfg.falling.gyro.mag.smoothing  = config["falling"]["gyro"]["mag"]["smoothing"].as<Expression>();
            cfg.falling.acc.mag.mean        = config["falling"]["acc"]["mag"]["mean"].as<Expression>();
            cfg.falling.acc.mag.unstable    = config["falling"]["acc"]["mag"]["unstable"].as<Expression>();
            cfg.falling.acc.mag.falling     = config["falling"]["acc"]["mag"]["falling"].as<Expression>();
            cfg.falling.acc.mag.smoothing   = config["falling"]["acc"]["mag"]["smoothing"].as<Expression>();
            cfg.falling.acc.angle.mean      = config["falling"]["acc"]["angle"]["mean"].as<Expression>();
            cfg.falling.acc.angle.unstable  = config["falling"]["acc"]["angle"]["unstable"].as<Expression>();
            cfg.falling.acc.angle.falling   = config["falling"]["acc"]["angle"]["falling"].as<Expression>();
            cfg.falling.acc.angle.smoothing = config["falling"]["acc"]["angle"]["smoothing"].as<Expression>();

            cfg.getup.angle         = config["getup"]["angle"].as<Expression>();
            cfg.getup.gyro_recovery = config["getup"]["gyro_recovery"].as<Expression>();
            cfg.getup.acc_recovery  = config["getup"]["acc_recovery"].as<Expression>();
        });

        on<Provide<Recover>, Trigger<Sensors>>().then([this](const RunInfo& info, const Sensors& sensors) {
            switch (info.run_reason) {
                case RunInfo::OTHER_TRIGGER: {
                    // OTHER_TRIGGER means we ran because of a sensors update
                    auto& a = sensors.accelerometer;
                    auto& g = sensors.gyroscope;

                    // Smooth the values we use to determine if we are falling
                    gyro.mag  = smooth(gyro.mag,
                                      std::abs(std::abs(g.x()) + std::abs(g.y()) - cfg.falling.gyro.mag.mean),
                                      cfg.falling.gyro.mag.smoothing);
                    acc.mag   = smooth(acc.mag,  //
                                     std::abs(a.norm() - cfg.falling.acc.mag.mean),
                                     cfg.falling.acc.mag.smoothing);
                    acc.angle = smooth(acc.angle,
                                       std::abs(1.0 - std::abs(a.normalized().z()) - cfg.falling.acc.angle.mean),
                                       cfg.falling.acc.angle.smoothing);

                    // If we are getting up we ignore what's going on until we are done
                    if (getting_up) {
                        emit(std::make_unique<Idle>());
                    }
                    else {

                        // Check if we are stable according to each sensor
                        State gyro_mag_state  = gyro.mag < cfg.falling.gyro.mag.unstable  ? State::STABLE
                                                : gyro.mag < cfg.falling.gyro.mag.falling ? State::UNSTABLE
                                                                                          : State::FALLING;
                        State acc_mag_state   = acc.mag < cfg.falling.acc.mag.unstable  ? State::STABLE
                                                : acc.mag < cfg.falling.acc.mag.falling ? State::UNSTABLE
                                                                                        : State::FALLING;
                        State acc_angle_state = acc.angle < cfg.falling.acc.angle.unstable  ? State::STABLE
                                                : acc.angle < cfg.falling.acc.angle.falling ? State::UNSTABLE
                                                                                            : State::FALLING;

                        // Falling if at least two of the three checks are unstable or if any one of them is falling
                        bool falling = (gyro_mag_state == State::FALLING || acc_mag_state == State::FALLING
                                        || acc_angle_state == State::FALLING)
                                       || (gyro_mag_state == State::UNSTABLE && acc_mag_state == State::UNSTABLE)
                                       || (gyro_mag_state == State::UNSTABLE && acc_angle_state == State::UNSTABLE)
                                       || (acc_mag_state == State::UNSTABLE && acc_angle_state == State::UNSTABLE);

                        // We are falling
                        if (falling) {
                            // TODO emit stability falling
                            emit(std::make_unique<Stability>(Stability::FALLING));
                            emit<Script>(std::make_unique<BodySequence>(), ScriptRequest{"Relax.yaml"});
                        }
                        // We can get up if all our sensors are in the recovery range and we are lying down
                        else if (gyro.mag < cfg.getup.gyro_recovery && acc.mag < cfg.getup.acc_recovery
                                 && acc.angle < cfg.getup.angle) {

                            // I've fallen
                            getting_up = true;
                            emit(std::make_unique<Stability>(Stability::FALLEN));

                            // Now we work out which way we are facing, splitting into four quadrants
                            // using the accelerometer to get the four diagonals
                            bool a = sensors.accelerometer.x() > sensors.accelerometer.y();
                            bool b = sensors.accelerometer.x() > -sensors.accelerometer.y();

                            if (a && b) {
                                emit<Script>(std::make_unique<BodySequence>(), ScriptRequest{"GetUpFront.yaml"});
                            }
                            else if (a && !b) {
                                // TODO not sure if this is the right way to roll
                                emit<Script>(std::make_unique<BodySequence>(), ScriptRequest{"RollLeft.yaml"});
                            }
                            else if (!a && b) {
                                // TODO not sure if this is the right way to roll
                                emit<Script>(std::make_unique<BodySequence>(), ScriptRequest{"RollRight.yaml"});
                            }
                            else if (!a && !b) {
                                emit<Script>(std::make_unique<BodySequence>(), ScriptRequest{"GetUpBack.yaml"});
                            }
                        }
                    }
                } break;
                case RunInfo::SUBTASK_DONE: {
                    // SUBTASK_DONE means the script we executed finished running

                    // Finished getting up, so we no longer have a subtask
                    if (getting_up) {
                        // TODO what if we failed to get up?
                        getting_up = false;
                        emit(std::make_unique<Stability>(Stability::STANDING));
                    }
                    // Finished a relax script, just emit idle
                    else {
                        emit<Task>(std::make_unique<Idle>());
                    }
                } break;
                // We don't care about other run reasons
                default: emit<Task>(std::make_unique<Idle>()); break;
            }
        });
    }

}  // namespace module::planning

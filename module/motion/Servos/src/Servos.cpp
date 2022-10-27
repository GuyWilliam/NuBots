/*
 * This file is part of the NUbots Codebase.
 *
 * The NUbots Codebase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The NUbots Codebase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the NUbots Codebase.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2022 NUbots <nubots@nubots.net>
 */

#include "Servos.hpp"

#include "extension/Behaviour.hpp"
#include "extension/Configuration.hpp"

#include "message/input/Sensors.hpp"
#include "message/motion/Limbs.hpp"
#include "message/motion/ServoTarget.hpp"
#include "message/motion/Servos.hpp"

#include "utility/input/ServoID.hpp"

namespace module::motion {

    using extension::Configuration;
    using message::input::Sensors;
    using message::motion::ArmID;
    using message::motion::Head;
    using message::motion::HeadID;
    using message::motion::HeadPitch;
    using message::motion::HeadYaw;
    using message::motion::LeftAnklePitch;
    using message::motion::LeftAnkleRoll;
    using message::motion::LeftArm;
    using message::motion::LeftElbow;
    using message::motion::LeftHipPitch;
    using message::motion::LeftHipRoll;
    using message::motion::LeftHipYaw;
    using message::motion::LeftKnee;
    using message::motion::LeftLeg;
    using message::motion::LeftShoulderPitch;
    using message::motion::LeftShoulderRoll;
    using message::motion::LegID;
    using message::motion::RightAnklePitch;
    using message::motion::RightAnkleRoll;
    using message::motion::RightArm;
    using message::motion::RightElbow;
    using message::motion::RightHipPitch;
    using message::motion::RightHipRoll;
    using message::motion::RightHipYaw;
    using message::motion::RightKnee;
    using message::motion::RightLeg;
    using message::motion::RightShoulderPitch;
    using message::motion::RightShoulderRoll;
    using message::motion::ServoTarget;
    using utility::input::ServoID;

    Servos::Servos(std::unique_ptr<NUClear::Environment> environment) : BehaviourReactor(std::move(environment)) {

        on<Configuration>("Servos.yaml").then([this](const Configuration& cfg) {
            // Use configuration here from file Servos.yaml
            this->log_level = cfg["log_level"].as<NUClear::LogLevel>();
        });

        on<Provide<LeftLeg>,
           Needs<LeftHipYaw>,
           Needs<LeftHipRoll>,
           Needs<LeftHipPitch>,
           Needs<LeftKnee>,
           Needs<LeftAnklePitch>,
           Needs<LeftAnkleRoll>>()
            .then([this](const LeftLeg& leg,
                         const RunInfo& info,
                         const Uses<LeftHipYaw>& lhy,
                         const Uses<LeftHipRoll>& lhr,
                         const Uses<LeftHipPitch>& lhp,
                         const Uses<LeftKnee>& lk,
                         const Uses<LeftAnklePitch>& lap,
                         const Uses<LeftAnkleRoll>& lar) {
                // This is done when all of the servos are done, so check them all
                if (info.run_reason == RunInfo::RunReason::SUBTASK_DONE) {
                    if (lhy.done && lhr.done && lhp.done && lk.done && lap.done && lar.done) {
                        emit<Task>(std::make_unique<Done>());
                        return;
                    }
                    emit<Task>(std::make_unique<Idle>());
                    return;
                }

                // Emit tasks for each servo
                emit<Task>(std::make_unique<LeftHipYaw>(leg.servos[LegID::HIP_YAW]));
                emit<Task>(std::make_unique<LeftHipRoll>(leg.servos[LegID::HIP_ROLL]));
                emit<Task>(std::make_unique<LeftHipPitch>(leg.servos[LegID::HIP_PITCH]));
                emit<Task>(std::make_unique<LeftKnee>(leg.servos[LegID::KNEE]));
                emit<Task>(std::make_unique<LeftAnklePitch>(leg.servos[LegID::ANKLE_PITCH]));
                emit<Task>(std::make_unique<LeftAnkleRoll>(leg.servos[LegID::ANKLE_ROLL]));
            });

        on<Provide<RightLeg>,
           Needs<RightHipYaw>,
           Needs<RightHipRoll>,
           Needs<RightHipPitch>,
           Needs<RightKnee>,
           Needs<RightAnklePitch>,
           Needs<RightAnkleRoll>>()
            .then([this](const RightLeg& leg,
                         const RunInfo& info,
                         const Uses<RightHipYaw>& rhy,
                         const Uses<RightHipRoll>& rhr,
                         const Uses<RightHipPitch>& rhp,
                         const Uses<RightKnee>& rk,
                         const Uses<RightAnklePitch>& rap,
                         const Uses<RightAnkleRoll>& rar) {
                // This is done when all of the servos are done, so check them all
                if (info.run_reason == RunInfo::RunReason::SUBTASK_DONE) {
                    if (rhy.done && rhr.done && rhp.done && rk.done && rap.done && rar.done) {
                        emit<Task>(std::make_unique<Done>());
                        return;
                    }
                    emit<Task>(std::make_unique<Idle>());
                    return;
                }

                // Emit tasks for each servo
                emit<Task>(std::make_unique<RightHipYaw>(leg.servos[LegID::HIP_YAW]));
                emit<Task>(std::make_unique<RightHipRoll>(leg.servos[LegID::HIP_ROLL]));
                emit<Task>(std::make_unique<RightHipPitch>(leg.servos[LegID::HIP_PITCH]));
                emit<Task>(std::make_unique<RightKnee>(leg.servos[LegID::KNEE]));
                emit<Task>(std::make_unique<RightAnklePitch>(leg.servos[LegID::ANKLE_PITCH]));
                emit<Task>(std::make_unique<RightAnkleRoll>(leg.servos[LegID::ANKLE_ROLL]));
            });

        on<Provide<LeftArm>, Needs<LeftShoulderPitch>, Needs<LeftShoulderRoll>, Needs<LeftElbow>>().then(
            [this](const LeftArm& arm,
                   const RunInfo& info,
                   const Uses<LeftShoulderPitch>& lsp,
                   const Uses<LeftShoulderRoll>& lsr,
                   const Uses<LeftElbow>& le) {
                // This is done when all of the servos are done, so check them all
                if (info.run_reason == RunInfo::RunReason::SUBTASK_DONE) {
                    if (lsp.done && lsr.done && le.done) {
                        emit<Task>(std::make_unique<Done>());
                        return;
                    }
                    emit<Task>(std::make_unique<Idle>());
                    return;
                }

                // Emit tasks for each servo
                emit<Task>(std::make_unique<LeftShoulderPitch>(arm.servos[ArmID::SHOULDER_PITCH]));
                emit<Task>(std::make_unique<LeftShoulderRoll>(arm.servos[ArmID::SHOULDER_ROLL]));
                emit<Task>(std::make_unique<LeftElbow>(arm.servos[ArmID::ELBOW]));
            });

        on<Provide<RightArm>, Needs<RightShoulderPitch>, Needs<RightShoulderRoll>, Needs<RightElbow>>().then(
            [this](const RightArm& arm,
                   const RunInfo& info,
                   const Uses<RightShoulderPitch>& rsp,
                   const Uses<RightShoulderRoll>& rsr,
                   const Uses<RightElbow>& re) {
                // This is done when all of the servos are done, so check them all
                if (info.run_reason == RunInfo::RunReason::SUBTASK_DONE) {
                    if (rsp.done && rsr.done && re.done) {
                        emit<Task>(std::make_unique<Done>());
                        return;
                    }
                    emit<Task>(std::make_unique<Idle>());
                    return;
                }

                // Emit tasks for each servo
                emit<Task>(std::make_unique<RightShoulderPitch>(arm.servos[ArmID::SHOULDER_PITCH]));
                emit<Task>(std::make_unique<RightShoulderRoll>(arm.servos[ArmID::SHOULDER_ROLL]));
                emit<Task>(std::make_unique<RightElbow>(arm.servos[ArmID::ELBOW]));
            });

        on<Provide<Head>, Needs<HeadYaw>, Needs<HeadPitch>>().then(
            [this](const Head& head, const RunInfo& info, const Uses<HeadYaw>& hy, const Uses<HeadPitch>& hp) {
                // This is done when all of the servos are done, so check them all
                if (info.run_reason == RunInfo::RunReason::SUBTASK_DONE) {
                    if (hy.done && hp.done) {
                        emit<Task>(std::make_unique<Done>());
                        return;
                    }
                    emit<Task>(std::make_unique<Idle>());
                    return;
                }

                // Emit tasks for each servo
                emit<Task>(std::make_unique<HeadYaw>(head.servos[HeadID::YAW]));
                emit<Task>(std::make_unique<HeadPitch>(head.servos[HeadID::PITCH]));
            });


        /// @brief Sends a right shoulder pitch servo command as a normal servo target command for the platform module
        /// to use
        on<Provide<RightShoulderPitch>, Trigger<Sensors>>().then(
            [this](const RightShoulderPitch& servo, const Sensors& /* sensors */) {
                // If the time to reach the position is over, then stop requesting the position
                if (NUClear::clock::now() >= servo.command.time) {
                    emit<Task>(std::make_unique<Done>());
                    return;
                }
                emit(std::make_unique<ServoTarget>(servo.command.time,
                                                   ServoID::R_SHOULDER_PITCH,
                                                   servo.command.position,
                                                   servo.command.state.gain,
                                                   servo.command.state.torque));
            });


        /// @brief Sends a left shoulder pitch servo command as a normal servo target command for the platform module
        /// to use
        on<Provide<LeftShoulderPitch>, Trigger<Sensors>>().then(
            [this](const LeftShoulderPitch& servo, const Sensors& /* sensors */) {
                // If the time to reach the position is over, then stop requesting the position
                if (NUClear::clock::now() >= servo.command.time) {
                    emit<Task>(std::make_unique<Done>());
                    return;
                }
                emit(std::make_unique<ServoTarget>(servo.command.time,
                                                   ServoID::L_SHOULDER_PITCH,
                                                   servo.command.position,
                                                   servo.command.state.gain,
                                                   servo.command.state.torque));
            });


        /// @brief Sends a right shoulder roll servo command as a normal servo target command for the platform module
        /// to use
        on<Provide<RightShoulderRoll>, Trigger<Sensors>>().then(
            [this](const RightShoulderRoll& servo, const Sensors& /* sensors */) {
                // If the time to reach the position is over, then stop requesting the position
                if (NUClear::clock::now() >= servo.command.time) {
                    emit<Task>(std::make_unique<Done>());
                    return;
                }
                emit(std::make_unique<ServoTarget>(servo.command.time,
                                                   ServoID::R_SHOULDER_ROLL,
                                                   servo.command.position,
                                                   servo.command.state.gain,
                                                   servo.command.state.torque));
            });

        /// @brief Sends a left shoulder roll servo command as a normal servo target command for the platform module
        /// to use
        on<Provide<LeftShoulderRoll>, Trigger<Sensors>>().then(
            [this](const LeftShoulderRoll& servo, const Sensors& /* sensors */) {
                // If the time to reach the position is over, then stop requesting the position
                if (NUClear::clock::now() >= servo.command.time) {
                    emit<Task>(std::make_unique<Done>());
                    return;
                }
                emit(std::make_unique<ServoTarget>(servo.command.time,
                                                   ServoID::L_SHOULDER_ROLL,
                                                   servo.command.position,
                                                   servo.command.state.gain,
                                                   servo.command.state.torque));
            });

        /// @brief Sends a right elbow servo command as a normal servo target command for the platform module
        /// to use
        on<Provide<RightElbow>, Trigger<Sensors>>().then([this](const RightElbow& servo, const Sensors& /* sensors */) {
            // If the time to reach the position is over, then stop requesting the position
            if (NUClear::clock::now() >= servo.command.time) {
                emit<Task>(std::make_unique<Done>());
                return;
            }
            emit(std::make_unique<ServoTarget>(servo.command.time,
                                               ServoID::R_ELBOW,
                                               servo.command.position,
                                               servo.command.state.gain,
                                               servo.command.state.torque));
        });

        /// @brief Sends a left elbow servo command as a normal servo target command for the platform module
        /// to use
        on<Provide<LeftElbow>, Trigger<Sensors>>().then([this](const LeftElbow& servo, const Sensors& /* sensors */) {
            // If the time to reach the position is over, then stop requesting the position
            if (NUClear::clock::now() >= servo.command.time) {
                emit<Task>(std::make_unique<Done>());
                return;
            }
            emit(std::make_unique<ServoTarget>(servo.command.time,
                                               ServoID::L_ELBOW,
                                               servo.command.position,
                                               servo.command.state.gain,
                                               servo.command.state.torque));
        });

        /// @brief Sends a right hip yaw command as a normal servo target command for the platform module
        /// to use
        on<Provide<RightHipYaw>, Trigger<Sensors>>().then(
            [this](const RightHipYaw& servo, const Sensors& /* sensors */) {
                // If the time to reach the position is over, then stop requesting the position
                if (NUClear::clock::now() >= servo.command.time) {
                    emit<Task>(std::make_unique<Done>());
                    return;
                }
                emit(std::make_unique<ServoTarget>(servo.command.time,
                                                   ServoID::R_HIP_YAW,
                                                   servo.command.position,
                                                   servo.command.state.gain,
                                                   servo.command.state.torque));
            });

        /// @brief Sends a left hip yaw command as a normal servo target command for the platform module
        /// to use
        on<Provide<LeftHipYaw>, Trigger<Sensors>>().then([this](const LeftHipYaw& servo, const Sensors& /* sensors */) {
            // If the time to reach the position is over, then stop requesting the position
            if (NUClear::clock::now() >= servo.command.time) {
                emit<Task>(std::make_unique<Done>());
                return;
            }
            emit(std::make_unique<ServoTarget>(servo.command.time,
                                               ServoID::L_HIP_YAW,
                                               servo.command.position,
                                               servo.command.state.gain,
                                               servo.command.state.torque));
        });

        /// @brief Sends a right hip roll command as a normal servo target command for the platform module
        /// to use
        on<Provide<RightHipRoll>, Trigger<Sensors>>().then(
            [this](const RightHipRoll& servo, const Sensors& /* sensors */) {
                // If the time to reach the position is over, then stop requesting the position
                if (NUClear::clock::now() >= servo.command.time) {
                    emit<Task>(std::make_unique<Done>());
                    return;
                }
                emit(std::make_unique<ServoTarget>(servo.command.time,
                                                   ServoID::R_HIP_ROLL,
                                                   servo.command.position,
                                                   servo.command.state.gain,
                                                   servo.command.state.torque));
            });

        /// @brief Sends a left hip roll command as a normal servo target command for the platform module
        /// to use
        on<Provide<LeftHipRoll>, Trigger<Sensors>>().then(
            [this](const LeftHipRoll& servo, const Sensors& /* sensors */) {
                // If the time to reach the position is over, then stop requesting the position
                if (NUClear::clock::now() >= servo.command.time) {
                    emit<Task>(std::make_unique<Done>());
                    return;
                }
                emit(std::make_unique<ServoTarget>(servo.command.time,
                                                   ServoID::L_HIP_ROLL,
                                                   servo.command.position,
                                                   servo.command.state.gain,
                                                   servo.command.state.torque));
            });

        /// @brief Sends a right hip pitch command as a normal servo target command for the platform module
        /// to use
        on<Provide<RightHipPitch>, Trigger<Sensors>>().then(
            [this](const RightHipPitch& servo, const Sensors& /* sensors */) {
                // If the time to reach the position is over, then stop requesting the position
                if (NUClear::clock::now() >= servo.command.time) {
                    emit<Task>(std::make_unique<Done>());
                    return;
                }
                emit(std::make_unique<ServoTarget>(servo.command.time,
                                                   ServoID::R_HIP_PITCH,
                                                   servo.command.position,
                                                   servo.command.state.gain,
                                                   servo.command.state.torque));
            });

        /// @brief Sends a left hip pitch command as a normal servo target command for the platform module
        /// to use
        on<Provide<LeftHipPitch>, Trigger<Sensors>>().then(
            [this](const LeftHipPitch& servo, const Sensors& /* sensors */) {
                // If the time to reach the position is over, then stop requesting the position
                if (NUClear::clock::now() >= servo.command.time) {
                    emit<Task>(std::make_unique<Done>());
                    return;
                }
                emit(std::make_unique<ServoTarget>(servo.command.time,
                                                   ServoID::L_HIP_PITCH,
                                                   servo.command.position,
                                                   servo.command.state.gain,
                                                   servo.command.state.torque));
            });

        /// @brief Sends a right knee command as a normal servo target command for the platform module
        /// to use
        on<Provide<RightKnee>, Trigger<Sensors>>().then([this](const RightKnee& servo, const Sensors& /* sensors */) {
            // If the time to reach the position is over, then stop requesting the position
            if (NUClear::clock::now() >= servo.command.time) {
                emit<Task>(std::make_unique<Done>());
                return;
            }
            emit(std::make_unique<ServoTarget>(servo.command.time,
                                               ServoID::R_KNEE,
                                               servo.command.position,
                                               servo.command.state.gain,
                                               servo.command.state.torque));
        });

        /// @brief Sends a left knee command as a normal servo target command for the platform module
        /// to use
        on<Provide<LeftKnee>, Trigger<Sensors>>().then([this](const LeftKnee& servo, const Sensors& /* sensors */) {
            // If the time to reach the position is over, then stop requesting the position
            if (NUClear::clock::now() >= servo.command.time) {
                emit<Task>(std::make_unique<Done>());
                return;
            }
            emit(std::make_unique<ServoTarget>(servo.command.time,
                                               ServoID::L_KNEE,
                                               servo.command.position,
                                               servo.command.state.gain,
                                               servo.command.state.torque));
        });

        /// @brief Sends a right ankle pitch command as a normal servo target command for the platform module
        /// to use
        on<Provide<RightAnklePitch>, Trigger<Sensors>>().then(
            [this](const RightAnklePitch& servo, const Sensors& /* sensors */) {
                // If the time to reach the position is over, then stop requesting the position
                if (NUClear::clock::now() >= servo.command.time) {
                    emit<Task>(std::make_unique<Done>());
                    return;
                }
                emit(std::make_unique<ServoTarget>(servo.command.time,
                                                   ServoID::R_ANKLE_PITCH,
                                                   servo.command.position,
                                                   servo.command.state.gain,
                                                   servo.command.state.torque));
            });

        /// @brief Sends a left ankle pitch command as a normal servo target command for the platform module
        /// to use
        on<Provide<LeftAnklePitch>, Trigger<Sensors>>().then(
            [this](const LeftAnklePitch& servo, const Sensors& /* sensors */) {
                // If the time to reach the position is over, then stop requesting the position
                if (NUClear::clock::now() >= servo.command.time) {
                    emit<Task>(std::make_unique<Done>());
                    return;
                }
                emit(std::make_unique<ServoTarget>(servo.command.time,
                                                   ServoID::L_ANKLE_PITCH,
                                                   servo.command.position,
                                                   servo.command.state.gain,
                                                   servo.command.state.torque));
            });

        /// @brief Sends a right ankle roll command as a normal servo target command for the platform module
        /// to use
        on<Provide<RightAnkleRoll>, Trigger<Sensors>>().then(
            [this](const RightAnkleRoll& servo, const Sensors& /* sensors */) {
                // If the time to reach the position is over, then stop requesting the position
                if (NUClear::clock::now() >= servo.command.time) {
                    emit<Task>(std::make_unique<Done>());
                    return;
                }
                emit(std::make_unique<ServoTarget>(servo.command.time,
                                                   ServoID::R_ANKLE_ROLL,
                                                   servo.command.position,
                                                   servo.command.state.gain,
                                                   servo.command.state.torque));
            });

        /// @brief Sends a left ankle roll command as a normal servo target command for the platform module
        /// to use
        on<Provide<LeftAnkleRoll>, Trigger<Sensors>>().then(
            [this](const LeftAnkleRoll& servo, const Sensors& /* sensors */) {
                // If the time to reach the position is over, then stop requesting the position
                if (NUClear::clock::now() >= servo.command.time) {
                    emit<Task>(std::make_unique<Done>());
                    return;
                }
                emit(std::make_unique<ServoTarget>(servo.command.time,
                                                   ServoID::L_ANKLE_ROLL,
                                                   servo.command.position,
                                                   servo.command.state.gain,
                                                   servo.command.state.torque));
            });

        /// @brief Sends a head yaw command as a normal servo target command for the platform module
        /// to use
        on<Provide<HeadYaw>, Trigger<Sensors>>().then([this](const HeadYaw& servo, const Sensors& /* sensors */) {
            // If the time to reach the position is over, then stop requesting the position
            if (NUClear::clock::now() >= servo.command.time) {
                emit<Task>(std::make_unique<Done>());
                return;
            }
            emit(std::make_unique<ServoTarget>(servo.command.time,
                                               ServoID::HEAD_YAW,
                                               servo.command.position,
                                               servo.command.state.gain,
                                               servo.command.state.torque));
        });

        /// @brief Sends a head pitch command as a normal servo target command for the platform module
        /// to use
        on<Provide<HeadPitch>, Trigger<Sensors>>().then([this](const HeadPitch& servo, const Sensors& /* sensors */) {
            // If the time to reach the position is over, then stop requesting the position
            if (NUClear::clock::now() >= servo.command.time) {
                emit<Task>(std::make_unique<Done>());
                return;
            }
            emit(std::make_unique<ServoTarget>(servo.command.time,
                                               ServoID::HEAD_PITCH,
                                               servo.command.position,
                                               servo.command.state.gain,
                                               servo.command.state.torque));
        });
    }

}  // namespace module::motion

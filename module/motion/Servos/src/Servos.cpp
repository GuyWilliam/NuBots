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

#include "message/motion/Limbs.hpp"
#include "message/motion/Servos.hpp"

#include "utility/input/ServoID.hpp"

namespace module::motion {

    using extension::Configuration;
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
    using utility::input::ServoID;

    Servos::Servos(std::unique_ptr<NUClear::Environment> environment) : BehaviourReactor(std::move(environment)) {

        on<Configuration>("Servos.yaml").then([this](const Configuration& cfg) {
            // Use configuration here from file Servos.yaml
            this->log_level = cfg["log_level"].as<NUClear::LogLevel>();
        });

        // Create providers for each limb and the head
        add_group_provider<RightLeg,
                           RightHipYaw,
                           RightHipRoll,
                           RightHipPitch,
                           RightKnee,
                           RightAnklePitch,
                           RightAnkleRoll>();
        add_group_provider<LeftLeg, LeftHipYaw, LeftHipRoll, LeftHipPitch, LeftKnee, LeftAnklePitch, LeftAnkleRoll>();
        add_group_provider<RightArm, RightShoulderPitch, RightShoulderRoll, RightElbow>();
        add_group_provider<LeftArm, LeftShoulderPitch, LeftShoulderRoll, LeftElbow>();
        add_group_provider<Head, HeadYaw, HeadPitch>();

        // Create providers for each servo
        add_servo_provider<RightShoulderPitch, ServoID::Value::R_SHOULDER_PITCH>();
        add_servo_provider<LeftShoulderPitch, ServoID::Value::L_SHOULDER_PITCH>();
        add_servo_provider<RightShoulderRoll, ServoID::Value::R_SHOULDER_ROLL>();
        add_servo_provider<LeftShoulderRoll, ServoID::Value::L_SHOULDER_ROLL>();
        add_servo_provider<RightElbow, ServoID::Value::R_ELBOW>();
        add_servo_provider<LeftElbow, ServoID::Value::L_ELBOW>();
        add_servo_provider<RightHipYaw, ServoID::Value::R_HIP_YAW>();
        add_servo_provider<LeftHipYaw, ServoID::Value::L_HIP_YAW>();
        add_servo_provider<RightHipRoll, ServoID::Value::R_HIP_ROLL>();
        add_servo_provider<LeftHipRoll, ServoID::Value::L_HIP_ROLL>();
        add_servo_provider<RightHipPitch, ServoID::Value::R_HIP_PITCH>();
        add_servo_provider<LeftHipPitch, ServoID::Value::L_HIP_PITCH>();
        add_servo_provider<RightKnee, ServoID::Value::R_KNEE>();
        add_servo_provider<LeftKnee, ServoID::Value::L_KNEE>();
        add_servo_provider<RightAnklePitch, ServoID::Value::R_ANKLE_PITCH>();
        add_servo_provider<LeftAnklePitch, ServoID::Value::L_ANKLE_PITCH>();
        add_servo_provider<RightAnkleRoll, ServoID::Value::R_ANKLE_ROLL>();
        add_servo_provider<LeftAnkleRoll, ServoID::Value::L_ANKLE_ROLL>();
        add_servo_provider<HeadYaw, ServoID::Value::HEAD_YAW>();
        add_servo_provider<HeadPitch, ServoID::Value::HEAD_PITCH>();
    }

}  // namespace module::motion

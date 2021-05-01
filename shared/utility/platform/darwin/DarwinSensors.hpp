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
 * Copyright 2013 NUbots <nubots@nubots.net>
 */
#ifndef UTILITY_PLATFORM_DARWIN_DARWINSENSORS_HPP
#define UTILITY_PLATFORM_DARWIN_DARWINSENSORS_HPP

#include "message/platform/darwin/DarwinSensors.hpp"

#include "utility/input/ServoID.hpp"

namespace utility::platform::darwin {

    using ServoID = utility::input::ServoID;
    using message::platform::darwin::DarwinSensors;

    inline const DarwinSensors::Servo& getDarwinServo(ServoID servoId, const DarwinSensors& sensors) {

        switch (servoId.value) {
            case ServoID::R_SHOULDER_PITCH: return sensors.servo.r_shoulder_pitch;
            case ServoID::L_SHOULDER_PITCH: return sensors.servo.l_shoulder_pitch;
            case ServoID::R_SHOULDER_ROLL: return sensors.servo.r_shoulder_roll;
            case ServoID::L_SHOULDER_ROLL: return sensors.servo.l_shoulder_roll;
            case ServoID::R_ELBOW: return sensors.servo.r_elbow;
            case ServoID::L_ELBOW: return sensors.servo.l_elbow;
            case ServoID::R_HIP_YAW: return sensors.servo.r_hip_yaw;
            case ServoID::L_HIP_YAW: return sensors.servo.l_hip_yaw;
            case ServoID::R_HIP_ROLL: return sensors.servo.r_hip_roll;
            case ServoID::L_HIP_ROLL: return sensors.servo.l_hip_roll;
            case ServoID::R_HIP_PITCH: return sensors.servo.r_hip_pitch;
            case ServoID::L_HIP_PITCH: return sensors.servo.l_hip_pitch;
            case ServoID::R_KNEE: return sensors.servo.r_knee;
            case ServoID::L_KNEE: return sensors.servo.l_knee;
            case ServoID::R_ANKLE_PITCH: return sensors.servo.r_ankle_pitch;
            case ServoID::L_ANKLE_PITCH: return sensors.servo.l_ankle_pitch;
            case ServoID::R_ANKLE_ROLL: return sensors.servo.r_ankle_roll;
            case ServoID::L_ANKLE_ROLL: return sensors.servo.l_ankle_roll;
            case ServoID::HEAD_YAW: return sensors.servo.head_pan;
            case ServoID::HEAD_PITCH: return sensors.servo.head_tilt;

            default: throw std::runtime_error("Out of bounds");
        }
    }

    inline DarwinSensors::Servo& getDarwinServo(ServoID servoId, DarwinSensors& sensors) {

        switch (servoId.value) {
            case ServoID::R_SHOULDER_PITCH: return sensors.servo.r_shoulder_pitch;
            case ServoID::L_SHOULDER_PITCH: return sensors.servo.l_shoulder_pitch;
            case ServoID::R_SHOULDER_ROLL: return sensors.servo.r_shoulder_roll;
            case ServoID::L_SHOULDER_ROLL: return sensors.servo.l_shoulder_roll;
            case ServoID::R_ELBOW: return sensors.servo.r_elbow;
            case ServoID::L_ELBOW: return sensors.servo.l_elbow;
            case ServoID::R_HIP_YAW: return sensors.servo.r_hip_yaw;
            case ServoID::L_HIP_YAW: return sensors.servo.l_hip_yaw;
            case ServoID::R_HIP_ROLL: return sensors.servo.r_hip_roll;
            case ServoID::L_HIP_ROLL: return sensors.servo.l_hip_roll;
            case ServoID::R_HIP_PITCH: return sensors.servo.r_hip_pitch;
            case ServoID::L_HIP_PITCH: return sensors.servo.l_hip_pitch;
            case ServoID::R_KNEE: return sensors.servo.r_knee;
            case ServoID::L_KNEE: return sensors.servo.l_knee;
            case ServoID::R_ANKLE_PITCH: return sensors.servo.r_ankle_pitch;
            case ServoID::L_ANKLE_PITCH: return sensors.servo.l_ankle_pitch;
            case ServoID::R_ANKLE_ROLL: return sensors.servo.r_ankle_roll;
            case ServoID::L_ANKLE_ROLL: return sensors.servo.l_ankle_roll;
            case ServoID::HEAD_YAW: return sensors.servo.head_pan;
            case ServoID::HEAD_PITCH: return sensors.servo.head_tilt;

            default: throw std::runtime_error("Out of bounds");
        }
    }
}  // namespace utility::platform::darwin

#endif

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
 * Copyright 2023 NUbots <nubots@nubots.net>
 */

#include "utility/platform/RawSensors.hpp"

#include "SensorFilter.hpp"

namespace module::input {

    using utility::platform::getRawServo;
    using utility::platform::make_packet_error_string;
    using utility::platform::make_servo_hardware_error_string;

    using message::platform::ButtonLeftDown;
    using message::platform::ButtonLeftUp;
    using message::platform::ButtonMiddleDown;
    using message::platform::ButtonMiddleUp;

    void SensorFilter::update_raw_sensors(std::unique_ptr<Sensors>& sensors,
                                          const std::shared_ptr<const Sensors>& previous_sensors,
                                          const RawSensors& raw_sensors) {
        // Check for errors on the platform and FSRs
        if (raw_sensors.subcontroller_error != RawSensors::PacketError::PACKET_OK) {
            NUClear::log<NUClear::WARN>(make_packet_error_string("Platform", raw_sensors.subcontroller_error));
        }

        // **************** Servos ****************
        for (uint32_t id = 0; id < 20; ++id) {
            const auto& raw_servo        = getRawServo(id, raw_sensors);
            const auto& hardware_status = raw_servo.hardware_error;

            // Check for an error on the servo and report it
            if (hardware_status != RawSensors::HardwareError::HARDWARE_OK) {
                NUClear::log<NUClear::WARN>(make_servo_hardware_error_string(raw_servo, id));
            }

            // If the RawSensors message for this servo has an error, but we have a previous Sensors message available,
            // then for some fields we will want to selectively use the old Sensors value, otherwise we just use the new
            // values as is
            sensors->servo.emplace_back(
                hardware_status,
                id,
                raw_servo.torque_enabled,
                raw_servo.position_p_gain,
                raw_servo.position_i_gain,
                raw_servo.position_d_gain,
                raw_servo.goal_position,
                raw_servo.profile_velocity,
                /* If there is an encoder error, then use the last known good position */
                ((hardware_status == RawSensors::HardwareError::MOTOR_ENCODER) && previous_sensors)
                    ? previous_sensors->servo[id].present_position
                    : raw_servo.present_position,
                /* If there is an encoder error, then use the last known good velocity */
                ((hardware_status == RawSensors::HardwareError::MOTOR_ENCODER) && previous_sensors)
                    ? previous_sensors->servo[id].present_velocity
                    : raw_servo.present_velocity,
                /* We may get a RawSensors::HardwareError::OVERLOAD error, but in this case the load value isn't *wrong*
                   so it doesn't make sense to use the last "good" value */
                raw_servo.present_current,
                /* Similarly for a RawSensors::HardwareError::INPUT_VOLTAGE error, no special action is needed */
                raw_servo.voltage,
                /* And similarly here for a RawSensors::HardwareError::OVERHEATING error, we still pass the temperature
                   no matter what */
                static_cast<float>(raw_servo.temperature));
        }

        // **************** Accelerometer and Gyroscope ****************
        // If we have a previous Sensors and our platform has errors then reuse our last sensor value of the
        // accelerometer
        if ((raw_sensors.subcontroller_error != RawSensors::PacketError::PACKET_OK) && previous_sensors) {
            sensors->accelerometer = previous_sensors->accelerometer;
        }
        else {
            sensors->accelerometer = raw_sensors.accelerometer.cast<double>();
        }

        // If we have a previous Sensors message AND (our platform has errors OR the gyro is spinning too fast) then
        // reuse our last sensor value of the gyroscope. Note: One of the gyros would occasionally
        // throw massive numbers without an error flag and if our hardware is working as intended, it should never
        // read that we're spinning at 2 revs/s
        if (previous_sensors
            && ((raw_sensors.subcontroller_error != RawSensors::PacketError::PACKET_OK)
                || raw_sensors.gyroscope.norm() > 4.0 * M_PI)) {
            NUClear::log<NUClear::WARN>("Bad gyroscope value", raw_sensors.gyroscope.norm());
            sensors->gyroscope = previous_sensors->gyroscope;
        }
        else {
            sensors->gyroscope = raw_sensors.gyroscope.cast<double>();
        }

        // **************** Timestamp ****************
        sensors->timestamp = raw_sensors.timestamp;

        // **************** Battery Voltage  ****************
        // Update the current battery voltage of the whole robot
        sensors->voltage = raw_sensors.battery;

        // **************** Buttons and LEDs ****************
        sensors->button.reserve(2);
        sensors->button.emplace_back(0, raw_sensors.buttons.left);
        sensors->button.emplace_back(1, raw_sensors.buttons.middle);
        sensors->led.reserve(5);
        sensors->led.emplace_back(0, raw_sensors.led_panel.led2 ? 0xFF0000 : 0);
        sensors->led.emplace_back(1, raw_sensors.led_panel.led3 ? 0xFF0000 : 0);
        sensors->led.emplace_back(2, raw_sensors.led_panel.led4 ? 0xFF0000 : 0);
        sensors->led.emplace_back(3, raw_sensors.head_led.RGB);  // Head
        sensors->led.emplace_back(4, raw_sensors.eye_led.RGB);   // Eye
    }

    void SensorFilter::detect_button_press(const std::list<std::shared_ptr<const RawSensors>>& sensors) {
        int left_count   = 0;
        int middle_count = 0;
        // If we have any downs in the last 20 frames then we are button pushed
        for (const auto& s : sensors) {
            if (s->buttons.left && (s->subcontroller_error == 0u)) {
                ++left_count;
            }
            if (s->buttons.middle && (s->subcontroller_error == 0u)) {
                ++middle_count;
            }
        }
        bool new_left_down   = left_count > cfg.buttons.debounce_threshold;
        bool new_middle_down = middle_count > cfg.buttons.debounce_threshold;
        if (new_left_down != left_down) {
            left_down = new_left_down;
            if (new_left_down) {
                log<NUClear::INFO>("Left Button Down");
                emit(std::make_unique<ButtonLeftDown>());
            }
            else {
                log<NUClear::INFO>("Left Button Up");
                emit(std::make_unique<ButtonLeftUp>());
            }
        }
        if (new_middle_down != middle_down) {
            middle_down = new_middle_down;
            if (new_middle_down) {
                log<NUClear::INFO>("Middle Button Down");
                emit(std::make_unique<ButtonMiddleDown>());
            }
            else {
                log<NUClear::INFO>("Middle Button Up");
                emit(std::make_unique<ButtonMiddleUp>());
            }
        }
    }

}  // namespace module::input

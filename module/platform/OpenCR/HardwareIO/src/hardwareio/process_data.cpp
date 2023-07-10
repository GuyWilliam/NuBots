#include <fmt/format.h>

#include "Convert.hpp"
#include "HardwareIO.hpp"

#include "message/input/Buttons.hpp"
#include "message/output/Buzzer.hpp"

#include "utility/math/comparison.hpp"

namespace module::platform::OpenCR {

    using message::input::ButtonLeftDown;
    using message::output::Buzzer;
    using message::platform::RawSensors;
    using message::platform::StatusReturn;

    /*
        Process the status return packet data
    */

    void HardwareIO::process_model_information(const StatusReturn& packet) {
        uint16_t model  = (packet.data[1] << 8) | packet.data[0];
        uint8_t version = packet.data[2];
        log<NUClear::INFO>(fmt::format("OpenCR Model...........: {:#06X}", model));
        log<NUClear::INFO>(fmt::format("OpenCR Firmware Version: {:#04X}", version));
    }

    void HardwareIO::process_opencr_data(const StatusReturn& packet) {
        const OpenCRReadData data = *(reinterpret_cast<const OpenCRReadData*>(packet.data.data()));

        // 00000321
        // LED_1 = 0x01
        // LED_2 = 0x02
        // LED_3 = 0x04
        opencr_state.led_panel = {bool(data.led & 0x01), bool(data.led & 0x02), bool(data.led & 0x04)};

        // 0BBBBBGG GGGRRRRR
        // R = 0x001F
        // G = 0x02E0
        // B = 0x7C00
        uint32_t RGB = 0;
        RGB |= uint8_t(data.rgb_led & 0x001F) << 16;  // R
        RGB |= uint8_t(data.rgb_led & 0x02E0) << 8;   // G
        RGB |= uint8_t(data.rgb_led & 0x7C00);        // B
        opencr_state.head_led = {RGB};

        // 00004321
        // Button_4 = Not used
        // Button Right (Reset) = 0x01
        // Button Middle = 0x02
        // Button Left = 0x04
        opencr_state.buttons = {bool(data.button & 0x04), bool(data.button & 0x02), bool(data.button & 0x01)};

        // If the left (black) button is pressed trigger field localisation reset
        if (opencr_state.buttons.left) {
            emit(std::make_unique<ButtonLeftDown>());
        }

        opencr_state.gyro = Eigen::Vector3f(convert::gyro(data.gyro[0]),    // X
                                            -convert::gyro(data.gyro[1]),   // Y
                                            -convert::gyro(data.gyro[2]));  // Z

        opencr_state.acc = Eigen::Vector3f(convert::acc(data.acc[0]),    // X
                                           -convert::acc(data.acc[1]),   // Y
                                           -convert::acc(data.acc[2]));  // Z
        // Command send/receive errors only
        opencr_state.error_flags = packet.error;

        // Work out a battery charged percentage
        battery_state.current_voltage = convert::voltage(data.voltage);
        float percentage              = std::max(0.0f,
                                    (battery_state.current_voltage - battery_state.flat_voltage)
                                        / (battery_state.charged_voltage - battery_state.flat_voltage));

        // Battery percentage has changed, recalculate LEDs
        if (!utility::math::almost_equal(percentage, battery_state.percentage, 2)) {
            battery_state.dirty      = true;
            battery_state.percentage = percentage;

            uint32_t ledr            = 0;
            std::array<bool, 3> ledp = {false, false, false};

            if (battery_state.percentage > 0.90f) {
                ledp = {true, true, true};
                ledr = (uint8_t(0x00) << 16) | (uint8_t(0xFF) << 8) | uint8_t(0x00);
            }
            else if (battery_state.percentage > 0.70f) {
                ledp = {false, true, true};
                ledr = (uint8_t(0x00) << 16) | (uint8_t(0xFF) << 8) | uint8_t(0x00);
            }
            else if (battery_state.percentage > 0.50f) {
                ledp = {false, false, true};
                ledr = (uint8_t(0x00) << 16) | (uint8_t(0xFF) << 8) | uint8_t(0x00);
            }
            else if (battery_state.percentage > 0.30f) {
                ledp = {false, false, false};
                ledr = (uint8_t(0x00) << 16) | (uint8_t(0xFF) << 8) | uint8_t(0x00);
            }
            else if (battery_state.percentage > 0.20f) {
                ledp = {false, false, false};
                ledr = (uint8_t(0xFF) << 16) | (uint8_t(0x00) << 8) | uint8_t(0x00);
            }
            else if (battery_state.percentage > 0) {
                ledp = {false, false, false};
                ledr = (uint8_t(0xFF) << 16) | (uint8_t(0x00) << 8) | uint8_t(0x00);
            }
            // Error in reading voltage blue
            else {
                ledp = {false, false, false};
                ledr = (uint8_t(0x00) << 16) | (uint8_t(0x00) << 8) | uint8_t(0xFF);
            }
            emit(std::make_unique<RawSensors::LEDPanel>(ledp[2], ledp[1], ledp[0]));
            emit(std::make_unique<RawSensors::HeadLED>(ledr));
        }
    }

    void HardwareIO::process_servo_data(const StatusReturn& packet) {
        const DynamixelServoReadData data = *(reinterpret_cast<const DynamixelServoReadData*>(packet.data.data()));

        // IDs are 1..20 so need to be converted for the servo_states index
        uint8_t servo_index = packet.id - 1;

        servo_states[servo_index].torque_enabled = (data.torque_enable == 1);

        // Although they're stored in the servo state here, packet errors are combined and processed all at once as
        // subcontroller errors in the RawSensors message
        servo_states[servo_index].packet_error = packet.error;

        // Servo error status from control table, NOT dynamixel status packet error.
        servo_states[servo_index].hardware_error = data.hardware_error_status;

        servo_states[servo_index].present_pwm      = convert::PWM(data.present_pwm);
        servo_states[servo_index].present_current  = convert::current(data.present_current);
        servo_states[servo_index].present_velocity = convert::velocity(data.present_velocity);  // todo: check
        servo_states[servo_index].present_position =
            convert::position(servo_index, data.present_position, nugus.servo_direction, nugus.servo_offset);
        servo_states[servo_index].voltage     = convert::voltage(data.present_voltage);
        servo_states[servo_index].temperature = convert::temperature(data.present_temperature);

        for (const auto& servo : servo_states) {
            if (servo.temperature > cfg.alarms.temperature.level) {
                emit(std::make_unique<Buzzer>(cfg.alarms.temperature.buzzer_frequency));
                break;
            }
        }

        // If this servo has not been initialised yet, set the goal states to the current states
        if (!servo_states[servo_index].initialised) {
            servo_states[servo_index].goal_position = servo_states[servo_index].present_position;
            servo_states[servo_index].torque        = servo_states[servo_index].torque_enabled ? 1.0f : 0.0f;
            servo_states[servo_index].initialised   = true;
        }
    }

}  // namespace module::platform::OpenCR

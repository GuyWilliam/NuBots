#include <fmt/format.h>

#include "Convert.hpp"
#include "HardwareIO.hpp"

#include "utility/math/comparison.hpp"

namespace module::platform::openCR {

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

        opencr_state.gyro = Eigen::Vector3f(convert::gyro(data.gyro[2]),    // X
                                            convert::gyro(data.gyro[1]),    // Y
                                            -convert::gyro(data.gyro[0]));  // Z

        opencr_state.acc = Eigen::Vector3f(convert::acc(data.acc[0]),   // X
                                           convert::acc(data.acc[1]),   // Y
                                           convert::acc(data.acc[2]));  // Z

        // Command send/receive errors only
        opencr_state.alert_flag   = static_cast<bool>(packet.alert);
        opencr_state.error_number = static_cast<int>(packet.error);

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

        // Servo error status, NOT dynamixel status packet error.
        servo_states[servo_index].error_flags = data.hardware_error_status;

        // Print error flags if there is an error
        /**
         * Bit      Item	                        Description
         * Bit 7	-	                            Unused, Always ‘0’
         * Bit 6	-	                            Unused, Always ‘0’
         * Bit 5	Overload Error(default)	        Detects that persistent load that exceeds maximum output
         * Bit 4	Electrical Shock Error(default)	Detects electric shock on the circuit or insufficient power to
         * operate the motor Bit 3	Motor Encoder Error	            Detects malfunction of the motor encoder Bit 2
         * Overheating Error(default)	    Detects that internal temperature exceeds the configured operating
         * temperature Bit 1	-	                            Unused, Always ‘0’ Bit 0	Input Voltage Error Detects
         * that input voltage exceeds the configured operating voltage
         */
        if (servo_states[servo_index].error_flags != 0) {
            log<NUClear::ERROR>(
                fmt::format("Servo {} error: {:#010b}", servo_index + 1, servo_states[servo_index].error_flags));
        }

        servo_states[servo_index].present_pwm      = convert::PWM(data.present_pwm);
        servo_states[servo_index].present_current  = convert::current(data.present_current);
        servo_states[servo_index].present_velocity = convert::velocity(data.present_velocity);  // todo: check
        servo_states[servo_index].present_position =
            convert::position(servo_index, data.present_position, nugus.servo_direction, nugus.servo_offset);
        servo_states[servo_index].voltage     = convert::voltage(data.present_voltage);
        servo_states[servo_index].temperature = convert::temperature(data.present_temperature);
    }

}  // namespace module::platform::openCR

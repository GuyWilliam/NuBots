#ifndef MODULE_PLATFORM_OPENCR_HARDWAREIO_HPP
#define MODULE_PLATFORM_OPENCR_HARDWAREIO_HPP

#include <Eigen/Core>
#include <map>
#include <nuclear>

#include "NUgus.hpp"
#include "dynamixel/v2/Dynamixel.hpp"

#include "message/platform/RawSensors.hpp"
#include "message/platform/ServoLED.hpp"
#include "message/platform/StatusReturn.hpp"

#include "utility/io/uart.hpp"
#include "utility/platform/RawSensors.hpp"

namespace module::platform::OpenCR {

    class HardwareIO : public NUClear::Reactor {

    public:
        /// @brief Called by the powerplant to build and setup the HardwareIO reactor.
        explicit HardwareIO(std::unique_ptr<NUClear::Environment> environment);

    private:
        /// @brief Manages the connection with OpenCR
        utility::io::uart opencr{};

        /// @brief How frequently the servos are read
        static constexpr int UPDATE_FREQUENCY = 90;

        /// @brief Contains device information specific to the NUgus robot
        NUgus nugus{};

        uint32_t byte_wait   = 0;
        uint32_t packet_wait = 0;

        /// @brief Maps device IDs to expected packet data
        enum class PacketTypes : uint8_t { MODEL_INFORMATION, OPENCR_DATA, SERVO_DATA, FSR_DATA };
        std::map<uint8_t, std::vector<PacketTypes>> packet_queue;

        /// @see opencr_state
        struct OpenCRState {
            bool dirty = false;

            /// @brief State of the LED panel, whether the lights are on or off
            message::platform::RawSensors::LEDPanel led_panel = {false, false, false};

            /// @brief Colour of the head LED
            ///  @details In the form of 0x00, 0xRR, 0xGG, 0xBB
            message::platform::RawSensors::HeadLED head_led = {0x0000FF00};

            /// @brief Colour of the eye LED
            ///  @details In the form of 0x00, 0xRR, 0xGG, 0xBB
            message::platform::RawSensors::EyeLED eye_led = {0x000000FF};

            /// @brief Pushed state of the buttons - left, middle, right
            message::platform::RawSensors::Buttons buttons = {false, false, false};

            /// @brief Accelerometer value of the OpenCR IMU
            Eigen::Vector3f acc = Eigen::Vector3f::Zero();

            /// @brief Gyroscope value of the OpenCR IMU
            Eigen::Vector3f gyro = Eigen::Vector3f::Zero();

            /// @brief Buzzer sound level of the OpenCR device
            uint16_t buzzer = 0;

            /// @brief Error status of the OpenCR device
            bool alert_flag = false;

            /// @brief Specific error from the OpenCR device
            uint8_t error_number = 0;
        };

        /// @see battery_state
        struct Battery {
            float charged_voltage = 0.0f;
            float nominal_voltage = 0.0f;
            float flat_voltage    = 0.0f;
            float current_voltage = 0.0f;
            float percentage      = 0.0f;
            bool dirty            = false;
        };

        /// @see servo_states
        struct ServoState {
            /// @brief True if we need to write new values to the hardware
            bool dirty = false;

            /// @brief Current error state of the servo
            /// @warning different to the dynamixel packet error status
            uint8_t error_flags = 0;

            /// @brief True if we simulate where we think the servos should be
            /// @note that we still write the commands to hardware
            bool simulated = false;

            /// @brief Our internal system torque target, this is never sent to the servo
            int torque = 0;
            /// @brief Control table value for internal servo state
            bool torque_enabled = true;

            // Cached values that are never read
            float velocity_i_gain      = 1920.0f / 65536.0f;  // ROBOTIS default
            float velocity_p_gain      = 100.0f / 128.0f;     // ROBOTIS default
            float position_d_gain      = 0.0f;
            float position_i_gain      = 0.0f;
            float position_p_gain      = 850.0f / 128.0f;  // ROBOTIS default
            float feedforward_1st_gain = 0.0f;
            float feedforward_2nd_gain = 0.0f;
            float goal_pwm             = 885.0f;               // ROBOTIS default
            float goal_current         = 6.52176f / 0.00336f;  // ROBOTIS default
            float goal_velocity        = 1.08775f;             // ROBOTIS default
            float goal_position        = 0.0f;
            float profile_acceleration = 0.0f;

            /// @brief The target velocity of the servo
            /// @details Replaces moving speed in v1 protocol
            float profile_velocity = 0.0f;

            // Values that are either simulated or read
            float present_pwm      = 0.0f;
            float present_current  = 0.0f;
            float present_velocity = 0.0f;
            float present_position = 0.0f;
            float voltage          = 0.0f;
            float temperature      = 0.0f;
        };

        /**
         * @brief Our state for our OpenCR for variables we send to it
         * Written to by processOpenCRData() and Read by constructSensors()
         */
        OpenCRState opencr_state{};

        /**
         * @brief Our state for our servos for variables we send to it
         * Written to by processServoData() and Read by constructSensors()
         */
        std::array<ServoState, 20> servo_states{};

        /**
         * @brief Our state for our battery
         * Written to by processOpenCRData() and Read by constructSensors()
         */
        Battery battery_state{};

        /**
         * @brief Reads information from an OpenCR packet and logs the model and firmware version.
         * @param packet a preprocessed OpenCR packet
         */
        void process_model_information(const message::platform::StatusReturn& packet);

        /**
         * @brief Reads information from an OpenCR packet and populates opencr_state and battery_state
         * @param packet a preprocessed OpenCR packet
         */
        void process_opencr_data(const message::platform::StatusReturn& packet);

        /**
         * @brief Reads information from an OpenCR packet and populates servo_states
         * @param packet a preprocessed OpenCR packet
         * @note Although we do a Sync Write to all servos, data is returned one by one
         */
        void process_servo_data(const message::platform::StatusReturn& packet);

        /**
         * @brief Reads info from the state variables and processes it into a RawSensors message
         * @return A RawSensors message created from the current state variables
         */
        message::platform::RawSensors construct_sensors();

        /// @brief Runs the setup for the devices
        void startup();

        /// @brief handles a response packet from the device
        void handle_response();

        /// @brief handle sending a request to the OpenCR device
        void send_opencr_request();

        /// @brief handle sending a request to the servo devices
        void send_servo_request();

        /// @brief Check if we're currently waiting on any servo packets
        /// @returns ID of FIRST servo we're waiting on, or 0 if none
        uint8_t servo_waiting();

        /// @brief Check if we're currently waiting on any OpenCR packets
        /// @returns number of OpenCR packets waiting
        uint8_t opencr_waiting();

        /// @brief Check if we're currently waiting on any packets
        /// @returns ID of FIRST device we're waiting on, or 0 if none
        uint8_t queue_item_waiting();

        /// @brief clear all packet queues
        /// @returns the number of packets cleared
        int queue_clear_all();
    };

}  // namespace module::platform::OpenCR

#endif  // MODULE_PLATFORM_OPENCR_HARDWAREIO_H

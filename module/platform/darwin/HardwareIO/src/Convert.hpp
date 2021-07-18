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

#ifndef MODULES_PLATFORM_DARWIN_CONVERT_HPP
#define MODULES_PLATFORM_DARWIN_CONVERT_HPP

#include <nuclear>

namespace module::platform::darwin {

    /**
     * @author Trent Houliston
     */
    struct Convert {

        /// The value from the Darwin is between 0 and 1023, representing a value between -4g and 4g.
        /// This means 512 = 0
        static constexpr float ACCELEROMETER_CONVERSION_FACTOR = (4.0f * 9.80665f) / 512.0f;

        /// The Gyrosocope value from the Darwin is between 0 and 1023, representing a value between -500 and
        /// 500 degrees per second. This means 512 = 0
        /// 1880.0 is an empirically measured value for this factor, which was calculated by spinning the robot on an
        /// office chair and comparing how far the robot thought it spun to how much it actually spun.
        /// Empirical estimates like this which don't match the data sheet are SUSPECT, and should be rechecked
        /// periodically
        static constexpr float GYROSCOPE_CONVERSION_FACTOR = (1880.0f * (float(M_PI) / 180.0f)) / 512.0f;

        /// The value that comes from the darwin is measured in decivolts (0.1 of a volt)
        static constexpr float VOLTAGE_CONVERSION_FACTOR = 0.1f;

        /// The FSR values that are measured by the darwins feet are measured in millinewtons
        static constexpr float FSR_FORCE_CONVERSION_FACTOR = 0.001f;

        /// The gain is a value between 0 and 254, we want a value between 0 and 100
        static constexpr float GAIN_CONVERSION_FACTOR = 100.0f / 254.0f;

        /// The angle is given as a value between 0 and 4095
        static constexpr float POSITION_CONVERSION_FACTOR = (2.0f * float(M_PI)) / 4095.0f;

        /// The load is measured as a value between 0 and 2047 where the 10th bit specifies direction and 1024 = 0
        /// We convert it to a value between -1 and 1 (percentage)
        static constexpr float LOAD_CONVERSION_FACTOR = 1.0f / 1023.0f;

        /// The torque limit is measured as a value between 0 and 1023. We use it between 0 and 100
        static constexpr float TORQUE_LIMIT_CONVERSION_FACTOR = 100.0f / 1023.0f;

        /// The temperatures are given in degrees anyway
        static constexpr float TEMPERATURE_CONVERSION_FACTOR = 1.0f;

        /// The MX64 and MX106 both measure speed between 0 and 1023, with unit of about 0.114rpm
        /// If the speed is 1023, then the speed is about 116.62 == 0.114 * 1023, per the datasheet
        static constexpr float SPEED_CONVERSION_FACTOR = (0.114f * 2.0f * float(M_PI)) / 60.0f;

        /// Picks which direction a motor should be measured in (forward or reverse) -- configurable based on
        /// the specific humanoid being used.
        static int8_t SERVO_DIRECTION[20];

        /// Offsets the radian angles of motors to change their 0 position -- configurable based on the specific
        /// humanoid being used.
        static float SERVO_OFFSET[20];

        template <int bit>
        static bool getBit(const uint16_t& value) {
            return (value & (1 << bit)) == (1 << bit);
        }

        static float accelerometer(const uint16_t& value);
        static float gyroscope(const uint16_t& value);
        static float voltage(const uint8_t& value);
        static float fsrForce(const uint16_t& value);
        static float fsrCentre(const bool& left, const uint8_t& value);

        static std::tuple<uint8_t, uint8_t, uint8_t> colourLED(const uint16_t& value);
        static uint16_t colourLEDInverse(const uint8_t& r, const uint8_t& g, const uint8_t& b);

        static float gain(const uint8_t& value);
        static uint8_t gainInverse(const float& value);

        static float servoPosition(const int& id, const uint16_t& value);
        static uint16_t servoPositionInverse(const int& id, const float& value);

        static float servoSpeed(const int& id, const uint16_t& value);
        static uint16_t servoSpeedInverse(const float& value);

        static float torqueLimit(const uint16_t& value);
        static uint16_t torqueLimitInverse(const float& value);

        static float servoLoad(const int& id, const uint16_t& value);

        static float temperature(const uint8_t& value);
    };
}  // namespace module::platform::darwin
#endif

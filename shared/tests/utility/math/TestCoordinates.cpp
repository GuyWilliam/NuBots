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

#include <Eigen/Core>
#include <array>
#include <catch.hpp>
#include <cfloat>
#include <cmath>
#include <iomanip>
#include <utility>

#include "utility/math/coordinates.h"
//#include "utility/math/quaternion.h"

static constexpr double ERROR_THRESHOLD = 1e-6;
// static constexpr double DBL_LWR_INVLD_EDGE = -1e-8;
// static constexpr double DBL_LWR_VALID_EDGE = 1e-8;


// vec3 cartesian test coords
static const std::array<Eigen::Vector3d, 15> cart_coords = {
    // NOTE: Get rid of min and max double vals - causing inf return values.
    // Add comments in the method being tested to explain.
    Eigen::Vector3d(0, 0, 0),
    Eigen::Vector3d(DBL_MIN, 0, 0),
    Eigen::Vector3d(0, DBL_MIN, 0),
    Eigen::Vector3d(0, 0, DBL_MIN),
    Eigen::Vector3d(0, DBL_MIN, DBL_MIN),
    Eigen::Vector3d(DBL_MIN, 0, DBL_MIN),
    Eigen::Vector3d(DBL_MIN, DBL_MIN, 0),
    Eigen::Vector3d(DBL_MIN, DBL_MIN, DBL_MIN),
    Eigen::Vector3d(DBL_MAX, 0, 0),  // error (r = inf)
    Eigen::Vector3d(0, DBL_MAX, 0),
    Eigen::Vector3d(0, 0, DBL_MAX),
    Eigen::Vector3d(0, DBL_MAX, DBL_MAX),
    Eigen::Vector3d(DBL_MAX, 0, DBL_MAX),
    Eigen::Vector3d(DBL_MAX, DBL_MAX, 0),
    Eigen::Vector3d(DBL_MAX, DBL_MAX, DBL_MAX)
    // TODO: remove edge cases
    // TODO: Add around 200 test values in between edge cases
};

// pre-calculated cartesian to spherical results
static const std::array<Eigen::Vector3d, 41> cartToSpher_results = {
    Eigen::Vector3d(0, 0, 0),
    Eigen::Vector3d(1.175494350822288e-38, 0, 0),
    Eigen::Vector3d(1.175494350822288e-38, 1.570796326794897e+00, 0),
    Eigen::Vector3d(1.175494350822288e-38, 0, 0),
    Eigen::Vector3d(1.662400053425836e-38, 1.570796326794897e+00, 0),
    Eigen::Vector3d(1.662400053425836e-38, 0, 7.853981633974482e-01),
    Eigen::Vector3d(1.662400053425836e-38, 7.853981633974483e-01, 0),
    Eigen::Vector3d(2.036015939634396e-38, 7.853981633974483e-01, 6.154797086703874e-01),
    Eigen::Vector3d(3.402823466385289e+38, 0, 0),  // error (inf)
    Eigen::Vector3d(3.402823466385289e+38, 1.570796326794897e+00, 0),
    Eigen::Vector3d(3.402823466385289e+38, 0, 0),
    Eigen::Vector3d(4.812319096523503e+38, 1.570796326794897e+00, 0),
    Eigen::Vector3d(4.812319096523503e+38, 0, 7.853981633974484e-01),
    Eigen::Vector3d(4.812319096523503e+38, 7.853981633974483e-01, 0),
    Eigen::Vector3d(5.893863132966965e+38, 7.853981633974483e-01, 6.154797086703874e-01)

};

// vec3 spherical test coords
// distance, theta, phi
// NOTE: Should be MatrixBase<double, 3, 1>
static const std::array<Eigen::Vector3d, 65> spher_coords = {
    // NOTE: should the valid/invalid radial boundaries be tested with each angle value?
    // Edge cases
    //
    Eigen::Vector3d(-5, 0, 0),        // invalid radial dist
    Eigen::Vector3d(-DBL_MIN, 0, 0),  // invalid edge radial distance....
    Eigen::Vector3d(0, 0, 0),         // valid boundary radial dist
    Eigen::Vector3d(DBL_MIN, 0, 0),   // valid open boundary
    Eigen::Vector3d(1, 0, 0),         // valid
    // the following tests values of theta and phi, from 0 to 2pi
    //(pi/2)
    Eigen::Vector3d(-5, (M_PI / 2), 0),
    Eigen::Vector3d(-5, 0, (M_PI / 2)),
    Eigen::Vector3d(-5, (M_PI / 2), (M_PI / 2)),

    Eigen::Vector3d(-DBL_MIN, (M_PI / 2), 0),
    Eigen::Vector3d(-DBL_MIN, 0, (M_PI / 2)),
    Eigen::Vector3d(-DBL_MIN, (M_PI / 2), (M_PI / 2)),

    Eigen::Vector3d(0, (M_PI / 2), 0),
    Eigen::Vector3d(0, 0, (M_PI / 2)),
    Eigen::Vector3d(0, (M_PI / 2), (M_PI / 2)),

    Eigen::Vector3d(DBL_MIN, (M_PI / 2), 0),
    Eigen::Vector3d(DBL_MIN, 0, (M_PI / 2)),
    Eigen::Vector3d(DBL_MIN, (M_PI / 2), (M_PI / 2)),

    Eigen::Vector3d(1, (M_PI / 2), 0),
    Eigen::Vector3d(1, 0, (M_PI / 2)),
    Eigen::Vector3d(1, (M_PI / 2), (M_PI / 2)),

    //(pi)
    Eigen::Vector3d(-5, (M_PI), 0),
    Eigen::Vector3d(-5, 0, (M_PI)),
    Eigen::Vector3d(-5, (M_PI), (M_PI)),

    Eigen::Vector3d(-DBL_MIN, (M_PI), 0),
    Eigen::Vector3d(-DBL_MIN, 0, (M_PI)),
    Eigen::Vector3d(-DBL_MIN, (M_PI), (M_PI)),

    Eigen::Vector3d(0, (M_PI), 0),
    Eigen::Vector3d(0, 0, (M_PI)),
    Eigen::Vector3d(0, (M_PI), (M_PI)),

    Eigen::Vector3d(DBL_MIN, (M_PI), 0),
    Eigen::Vector3d(DBL_MIN, 0, (M_PI)),
    Eigen::Vector3d(DBL_MIN, (M_PI), (M_PI)),

    Eigen::Vector3d(1, M_PI, 0),
    Eigen::Vector3d(1, 0, M_PI),
    Eigen::Vector3d(1, M_PI, M_PI),

    //(3pi/2)
    Eigen::Vector3d(-5, ((3 * M_PI) / 2), 0),
    Eigen::Vector3d(-5, 0, ((3 * M_PI) / 2)),
    Eigen::Vector3d(-5, ((3 * M_PI) / 2), ((3 * M_PI) / 2)),

    Eigen::Vector3d(-DBL_MIN, ((3 * M_PI) / 2), 0),
    Eigen::Vector3d(-DBL_MIN, 0, ((3 * M_PI) / 2)),
    Eigen::Vector3d(-DBL_MIN, ((3 * M_PI) / 2), ((3 * M_PI) / 2)),

    Eigen::Vector3d(0, ((3 * M_PI) / 2), 0),
    Eigen::Vector3d(0, 0, ((3 * M_PI) / 2)),
    Eigen::Vector3d(0, ((3 * M_PI) / 2), ((3 * M_PI) / 2)),

    Eigen::Vector3d(DBL_MIN, ((3 * M_PI) / 2), 0),
    Eigen::Vector3d(DBL_MIN, 0, ((3 * M_PI) / 2)),
    Eigen::Vector3d(DBL_MIN, ((3 * M_PI) / 2), ((3 * M_PI) / 2)),

    Eigen::Vector3d(1, ((3 * M_PI) / 2), 0),
    Eigen::Vector3d(1, 0, ((3 * M_PI) / 2)),
    Eigen::Vector3d(1, ((3 * M_PI) / 2), ((3 * M_PI) / 2)),
    //(2pi)
    Eigen::Vector3d(-5, (2 * M_PI), 0),
    Eigen::Vector3d(-5, 0, (2 * M_PI)),
    Eigen::Vector3d(-5, (2 * M_PI), (2 * M_PI)),

    Eigen::Vector3d(-DBL_MIN, (2 * M_PI), 0),
    Eigen::Vector3d(-DBL_MIN, 0, (2 * M_PI)),
    Eigen::Vector3d(-DBL_MIN, (2 * M_PI), (2 * M_PI)),

    Eigen::Vector3d(0, (2 * M_PI), 0),
    Eigen::Vector3d(0, 0, (2 * M_PI)),
    Eigen::Vector3d(0, (2 * M_PI), (2 * M_PI)),

    Eigen::Vector3d(DBL_MIN, (2 * M_PI), 0),
    Eigen::Vector3d(DBL_MIN, 0, (2 * M_PI)),
    Eigen::Vector3d(DBL_MIN, (2 * M_PI), (2 * M_PI)),

    Eigen::Vector3d(1, 2 * M_PI, 0),
    Eigen::Vector3d(1, 0, 2 * M_PI),
    Eigen::Vector3d(1, 2 * M_PI, 2 * M_PI)

    // TODO: add random values between boundaries


};

// pre-calculated spherical to cartesian conversion results
static const std::array<Eigen::Matrix<double, 3, 1>, 65> spherToCart_results = {
    Eigen::Matrix<double, 3, 1>(0, 0, -5),
    Eigen::Matrix<double, 3, 1>(0, 0, -1.175494350822288e-38),
    Eigen::Matrix<double, 3, 1>(0, 0, 0),
    Eigen::Matrix<double, 3, 1>(0, 0, 1.175494350822288e-38),
    Eigen::Matrix<double, 3, 1>(0, 0, 1),
    Eigen::Matrix<double, 3, 1>(0, 0, -5),
    Eigen::Matrix<double, 3, 1>(-5.000000000000000e+00, 0, 2.185569414336896e-07),
    Eigen::Matrix<double, 3, 1>(2.185569414336896e-07, -5.000000000000000e+00, 2.185569414336896e-07),
    Eigen::Matrix<double, 3, 1>(0, 0, -1.175494350822288e-38),
    Eigen::Matrix<double, 3, 1>(-1.175494350822288e-38, 0, 5.138248999765994e-46),
    Eigen::Matrix<double, 3, 1>(5.138248999765994e-46, -1.175494350822288e-38, 5.138248999765994e-46),
    Eigen::Matrix<double, 3, 1>(0, 0, 0),
    Eigen::Matrix<double, 3, 1>(0, 0, 0),
    Eigen::Matrix<double, 3, 1>(0, 0, 0),
    Eigen::Matrix<double, 3, 1>(0, 0, 1.175494350822288e-38),
    Eigen::Matrix<double, 3, 1>(1.175494350822288e-38, 0, -5.138248999765994e-46),
    Eigen::Matrix<double, 3, 1>(-5.138248999765994e-46, 1.175494350822288e-38, -5.138248999765994e-46),
    Eigen::Matrix<double, 3, 1>(0, 0, 1),
    Eigen::Matrix<double, 3, 1>(1.000000000000000e+00, 0, -4.371138828673793e-08),
    Eigen::Matrix<double, 3, 1>(-4.371138828673793e-08, 1.000000000000000e+00, -4.371138828673793e-08),
    Eigen::Matrix<double, 3, 1>(0, 0, -5),
    Eigen::Matrix<double, 3, 1>(4.371138828673793e-07, 0, 5.000000000000000e+00),
    Eigen::Matrix<double, 3, 1>(-4.371138828673793e-07, -3.821370931907940e-14, 5.000000000000000e+00),
    Eigen::Matrix<double, 3, 1>(0, 0, -1.175494350822288e-38),
    Eigen::Matrix<double, 3, 1>(1.027649799953199e-45, 0, 1.175494350822288e-38),
    Eigen::Matrix<double, 3, 1>(-1.027649799953199e-45, -8.983999885708567e-53, 1.175494350822288e-38),
    Eigen::Matrix<double, 3, 1>(0, 0, 0),
    Eigen::Matrix<double, 3, 1>(0, 0, 0),
    Eigen::Matrix<double, 3, 1>(0, 0, 0),
    Eigen::Matrix<double, 3, 1>(0, 0, 1.175494350822288e-38),
    Eigen::Matrix<double, 3, 1>(-1.027649799953199e-45, 0, -1.175494350822288e-38),
    Eigen::Matrix<double, 3, 1>(1.027649799953199e-45, 8.983999885708567e-53, -1.175494350822288e-38),
    Eigen::Matrix<double, 3, 1>(0, 0, 1),
    Eigen::Matrix<double, 3, 1>(-8.742277657347586e-08, 0, -1.000000000000000e+00),
    Eigen::Matrix<double, 3, 1>(8.742277657347586e-08, 7.642741863815879e-15, -1.000000000000000e+00),
    Eigen::Matrix<double, 3, 1>(0, 0, -5),
    Eigen::Matrix<double, 3, 1>(5.000000000000000e+00, 0, -5.962440319251527e-08),
    Eigen::Matrix<double, 3, 1>(5.962440319251527e-08, -5.000000000000000e+00, -5.962440319251527e-08),
    Eigen::Matrix<double, 3, 1>(0, 0, -1.175494350822288e-38),
    Eigen::Matrix<double, 3, 1>(1.175494350822288e-38, 0, -1.401762982479041e-46),
    Eigen::Matrix<double, 3, 1>(1.401762982479041e-46, -1.175494350822288e-38, -1.401762982479041e-46),
    Eigen::Matrix<double, 3, 1>(0, 0, 0),
    Eigen::Matrix<double, 3, 1>(0, 0, 0),
    Eigen::Matrix<double, 3, 1>(0, 0, 0),
    Eigen::Matrix<double, 3, 1>(0, 0, 1.175494350822288e-38),
    Eigen::Matrix<double, 3, 1>(-1.175494350822288e-38, 0, 1.401762982479041e-46),
    Eigen::Matrix<double, 3, 1>(-1.401762982479041e-46, 1.175494350822288e-38, 1.401762982479041e-46),
    Eigen::Matrix<double, 3, 1>(0, 0, 1),
    Eigen::Matrix<double, 3, 1>(-1.000000000000000e+00, 0, 1.192488063850305e-08),
    Eigen::Matrix<double, 3, 1>(-1.192488063850305e-08, 1.000000000000000e+00, 1.192488063850305e-08),
    Eigen::Matrix<double, 3, 1>(0, 0, -5),
    Eigen::Matrix<double, 3, 1>(-8.742277657347586e-07, 0, -5.000000000000000e+00),
    Eigen::Matrix<double, 3, 1>(-8.742277657347586e-07, -1.528548372763176e-13, -5.000000000000000e+00),
    Eigen::Matrix<double, 3, 1>(0, 0, -1.175494350822288e-38),
    Eigen::Matrix<double, 3, 1>(-2.055299599906398e-45, 0, -1.175494350822288e-38),
    Eigen::Matrix<double, 3, 1>(-2.055299599906398e-45, -3.593599954283427e-52, -1.175494350822288e-38),
    Eigen::Matrix<double, 3, 1>(0, 0, 0),
    Eigen::Matrix<double, 3, 1>(0, 0, 0),
    Eigen::Matrix<double, 3, 1>(0, 0, 0),
    Eigen::Matrix<double, 3, 1>(0, 0, 1.175494350822288e-38),
    Eigen::Matrix<double, 3, 1>(2.055299599906398e-45, 0, 1.175494350822288e-38),
    Eigen::Matrix<double, 3, 1>(2.055299599906398e-45, 3.593599954283427e-52, 1.175494350822288e-38),
    Eigen::Matrix<double, 3, 1>(0, 0, 1),
    Eigen::Matrix<double, 3, 1>(1.748455531469517e-07, 0, 1.000000000000000e+00),
    Eigen::Matrix<double, 3, 1>(1.748455531469517e-07, 3.057096745526352e-14, 1.000000000000000e+00)
    // TODO: add values between edge cases
};

// Test cartesianToSpherical conversion'
TEST_CASE("Test coordinate conversion - Cartesian to spherical.", "[utility][math][coordinates]") {
    //***Table heading***
    INFO("************Calculating Spherical coordinates for the origin****************");
    INFO("----------------------------------------------------------------------------");
    INFO(std::left << std::setw(18) << "Input" << std::setw(18) << "Ext calc result" << std::setw(18) << "Util result"
                   << std::setw(18) << "Difference");
    INFO("----------------------------------------------------------------------------");
    // Loop through test values and compare to results array

    for (size_t i = 0; i < cart_coords.size(); i++) {
        Eigen::Vector3d cart_input   = cart_coords[i];
        Eigen::Vector3d cart_compare = cartToSpher_results[i];
        Eigen::Vector3d spher_result = utility::math::coordinates::cartesianToSpherical(cart_input);
        // NOTE: possibly use epsilon
        Eigen::Vector3d result_diff = (spher_result - cart_compare);

        //***New output - Table format***
        INFO(std::left << std::setw(18) << cart_input.x() << std::setw(18) << cart_compare.x() << std::setw(18)
                       << spher_result.x() << std::setw(18) << result_diff.x() << "\n"
                       << std::left << std::setw(18) << cart_input.y() << std::setw(18) << cart_compare.y()
                       << std::setw(18) << spher_result.y() << std::setw(18) << result_diff.y() << "\n"
                       << std::left << std::setw(18) << cart_input.z() << std::setw(18) << cart_compare.z()
                       << std::setw(18) << spher_result.z() << std::setw(18) << result_diff.z());
        // old output
        // INFO("Input: \n"
        //      << std::left << cart_input << "\nExternally calculated result: \n"
        //      << std::left << cart_compare << "\nUtilities method result: \n"
        //      << std::left << spher_result
        //      << "\nThe difference between the externally calculated result and utilities method result: \n"
        //      << std::left << result_diff);
        INFO("----------------------------------------------------------------------------");
        INFO("Difference should be within the error threshold: " << ERROR_THRESHOLD << ".");
        INFO("Failed test value at index: " << i);
        // TEST purposes only
        // REQUIRE(true);

        REQUIRE(result_diff[0] <= ERROR_THRESHOLD);
        REQUIRE(result_diff[1] <= ERROR_THRESHOLD);
        REQUIRE(result_diff[2] <= ERROR_THRESHOLD);
    }
}

// Test sphericalToCartesian conversion
TEST_CASE("Test coordinate conversion - Spherical to cartesian.", "[utility][math][coordinates]") {
    INFO("************Calculating cartesian coordinates for the origin****************");

    for (size_t i = 0; i < spher_coords.size(); i++) {
        Eigen::Matrix<double, 3, 1> spher_input   = spher_coords[i];  // cast vector3d to matrix
        Eigen::Matrix<double, 3, 1> spher_compare = spherToCart_results[i];
        Eigen::Matrix<double, 3, 1> cart_result   = Eigen::Matrix<double, 3, 1>(0, 0, 0);
        //
        if (spher_input.x() < 0) {
            //
            REQUIRE_THROWS_AS(utility::math::coordinates::sphericalToCartesian(spher_input), std::domain_error);
        }
        else {
            cart_result = utility::math::coordinates::sphericalToCartesian(spher_input);
            // NOTE: possibly use epsilon
            Eigen::Matrix<double, 3, 1> result_diff = (cart_result - spher_compare);
            // NOTE: changed table heading from outside of for loop (as in the above example) to inside,
            // so it doesn't show up when testing for exception.
            //***Table heading***
            INFO("----------------------------------------------------------------------------");
            INFO(std::left << std::setw(18) << "Input" << std::setw(18) << "Ext calc result" << std::setw(18)
                           << "Util result" << std::setw(18) << "Difference");
            INFO("----------------------------------------------------------------------------");
            // New output - Table format
            INFO(std::left << std::setw(18) << spher_input.x() << std::setw(18) << spher_compare.x() << std::setw(18)
                           << cart_result.x() << std::setw(18) << result_diff.x() << "\n"
                           << std::left << std::setw(18) << spher_input.y() << std::setw(18) << spher_compare.y()
                           << std::setw(18) << cart_result.y() << std::setw(18) << result_diff.y() << "\n"
                           << std::left << std::setw(18) << spher_input.z() << std::setw(18) << spher_compare.z()
                           << std::setw(18) << cart_result.z() << std::setw(18) << result_diff.y());
            INFO("----------------------------------------------------------------------------");
            INFO("Difference should be within the error threshold: " << ERROR_THRESHOLD << ".");
            INFO("Failed test value at index: " << i);
            // old output
            // INFO("Input: \n"
            //      << std::left << spher_input << "\nExternally calculated result: \n"
            //      << std::left << spher_compare << "\nUtilities method result: \n"
            //      << std::left << cart_result
            //      << "\nThe difference between the externally calculated result and utilities method result: \n"
            //      << std::left << result_diff << ". \nThis should be within the error threshold: \n"
            //      << std::left << ERROR_THRESHOLD << ".");
            // Exception should be thrown if radial distance is negative
            // NOTE: .x()?
            // TEST for fail - set <= to >
            REQUIRE(result_diff.x() <= ERROR_THRESHOLD);
            REQUIRE(result_diff.y() <= ERROR_THRESHOLD);
            REQUIRE(result_diff.z() <= ERROR_THRESHOLD);
        }
    }
}

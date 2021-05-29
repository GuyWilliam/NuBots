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

#ifndef NUHELPERS_HPP
#define NUHELPERS_HPP

#include <nuclear>
#include <utility>

#include "message/support/nusight/DataPoint.hpp"

namespace utility::nusight {

    namespace {

        using message::support::nusight::DataPoint;

        constexpr float TIMEOUT = 2.5;

        template <typename T>
        struct is_iterable {
        private:
            using yes = std::true_type;
            using no  = std::false_type;

            template <typename U>
            static auto test_begin(int) -> decltype(std::declval<U>().begin(), yes());
            template <typename>
            static no test_begin(...);

            template <typename U>
            static auto test_end(int) -> decltype(std::declval<U>().end(), yes());
            template <typename>
            static no test_end(...);

        public:
            static constexpr bool value = std::is_same<decltype(test_begin<T>(0)), yes>::value
                                          && std::is_same<decltype(test_end<T>(0)), yes>::value;
        };

        inline void buildGraph(DataPoint& /*dataPoint*/) {}

        template <typename First, typename... Remainder>
        typename std::enable_if<!is_iterable<First>::value>::type buildGraph(DataPoint& dataPoint,
                                                                             First first,
                                                                             Remainder... remainder) {
            dataPoint.value.push_back(float(first));
            buildGraph(dataPoint, remainder...);
        }

        template <typename First, typename... Remainder>
        typename std::enable_if<is_iterable<First>::value>::type buildGraph(DataPoint& dataPoint,
                                                                            First first,
                                                                            Remainder... remainder) {
            for (const auto& value : first) {
                dataPoint.value.push_back(float(value));
            }
            buildGraph(dataPoint, remainder...);
        }
    }  // namespace

    template <typename... Values>
    inline std::unique_ptr<message::support::nusight::DataPoint> graph(std::string label, Values... values) {
        auto dataPoint   = std::make_unique<DataPoint>();
        dataPoint->label = std::move(label);
        buildGraph(*dataPoint, values...);
        return dataPoint;
    }

}  // namespace utility::nusight
#endif

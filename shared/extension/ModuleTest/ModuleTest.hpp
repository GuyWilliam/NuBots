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
 * Copyright 2021 NUbots <nubots@nubots.net>
 */

#ifndef EXTENSION_MODULETEST_HPP
#define EXTENSION_MODULETEST_HPP

#include <catch.hpp>
#include <functional>
#include <nuclear>
#include <thread>

#include "EmissionCatcher.hpp"

namespace extension::moduletest {

    template <typename Module>
    class ModuleTest : public NUClear::PowerPlant {
    public:
        // TODO: somehow disable the reactions which aren't on<Trigger<..>>, such as on<Every<..>> or on<Always>
        // ModuleTest() : NUClear::PowerPlant(get_single_thread_config()) {}
        ModuleTest() = default;

        // ModuleTest(ModuleTest& other)  = delete;
        // ModuleTest(ModuleTest&& other) = delete;
        // ModuleTest& operator=(ModuleTest& other) = delete;
        // ModuleTest&& operator=(ModuleTest&& other) = delete;

    private:
        // static const NUClear::PowerPlant::Configuration get_single_thread_config() {
        //     NUClear::PowerPlant::Configuration cfg;
        //     cfg.thread_count = 1;
        //     return cfg;
        // }
    };


}  // namespace extension::moduletest

#endif  // EXTENSION_MODULETEST_HPP

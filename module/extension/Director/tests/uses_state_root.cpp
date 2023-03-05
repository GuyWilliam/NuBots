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

#include <catch.hpp>
#include <nuclear>
#include <string>

#include "Director.hpp"
#include "TestBase.hpp"
#include "util/diff_string.hpp"

// Anonymous namespace to avoid name collisions
namespace {

    struct PrimaryTrigger {
        PrimaryTrigger(const bool& run_) : run(run_) {}
        bool run = false;
    };
    struct SecondaryTask {};
    struct SubTask {};

    std::vector<std::string> events;

    class TestReactor : public TestBase<TestReactor> {
    public:
        /// Print the subtask state
        std::string decode_run_state(GroupInfo::RunState state) {
            switch (state) {
                case GroupInfo::RunState::NO_TASK: return "NO_TASK";
                case GroupInfo::RunState::RUNNING: return "RUNNING";
                case GroupInfo::RunState::QUEUED: return "QUEUED";
                default: return "ERROR";
            }
        }

        explicit TestReactor(std::unique_ptr<NUClear::Environment> environment)
            : TestBase<TestReactor>(std::move(environment)) {


            on<Trigger<PrimaryTrigger>, Uses<SubTask>>().then(
                [this](const PrimaryTrigger& primary, const Uses<SubTask>& subtask) {
                    events.push_back("primary trigger subtask run state: " + decode_run_state(subtask.run_state));

                    if (primary.run) {
                        events.push_back("emitting subtask");
                        emit<Task>(std::make_unique<SubTask>());
                    }
                    else {
                        events.push_back("removing subtask");
                        emit<Task>(std::unique_ptr<SubTask>(nullptr));
                    }
                });

            on<Provide<SecondaryTask>, Uses<SubTask>>().then([this](const Uses<SubTask>& subtask) {
                events.push_back("secondary task subtask run state: " + decode_run_state(subtask.run_state));
                events.push_back("emitting subtask");
                emit<Task>(std::make_unique<SubTask>());
            });

            on<Provide<SubTask>>().then([this] { events.push_back("subtask executed"); });

            /**************
             * TEST STEPS *
             **************/
            // Start the primary trigger, subtask will initially not be running
            on<Trigger<Step<1>>, Priority::LOW>().then([this] {
                events.push_back("emitting primary trigger");
                emit<Scope::DIRECT>(std::make_unique<PrimaryTrigger>(true));
            });

            // Run the primary task again, subtask will now be running
            on<Trigger<Step<2>>, Priority::LOW>().then([this] {
                events.push_back("emitting primary trigger again");
                emit<Scope::DIRECT>(std::make_unique<PrimaryTrigger>(true));
            });

            // Run the secondary task, which has a lower priority than the root primary trigger subtask
            // This should give a queued subtask state on the next run
            on<Trigger<Step<3>>, Priority::LOW>().then([this] {
                events.push_back("emitting secondary task");
                emit<Task>(std::make_unique<SecondaryTask>());
            });

            // Run the secondary task again to get the queued state
            on<Trigger<Step<4>>, Priority::LOW>().then([this] {
                events.push_back("emitting secondary task again");
                emit<Task>(std::make_unique<SecondaryTask>());
            });

            // Remove the primary trigger subtask to detect a running subtask on the secondary task
            on<Trigger<Step<5>>, Priority::LOW>().then([this] {
                events.push_back("removing primary trigger");
                emit<Scope::DIRECT>(std::make_unique<PrimaryTrigger>(false));
            });

            // Run the secondary task again to see the running state
            on<Trigger<Step<6>>, Priority::LOW>().then([this] {
                events.push_back("emitting secondary task again");
                emit<Task>(std::make_unique<SecondaryTask>());
            });

            on<Startup>().then([this] {
                emit(std::make_unique<Step<1>>());
                emit(std::make_unique<Step<2>>());
                emit(std::make_unique<Step<3>>());
                emit(std::make_unique<Step<4>>());
                emit(std::make_unique<Step<5>>());
                emit(std::make_unique<Step<6>>());
            });
        }

        bool executed = false;
    };
}  // namespace

TEST_CASE("Test that the Uses run state information is correct with root tasks", "[director][uses][state][root]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant powerplant(config);
    powerplant.install<module::extension::Director>();
    powerplant.install<TestReactor>();
    powerplant.start();

    std::vector<std::string> expected = {
        "emitting primary trigger",
        "primary trigger subtask run state: NO_TASK",
        "emitting subtask",
        "subtask executed",
        "emitting primary trigger again",
        "primary trigger subtask run state: RUNNING",
        "emitting subtask",
        "subtask executed",
        "emitting secondary task",
        "secondary task subtask run state: NO_TASK",
        "emitting subtask",
        "emitting secondary task again",
        "secondary task subtask run state: QUEUED",
        "emitting subtask",
        "removing primary trigger",
        "primary trigger subtask run state: RUNNING",
        "removing subtask",
        "subtask executed",
        "emitting secondary task again",
        "secondary task subtask run state: RUNNING",
        "emitting subtask",
        "subtask executed",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(util::diff_string(expected, events));

    // Check the events fired in order and only those events
    REQUIRE(events == expected);
}

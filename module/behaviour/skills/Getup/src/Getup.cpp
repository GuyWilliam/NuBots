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

#include "Getup.hpp"

#include <Eigen/Core>
#include <cmath>

#include "extension/Configuration.hpp"
#include "extension/Script.hpp"

#include "message/behaviour/ServoCommand.hpp"
#include "message/motion/GetupCommand.hpp"
#include "message/platform/RawSensors.hpp"

#include "utility/behaviour/Action.hpp"
#include "utility/input/LimbID.hpp"

namespace module::behaviour::skills {

    using extension::Configuration;
    using extension::ExecuteScriptByName;

    using message::motion::ExecuteGetup;
    using message::motion::KillGetup;
    using message::platform::RawSensors;

    using utility::behaviour::ActionPriorities;
    using utility::behaviour::RegisterAction;
    using LimbID  = utility::input::LimbID;
    using ServoID = utility::input::ServoID;

    Getup::Getup(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment))
        , id(size_t(this) * size_t(this) - size_t(this))
        , isFront(true)
        , gettingUp(false)
        , fallenCheck()
        , GETUP_PRIORITY(0.0f)
        , EXECUTION_PRIORITY(0.0f) {
        // do a little configurating
        on<Configuration>("Getup.yaml").then([this](const Configuration& file) {
            // encode fallen angle as a cosine so we can compare it directly to the z
            // axis value
            double fallenAngleConfig = file["FALLEN_ANGLE"].as<double>();
            FALLEN_ANGLE             = cos(fallenAngleConfig);

            // load priorities for the getup
            GETUP_PRIORITY     = file["GETUP_PRIORITY"].as<float>();
            EXECUTION_PRIORITY = file["EXECUTION_PRIORITY"].as<float>();
        });

        fallenCheck = on<Last<20, Trigger<RawSensors>>, Single>().then(
            "Getup Fallen Check",
            [this](const std::list<std::shared_ptr<const RawSensors>>& sensors) {
                Eigen::Vector3d acc_reading = Eigen::Vector3d::Zero();

                for (const auto& s : sensors) {
                    acc_reading += Eigen::Vector3d(s->accelerometer.x, s->accelerometer.y, s->accelerometer.z);
                }
                acc_reading.normalize();

                // check that the accelerometer reading is less than some predetermined
                // amount
                if (!gettingUp && std::acos(EigenVector3d::UnitZ().dot(acc_reading)) > FALLEN_ANGLE) {
                    isFront = false;
                    if (M_PI / 2 - std::acos(EigenVector3d::UnitX().dot(acc_reading)) > 0.0) {
                        isFront = true;
                    }

                    updatePriority(GETUP_PRIORITY);
                    fallenCheck.disable();
                }
            });

        on<Trigger<ExecuteGetup>>().then("Execute Getup", [this]() {
            gettingUp = true;

            // Check with side we're getting up from
            if (isFront) {
                emit(std::make_unique<ExecuteScriptByName>(
                    id,
                    std::vector<std::string>({"RollOverFront.yaml", "StandUpBack.yaml", "Stand.yaml"})));
            }
            else {
                emit(std::make_unique<ExecuteScriptByName>(
                    id,
                    std::vector<std::string>({"StandUpBack.yaml", "Stand.yaml"})));
            }
            updatePriority(EXECUTION_PRIORITY);
        });

        on<Trigger<KillGetup>>().then([this] {
            gettingUp = false;
            updatePriority(0);
            fallenCheck.enable();
        });

        emit<Scope::INITIALIZE>(std::make_unique<RegisterAction>(RegisterAction{
            id,
            "Get Up",
            {std::pair<float, std::set<LimbID>>(
                0,
                {LimbID::LEFT_LEG, LimbID::RIGHT_LEG, LimbID::LEFT_ARM, LimbID::RIGHT_ARM, LimbID::HEAD})},
            [this](const std::set<LimbID>&) { emit(std::make_unique<ExecuteGetup>()); },
            [this](const std::set<LimbID>&) { emit(std::make_unique<KillGetup>()); },
            [this](const std::set<ServoID>& servoSet) {
                // HACK 2014 Jake Fountain, Trent Houliston
                // TODO track set limbs and wait for all to finish
                log("Checking ankles: ",
                    servoSet.find(ServoID::L_ANKLE_PITCH) != servoSet.end(),
                    servoSet.find(ServoID::R_ANKLE_PITCH) != servoSet.end());
                if (servoSet.find(ServoID::L_ANKLE_PITCH) != servoSet.end()
                    || servoSet.find(ServoID::R_ANKLE_PITCH) != servoSet.end()) {
                    emit(std::make_unique<KillGetup>());
                }
            }}));
    }

    void Getup::updatePriority(const float& priority) {
        emit(std::make_unique<ActionPriorities>(ActionPriorities{id, {priority}}));
    }

}  // namespace module::behaviour::skills

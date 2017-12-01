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

#ifndef MODULES_SUPPORT_NUBUGGER_H
#define MODULES_SUPPORT_NUBUGGER_H

#include <fstream>
#include <nuclear>

#include "extension/Configuration.h"

#include "message/behaviour/Subsumption.h"
#include "message/input/CameraParameters.h"
#include "message/input/GameEvents.h"
#include "message/input/GameState.h"

namespace module {
namespace support {

    /**
     * Intelligent debugging, logging and graphing for the NUbots system.
     *
     * @author Brendan Annable
     * @author Trent Houliston
     */
    class NUbugger : public NUClear::Reactor {
    private:
        NUClear::clock::duration max_image_duration;
        NUClear::clock::time_point last_image             = NUClear::clock::now();
        NUClear::clock::time_point last_reprojected_image = NUClear::clock::now();
        NUClear::clock::time_point last_baked_image       = NUClear::clock::now();
        NUClear::clock::duration max_reprojected_image_duration;
        NUClear::clock::duration max_baked_image_duration;
        NUClear::clock::duration max_classified_image_duration;
        NUClear::clock::time_point last_classified_image = NUClear::clock::now();

        bool listening = true;

        // Reaction Handles
        std::map<std::string, std::vector<ReactionHandle>> handles;

        std::map<uint, message::behaviour::Subsumption::ActionRegister> actionRegisters;

        NUClear::clock::time_point last_camera_image = NUClear::clock::now();
        NUClear::clock::time_point last_seen_ball    = NUClear::clock::now();
        NUClear::clock::time_point last_seen_goal    = NUClear::clock::now();

        void provideOverview();
        void provideDataPoints();
        void provideDrawObjects();
        void provideSubsumption();
        void provideGameController();
        void provideLocalisation();
        void provideReactionStatistics();
        void provideSensors();
        void provideVision();

        void sendReactionHandles();

        void sendGameState(std::string event, std::shared_ptr<const message::input::GameState> gameState);

        void saveConfigurationFile(std::string path, const std::string& root);
        void sendSubsumption();

    public:
        static constexpr const char* IGNORE_TAG = "IGNORE";
        explicit NUbugger(std::unique_ptr<NUClear::Environment> environment);
    };

}  // namespace support
}  // namespace module

#endif  // MODULES_SUPPORT_NUBUGGER_H

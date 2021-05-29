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

#ifndef MODULES_BEHAVIOUR_CONTROLLER_HPP
#define MODULES_BEHAVIOUR_CONTROLLER_HPP

#include <array>
#include <list>
#include <map>
#include <nuclear>
#include <set>
#include <vector>

#include "message/behaviour/ServoCommand.hpp"
#include "message/input/Sensors.hpp"

#include "utility/behaviour/Action.hpp"
#include "utility/input/ServoID.hpp"

namespace module::behaviour {

    struct RequestItem;

    struct Request {
        using callback = std::function<void(std::set<utility::input::LimbID>)>;

        Request()
            : id(0)
            , name("")
            , active(false)
            , maxPriority(std::numeric_limits<float>::min())
            , mainElement(0)
            , items()
            , start()
            , kill()
            , completed() {}

        Request(size_t id_,
                std::string name_,
                callback start_,
                callback kill_,
                std::function<void(std::set<utility::input::ServoID>)> completed_)
            : id(id_)
            , name(name_)
            , active(false)
            , maxPriority(std::numeric_limits<float>::min())
            , mainElement(0)
            , items()
            , start(start_)
            , kill(kill_)
            , completed(completed_) {}

        /// The ID of this request that will be sent with any motion commands
        size_t id;

        /// The name of the requester
        std::string name;

        /// If the main element of this request is active
        bool active;

        /// The maximum priority for any of the items
        float maxPriority;

        /// The index of the main item
        size_t mainElement;

        /// The items in this list
        std::vector<RequestItem> items;

        /// The callback to execute when a new limb is started
        callback start;
        callback kill;
        std::function<void(std::set<utility::input::ServoID>)> completed;
    };

    struct RequestItem {

        // RequestItem() : group(Request()), index(0), active(false), priority(std::numeric_limits<float>::min()),
        // limbSet() {}
        RequestItem(Request& group_, size_t index_, float priority_, const std::set<utility::input::LimbID>& limbSet_)
            : group(group_), index(index_), active(false), priority(priority_), limbSet(limbSet_) {}

        Request& group;

        size_t index;

        bool active;

        float priority;
        std::set<utility::input::LimbID> limbSet;
    };

    /**
     * Controls which of the behaviours are able to access motors.
     *
     * @author Trent Houliston
     */
    class Controller : public NUClear::Reactor {
    private:
        std::array<std::vector<std::reference_wrapper<RequestItem>>, 5> actions;
        std::array<size_t, 5> limbAccess;
        std::map<size_t, std::unique_ptr<Request>> requests;
        std::vector<std::reference_wrapper<RequestItem>> currentActions;


        std::array<std::list<message::behaviour::ServoCommand>, 20> commandQueues;

        void selectAction();

    public:
        explicit Controller(std::unique_ptr<NUClear::Environment> environment);
    };

}  // namespace module::behaviour

#endif  // MODULES_BEHAVIOUR_CONTROLLER_HPP

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

#ifndef MODULES_BEHAVIOUR_PLANNERS_SIMPLEWALKPATHPLANNER_HPP
#define MODULES_BEHAVIOUR_PLANNERS_SIMPLEWALKPATHPLANNER_HPP

#include <Eigen/Core>
#include <cmath>
#include <nuclear>

#include "extension/Configuration.hpp"

#include "message/behaviour/KickPlan.hpp"
#include "message/behaviour/MotionCommand.hpp"
#include "message/input/Sensors.hpp"
#include "message/localisation/Ball.hpp"
#include "message/localisation/Field.hpp"
#include "message/motion/KickCommand.hpp"
#include "message/motion/WalkCommand.hpp"
#include "message/support/FieldDescription.hpp"
#include "message/vision/Ball.hpp"

namespace module::behaviour::planning {

    // using namespace message;

    using message::behaviour::KickPlan;
    using message::behaviour::MotionCommand;
    using message::behaviour::WantsToKick;
    using message::input::Sensors;
    using message::localisation::Ball;
    using message::localisation::Field;
    using message::support::FieldDescription;

    /**
     * Executes a getup script if the robot falls over.
     *
     * @author Josiah Walker
     */
    class SimpleWalkPathPlanner : public NUClear::Reactor {
    private:
        message::behaviour::MotionCommand latestCommand;
        const size_t subsumptionId;
        float max_turn_speed         = 0.2;
        float min_turn_speed         = 0.2;
        float forward_speed          = 1;
        float side_speed             = 1;
        float rotate_speed_x         = -0.04;
        float rotate_speed_y         = 0;
        float rotate_speed           = 0.2;
        float walk_to_ready_speed_x  = 0.1;
        float walk_to_ready_speed_y  = 0.1;
        float walk_to_ready_rotation = 0.5;
        float slow_approach_factor   = 0.5;
        float a                      = 7;
        float b                      = 0;
        float search_timeout         = 3;

        //----------- non-config variables (not defined in WalkPathPlanner.yaml)----

        // info for the current walk
        Eigen::Vector2d current_target_position;
        Eigen::Vector2d current_target_heading;
        message::behaviour::KickPlan target_heading;
        Eigen::Vector2d target_position = Eigen::Vector2d::Zero();

        NUClear::clock::time_point time_ball_last_seen;
        Eigen::Vector3d rBWw     = Eigen::Vector3d(10.0, 0.0, 0.0);
        bool robot_ground_space  = true;
        Eigen::Vector2d position = Eigen::Vector2d::UnitX();  // ball pos rel to robot
        float ball_approach_dist = 0.2;
        float slowdown_distance  = 0.2;
        bool use_localisation    = true;
        Eigen::Vector3f rBTt     = Eigen::Vector3f(1.0, 0.0, 0.0);

        void walk_directly();

        void determine_simple_walk_path(const Ball& ball,
                                        const Field& field,
                                        const Sensors& sensors,
                                        const KickPlan& kickPlan,
                                        const FieldDescription& field_description);

        void vision_walk_path();
        void rotate_on_spot();
        void walk_to_ready();

    public:
        explicit SimpleWalkPathPlanner(std::unique_ptr<NUClear::Environment> environment);
    };
}  // namespace module::behaviour::planning

#endif  // MODULES_BEHAVIOUR_PLANNERS_SIMPLEWALKPATHPLANNER_HPP

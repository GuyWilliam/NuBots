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

#ifndef MODULES_BEHAVIOUR_REFLEX_HEADCONTROLLER_HPP
#define MODULES_BEHAVIOUR_REFLEX_HEADCONTROLLER_HPP

#include <Eigen/Core>
#include <nuclear>

namespace module::motion {

    /**
     * Executes a HeadController action.
     *
     * @author Jade Fountain
     */
    class HeadController : public NUClear::Reactor {
    private:
        const size_t id;
        float min_yaw, max_yaw, min_pitch, max_pitch;
        double head_motor_gain, head_motor_torque, smoothing_factor;
        ReactionHandle updateHandle;
        // Debug var:
        NUClear::clock::time_point lastTime;

    public:
        explicit HeadController(std::unique_ptr<NUClear::Environment> environment);

        Eigen::Vector2f currentAngles;
        Eigen::Vector2f goalAngles;
        bool goalRobotSpace = true;
        bool smooth         = true;
    };
}  // namespace module::motion

#endif  // MODULES_BEHAVIOURS_REFLEX_HEADCONTROLLER_HPP

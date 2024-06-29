/*
 * MIT License
 *
 * Copyright (c) 2023 NUbots
 *
 * This file is part of the NUbots codebase.
 * See https://github.com/NUbots/NUbots for further info.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "PlanWalkPath.hpp"

#include "extension/Behaviour.hpp"
#include "extension/Configuration.hpp"

#include "message/input/Sensors.hpp"
#include "message/localisation/Robot.hpp"
#include "message/planning/WalkPath.hpp"
#include "message/skill/Walk.hpp"
#include "message/strategy/StandStill.hpp"

#include "utility/math/comparison.hpp"
#include "utility/math/euler.hpp"
#include "utility/math/geometry/intersection.hpp"
#include "utility/nusight/NUhelpers.hpp"
#include "utility/support/yaml_expression.hpp"

namespace module::planning {

    using extension::Configuration;

    using message::input::Sensors;
    using message::localisation::Robots;
    using message::planning::PivotAroundPoint;
    using message::planning::TurnOnSpot;
    using message::planning::WalkTo;
    using message::planning::WalkToDebug;
    using message::skill::Walk;

    using message::strategy::StandStill;

    using utility::math::euler::rpy_intrinsic_to_mat;
    using utility::math::geometry::intersection_line_and_circle;
    using utility::nusight::graph;
    using utility::support::Expression;

    PlanWalkPath::PlanWalkPath(std::unique_ptr<NUClear::Environment> environment)
        : BehaviourReactor(std::move(environment)) {

        on<Configuration>("PlanWalkPath.yaml").then([this](const Configuration& config) {
            // Use configuration here from file PlanWalkPath.yaml
            this->log_level = config["log_level"].as<NUClear::LogLevel>();

            // WalkTo tuning
            cfg.max_translational_velocity_x = config["max_translational_velocity_x"].as<double>();
            cfg.max_translational_velocity_y = config["max_translational_velocity_y"].as<double>();
            max_velocity_magnitude   = std::max(cfg.max_translational_velocity_x, cfg.max_translational_velocity_y);
            cfg.max_angular_velocity = config["max_angular_velocity"].as<double>();
            cfg.acceleration         = config["acceleration"].as<double>();

            cfg.max_align_radius = config["max_align_radius"].as<double>();
            cfg.min_align_radius = config["min_align_radius"].as<double>();
            cfg.max_angle_error  = config["max_angle_error"].as<Expression>();
            cfg.min_angle_error  = config["min_angle_error"].as<Expression>();
            cfg.strafe_gain      = config["strafe_gain"].as<double>();

            // TurnOnSpot tuning
            cfg.rotate_velocity   = config["rotate_velocity"].as<double>();
            cfg.rotate_velocity_x = config["rotate_velocity_x"].as<double>();
            cfg.rotate_velocity_y = config["rotate_velocity_y"].as<double>();

            // PivotAroundPoint tuning
            cfg.pivot_ball_velocity   = config["pivot_ball_velocity"].as<double>();
            cfg.pivot_ball_velocity_x = config["pivot_ball_velocity_x"].as<double>();
            cfg.pivot_ball_velocity_y = config["pivot_ball_velocity_y"].as<double>();

            cfg.obstacle_radius = config["obstacle_radius"].as<double>();
        });

        on<Start<WalkTo>>().then([this] {
            log<NUClear::DEBUG>("Starting walk to task");

            // Reset the velocity magnitude to zero
            velocity_magnitude = 0;
        });

        on<Provide<WalkTo>, Optional<With<Robots>>, With<Sensors>>().then(
            [this](const WalkTo& walk_to, const std::shared_ptr<const Robots>& robots, const Sensors& sensors) {
                // Decompose the target pose into position and orientation
                Eigen::Vector2d rDRr = walk_to.Hrd.translation().head(2);
                log<NUClear::DEBUG>("Plan path");
                std::vector<Eigen::Vector2d> obstacles{};
                if (robots != nullptr) {
                    for (const auto& robot : robots->robots) {
                        Eigen::Vector2d rORr = (sensors.Hrw * robot.rRWw).head(2);

                        // Check if the obstacle intersects our path
                        // If the obstacle is close to us do not attempt to avoid it
                        if (rORr.norm() > cfg.obstacle_radius
                            && intersection_line_and_circle(Eigen::Vector2d::Zero(), rDRr, rORr, cfg.obstacle_radius)) {
                            log<NUClear::DEBUG>("1 Obstacle detected");
                            obstacles.push_back(rORr);
                        }
                    }
                }

                while (!obstacles.empty()) {
                    // Calculate direction vector from robot to target
                    Eigen::Vector2d direction = rDRr - Eigen::Vector2d::Zero();  // Assuming robot's position is (0,0)
                    // Calculate a perpendicular vector to the direction
                    Eigen::Vector2d perp_direction(-direction.y(), direction.x());
                    perp_direction.normalize();
                    // Scale the perpendicular vector by obstacle_radius to ensure clearance
                    Eigen::Vector2d avoid_vector = perp_direction * cfg.obstacle_radius * 2;
                    // Adjust rDRr to navigate around the obstacle
                    rDRr = obstacles.front() + avoid_vector;

                    log<NUClear::DEBUG>("Avoiding obstacle");

                    // Clear obstacles and reevaluate
                    obstacles.clear();
                    if (robots != nullptr) {
                        for (const auto& robot : robots->robots) {
                            Eigen::Vector2d rORr = (sensors.Hrw * robot.rRWw).head(2);
                            if (rORr.norm() > cfg.obstacle_radius
                                && intersection_line_and_circle(Eigen::Vector2d::Zero(),
                                                                rDRr,
                                                                rORr,
                                                                cfg.obstacle_radius)) {
                                log<NUClear::DEBUG>("2 Obstacle detected", rORr.transpose(), rDRr.transpose());

                                obstacles.push_back(rORr);
                            }
                        }
                    }
                }

                // Calculate the translational error between the robot and the target point (x, y)
                double translational_error = rDRr.norm();

                // Calculate the angle between the robot and the target point (x, y)
                double angle_to_target = std::atan2(rDRr.y(), rDRr.x());

                // Calculate the angle between the robot and the final desired heading
                double angle_to_final_heading =
                    std::atan2(walk_to.Hrd.linear().col(0).y(), walk_to.Hrd.linear().col(0).x());

                // Linearly interpolate between angle to the target and desired heading when inside the align radius
                // region
                double translation_progress = std::clamp(
                    (cfg.max_align_radius - translational_error) / (cfg.max_align_radius - cfg.min_align_radius),
                    0.0,
                    1.0);
                double desired_heading =
                    (1 - translation_progress) * angle_to_target + translation_progress * angle_to_final_heading;

                double desired_velocity_magnitude = 0;
                if (translational_error > cfg.max_align_radius) {
                    // "Accelerate"
                    velocity_magnitude += cfg.acceleration;
                    // Limit the velocity magnitude to the maximum velocity
                    velocity_magnitude = std::min(velocity_magnitude, max_velocity_magnitude);
                    // Scale the velocity by angle error to have robot rotate on spot when far away and not facing
                    // target [0 at max_angle_error, linearly interpolate between, 1 at min_angle_error]
                    double angle_error_gain = std::clamp(
                        (cfg.max_angle_error - std::abs(desired_heading)) / (cfg.max_angle_error - cfg.min_angle_error),
                        0.0,
                        1.0);
                    desired_velocity_magnitude = angle_error_gain * velocity_magnitude;
                }
                else {
                    // "Decelerate"
                    velocity_magnitude -= cfg.acceleration;
                    // Limit the velocity to zero
                    velocity_magnitude = std::max(velocity_magnitude, 0.0);
                    // Normalise error between [0, 1] inside align radius
                    double error = translational_error / cfg.max_align_radius;
                    // "Proportional control" to strafe towards the target inside align radius
                    desired_velocity_magnitude = cfg.strafe_gain * error;
                }

                // Calculate the target velocity
                Eigen::Vector2d desired_translational_velocity = desired_velocity_magnitude * rDRr.normalized();
                Eigen::Vector3d velocity_target;
                velocity_target << desired_translational_velocity, desired_heading;

                // Limit the velocity to the maximum translational and angular velocity
                velocity_target = constrain_velocity(velocity_target,
                                                     cfg.max_translational_velocity_x,
                                                     cfg.max_translational_velocity_y,
                                                     cfg.max_angular_velocity);

                // Emit the walk task with the calculated velocities
                emit<Task>(std::make_unique<Walk>(velocity_target));

                // Emit debugging information for visualization and monitoring
                auto debug_information              = std::make_unique<WalkToDebug>();
                Eigen::Isometry3d Hrd               = Eigen::Isometry3d::Identity();
                Hrd.translation().head(2)           = rDRr;
                Hrd.linear()                        = rpy_intrinsic_to_mat(Eigen::Vector3d(0, 0, desired_heading));
                debug_information->Hrd              = Hrd;
                debug_information->min_align_radius = cfg.min_align_radius;
                debug_information->max_align_radius = cfg.max_align_radius;
                debug_information->min_angle_error  = cfg.min_angle_error;
                debug_information->max_angle_error  = cfg.max_angle_error;
                debug_information->angle_to_target  = angle_to_target;
                debug_information->angle_to_final_heading = angle_to_final_heading;
                debug_information->translational_error    = translational_error;
                debug_information->velocity_target        = velocity_target;
                emit(debug_information);
            });

        on<Provide<TurnOnSpot>>().then([this](const TurnOnSpot& turn_on_spot) {
            // Determine the direction of rotation
            int sign = turn_on_spot.clockwise ? -1 : 1;

            // Turn on the spot
            emit<Task>(std::make_unique<Walk>(
                Eigen::Vector3d(cfg.rotate_velocity_x, cfg.rotate_velocity_y, sign * cfg.rotate_velocity)));
        });

        on<Provide<PivotAroundPoint>>().then([this](const PivotAroundPoint& pivot_around_point) {
            // Determine the direction of rotation
            int sign = pivot_around_point.clockwise ? -1 : 1;
            // Turn around the ball
            emit<Task>(std::make_unique<Walk>(Eigen::Vector3d(cfg.pivot_ball_velocity_x,
                                                              sign * cfg.pivot_ball_velocity_y,
                                                              sign * cfg.pivot_ball_velocity)));
        });
    }
}  // namespace module::planning

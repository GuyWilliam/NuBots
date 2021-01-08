/*
 * This file is part of NUbots Codebase.
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
 * Copyright 2013 NUBots <nubots@nubots.net>
 */

#include "KickPlanner.hpp"

#include "extension/Configuration.hpp"

#include "message/behaviour/KickPlan.hpp"
#include "message/behaviour/ServoCommand.hpp"
#include "message/localisation/Ball.hpp"
#include "message/localisation/Field.hpp"
#include "message/motion/KinematicsModel.hpp"
#include "message/motion/WalkCommand.hpp"
#include "message/support/FieldDescription.hpp"
#include "message/vision/Ball.hpp"

#include "utility/behaviour/Action.hpp"
#include "utility/input/LimbID.hpp"
#include "utility/localisation/transform.hpp"
#include "utility/math/coordinates.hpp"
#include "utility/math/matrix/Transform3D.hpp"
#include "utility/motion/InverseKinematics.hpp"
#include "utility/nusight/NUhelpers.hpp"
#include "utility/support/eigen_armadillo.hpp"
#include "utility/support/yaml_armadillo.hpp"


namespace module {
namespace behaviour {
    namespace planning {

        using extension::Configuration;

        using message::behaviour::KickPlan;
        using KickType = message::behaviour::KickPlan::KickType;
        using message::behaviour::WantsToKick;
        using message::input::GameState;
        using message::input::Sensors;
        using message::localisation::Ball;
        using message::localisation::Field;
        using Phase         = message::input::GameState::Data::Phase;
        using PenaltyReason = message::input::GameState::Data::PenaltyReason;
        using message::input::Sensors;
        using VisionBall  = message::vision::Ball;
        using VisionBalls = message::vision::Balls;
        using message::motion::IKKickParams;
        using message::motion::KickCommand;
        using message::platform::darwin::ButtonMiddleDown;
        using KickCommandType = message::motion::KickCommandType;
        using message::motion::KickPlannerConfig;
        using message::motion::KickScriptCommand;
        using message::motion::KinematicsModel;
        using message::support::FieldDescription;

        using LimbID = utility::input::LimbID;
        using utility::localisation::fieldStateToTransform3D;
        using utility::math::coordinates::sphericalToCartesian;
        using utility::math::matrix::Rotation2D;
        using utility::math::matrix::Transform3D;
        using utility::motion::kinematics::legPoseValid;
        using utility::nusight::graph;

        KickPlanner::KickPlanner(std::unique_ptr<NUClear::Environment> environment)
            : Reactor(std::move(environment))
            , cfg()
            , ballLastSeen(std::chrono::seconds(0))
            , lastTimeValid(NUClear::clock::now()) {


            on<Configuration>("KickPlanner.yaml").then([this](const Configuration& config) {
                cfg.max_ball_distance        = config["max_ball_distance"].as<float>();
                cfg.kick_corridor_width      = config["kick_corridor_width"].as<float>();
                cfg.seconds_not_seen_limit   = config["seconds_not_seen_limit"].as<float>();
                cfg.kick_forward_angle_limit = config["kick_forward_angle_limit"].as<float>();
                emit(std::make_unique<KickPlannerConfig>(cfg));
                emit(std::make_unique<WantsToKick>(false));
            });

            on<Trigger<VisionBalls>>().then([this](const VisionBalls& balls) {
                if (balls.balls.size() > 0) {
                    ballLastSeen = NUClear::clock::now();
                }
            });

            on<Trigger<ButtonMiddleDown>, Single>().then([this] { forcePlaying = true; });

            on<Trigger<Ball>,
               With<Field>,
               With<FieldDescription>,
               With<KickPlan>,
               With<Sensors>,
               Optional<With<GameState>>>()
                .then([this](const Ball& ball,
                             const Field& field,
                             const FieldDescription& fd,
                             const KickPlan& kickPlan,
                             const Sensors& sensors,
                             std::shared_ptr<const GameState> gameState) {
                    // Get time since last seen ball
                    auto now = NUClear::clock::now();
                    double secondsSinceLastSeen =
                        std::chrono::duration_cast<std::chrono::microseconds>(now - ballLastSeen).count() * 1e-6;

                    // Compute target in robot coords
                    // arma::vec2 kickTarget = {1,0,0}; //Kick forwards
                    // TODO: The heading seems to judder here!!
                    // TODO: use sensors.Htw instead
                    Transform3D Hfw = fieldStateToTransform3D(convert(field.position));

                    Transform3D Htw         = convert(sensors.Htw);
                    arma::vec3 ballPosition = Htw.transformPoint({ball.position[0], ball.position[1], fd.ball_radius});

                    // Transform target from field to torso space
                    Transform3D Htf       = (Htw * Hfw.i());
                    arma::vec3 kickTarget = Htf.transformPoint(arma::vec3({kickPlan.target[0], kickPlan.target[1], 0}));
                    float KickAngle       = std::fabs(std::atan2(kickTarget[1], kickTarget[0]));

                    // log("KickPlan target global",convert(kickPlan.target).t());
                    // log("Target of Kick",kickTarget.t());
                    // log("KickAngle",KickAngle);

                    // Check whether to kick
                    // log("kickTarget",kickTarget.t());
                    // log("KickAngle",KickAngle);
                    // log("ballPosition",ballPosition);
                    // log("secondsSinceLastSeen",secondsSinceLastSeen);
                    bool correctState = true;
                    if (gameState) {
                        // If we are playing with GameController, only kick if we are in the playing state, are not
                        // penalised and are not in ready state
                        correctState = gameState->data.phase == Phase::PLAYING
                                       && gameState->data.self.penalty_reason == PenaltyReason::UNPENALISED
                                       && gameState->data.phase != Phase::READY;
                    }
                    bool kickIsValid = kickValid(ballPosition);
                    if (kickIsValid) {
                        lastTimeValid = now;
                    }
                    float timeSinceValid = (now - lastTimeValid).count() * (1 / double(NUClear::clock::period::den));

                    // log("kick checks",secondsSinceLastSeen < cfg.seconds_not_seen_limit
                    //     , kickIsValid
                    //     , KickAngle < cfg.kick_forward_angle_limit);
                    if (secondsSinceLastSeen < cfg.seconds_not_seen_limit && kickIsValid
                        && (correctState || forcePlaying) && KickAngle < cfg.kick_forward_angle_limit) {

                        switch (kickPlan.kickType.value) {
                            case KickType::IK_KICK:
                                // NUClear::log("ik_kick");
                                if (ballPosition[1] > 0) {
                                    emit(std::make_unique<KickCommand>(KickCommand(Eigen::Vector3d(0.1, 0.04, 0),
                                                                                   Eigen::Vector3d(1.0, 0.0, 0.0),
                                                                                   KickCommandType::NORMAL)));
                                    emit(std::make_unique<WantsToKick>(true));
                                }
                                else {
                                    emit(std::make_unique<KickCommand>(KickCommand(Eigen::Vector3d(0.1, -0.04, 0),
                                                                                   Eigen::Vector3d(1.0, 0.0, 0.0),
                                                                                   KickCommandType::NORMAL)));
                                    emit(std::make_unique<WantsToKick>(true));
                                }
                                break;
                            case KickType::SCRIPTED:
                                // NUClear::log("scripted");
                                if (ballPosition[1] > 0) {
                                    emit(std::make_unique<KickScriptCommand>(
                                        KickScriptCommand(Eigen::Vector3d(1.0, 0.0, 0.0), LimbID::LEFT_LEG)));
                                    emit(std::make_unique<WantsToKick>(true));
                                    ;
                                }
                                else {
                                    emit(std::make_unique<KickScriptCommand>(
                                        KickScriptCommand(Eigen::Vector3d(1.0, 0.0, 0.0), LimbID::RIGHT_LEG)));
                                    emit(std::make_unique<WantsToKick>(true));
                                    ;
                                }
                                break;
                            default: throw new std::runtime_error("KickPlanner: Invalid KickType");
                        }
                    }
                    else if (secondsSinceLastSeen > cfg.seconds_not_seen_limit
                             || timeSinceValid > cfg.seconds_not_seen_limit) {
                        emit(std::make_unique<WantsToKick>(WantsToKick(false)));
                    }
                });
        }


        bool KickPlanner::kickValid(const arma::vec3& ballPos) {
            return (ballPos[0] > 0) && (ballPos[0] < cfg.max_ball_distance)
                   && (std::fabs(ballPos[1]) < cfg.kick_corridor_width / 2.0);
        }
    }  // namespace planning
}  // namespace behaviour
}  // namespace module

#include "QuinticWalk.hpp"

#include <fmt/format.h>

#include "extension/Configuration.hpp"

#include "message/motion/GetupCommand.hpp"
#include "message/motion/KinematicsModel.hpp"
#include "message/motion/WalkCommand.hpp"
#include "message/support/SaveConfiguration.hpp"

#include "utility/math/comparison.hpp"
#include "utility/math/euler.hpp"
#include "utility/motion/InverseKinematics.hpp"
#include "utility/support/yaml_expression.hpp"

namespace module::motion {

    using extension::Configuration;

    using message::behaviour::ServoCommands;
    using message::input::Sensors;
    using message::motion::DisableWalkEngineCommand;
    using message::motion::EnableWalkEngineCommand;
    using message::motion::ExecuteGetup;
    using message::motion::KillGetup;
    using message::motion::KinematicsModel;
    using message::motion::StopCommand;
    using message::motion::WalkCommand;
    using utility::support::Expression;

    using utility::input::ServoID;
    using utility::motion::kinematics::calculateLegJoints;

    QuinticWalk::QuinticWalk(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        imu_reaction = on<Trigger<Sensors>>().then([this](const Sensors& sensors) {
            Eigen::Vector3f RPY =
                utility::math::euler::MatrixToEulerIntrinsic(sensors.Htw.topLeftCorner<3, 3>().cast<float>());

            // compute the pitch offset to the currently wanted pitch of the engine
            float wanted_pitch = params.trunk_pitch
                                 + params.trunk_pitch_p_coef_forward * walk_engine.getFootstep().getNext().x()
                                 + params.trunk_pitch_p_coef_turn * std::abs(walk_engine.getFootstep().getNext().z());
            RPY.y() += wanted_pitch;

            // threshold pitch and roll
            if (std::abs(RPY.x()) > config.imu_roll_threshold) {
                log<NUClear::WARN>(fmt::format("Robot roll exceeds threshold - {} > {}",
                                               std::abs(RPY.x()),
                                               config.imu_roll_threshold));
                walk_engine.requestPause();
            }
            else if (std::abs(RPY.y()) > config.imu_pitch_threshold) {
                log<NUClear::WARN>(fmt::format("Robot pitch exceeds threshold - {} > {}",
                                               std::abs(RPY.y()),
                                               config.imu_pitch_threshold));
                walk_engine.requestPause();
            }
        });

        on<Configuration>("QuinticWalk.yaml").then([this](const Configuration& cfg) {
            // Use configuration here from file QuinticWalk.yaml
            params.freq                          = cfg["walk"]["freq"].as<float>();
            params.double_support_ratio          = cfg["walk"]["double_support_ratio"].as<float>();
            params.first_step_swing_factor       = cfg["walk"]["first_step_swing_factor"].as<float>();
            params.foot_distance                 = cfg["walk"]["foot"]["distance"].as<float>();
            params.foot_rise                     = cfg["walk"]["foot"]["rise"].as<float>();
            params.foot_z_pause                  = cfg["walk"]["foot"]["z_pause"].as<float>();
            params.foot_put_down_z_offset        = cfg["walk"]["foot"]["put_down"]["z_offset"].as<float>();
            params.foot_put_down_phase           = cfg["walk"]["foot"]["put_down"]["phase"].as<float>();
            params.foot_put_down_roll_offset     = cfg["walk"]["foot"]["put_down"]["roll_offset"].as<float>();
            params.foot_apex_phase               = cfg["walk"]["foot"]["apex_phase"].as<float>();
            params.foot_overshoot_ratio          = cfg["walk"]["foot"]["overshoot"]["ratio"].as<float>();
            params.foot_overshoot_phase          = cfg["walk"]["foot"]["overshoot"]["phase"].as<float>();
            params.trunk_height                  = cfg["walk"]["trunk"]["height"].as<float>();
            params.trunk_pitch                   = 1.0f + cfg["walk"]["trunk"]["pitch"].as<Expression>();
            params.trunk_phase                   = cfg["walk"]["trunk"]["phase"].as<float>();
            params.trunk_x_offset                = cfg["walk"]["trunk"]["x_offset"].as<float>();
            params.trunk_y_offset                = cfg["walk"]["trunk"]["y_offset"].as<float>();
            params.trunk_swing                   = cfg["walk"]["trunk"]["swing"].as<float>();
            params.trunk_pause                   = cfg["walk"]["trunk"]["pause"].as<float>();
            params.trunk_x_offset_p_coef_forward = cfg["walk"]["trunk"]["x_offset_p_coef"]["forward"].as<float>();
            params.trunk_x_offset_p_coef_turn    = cfg["walk"]["trunk"]["x_offset_p_coef"]["turn"].as<float>();
            params.trunk_pitch_p_coef_forward = 1.0f + cfg["walk"]["trunk"]["pitch_p_coef"]["forward"].as<Expression>();
            params.trunk_pitch_p_coef_turn    = 1.0f + cfg["walk"]["trunk"]["pitch_p_coef"]["turn"].as<Expression>();
            params.kick_length                = cfg["walk"]["kick"]["length"].as<float>();
            params.kick_phase                 = cfg["walk"]["kick"]["phase"].as<float>();
            params.kick_vel                   = cfg["walk"]["kick"]["vel"].as<float>();
            params.pause_duration             = cfg["walk"]["pause"]["duration"].as<float>();

            // Send these parameters to the walk engine
            walk_engine.setParameters(params);

            config.max_step[0] = cfg["max_step"]["x"].as<float>();
            config.max_step[1] = cfg["max_step"]["y"].as<float>();
            config.max_step[2] = cfg["max_step"]["z"].as<float>();
            config.max_step_xy = cfg["max_step"]["xy"].as<float>();

            config.imu_active          = cfg["imu"]["active"].as<bool>();
            config.imu_pitch_threshold = 1.0f + cfg["imu"]["pitch"]["threshold"].as<float>();
            config.imu_roll_threshold  = cfg["imu"]["roll"]["threshold"].as<float>();

            for (int i = 0; i < ServoID::NUMBER_OF_SERVOS; ++i) {
                if ((i >= 6) && (i < 18)) {
                    jointGains[i] = cfg["gains"]["legs"].as<float>();
                }
                if (i < 6) {
                    jointGains[i] = cfg["gains"]["arms"].as<float>();
                }
            }

            arm_positions.push_back(
                std::make_pair(ServoID::R_SHOULDER_PITCH, cfg["arms"]["right_shoulder_pitch"].as<float>()));
            arm_positions.push_back(
                std::make_pair(ServoID::L_SHOULDER_PITCH, cfg["arms"]["left_shoulder_pitch"].as<float>()));
            arm_positions.push_back(
                std::make_pair(ServoID::R_SHOULDER_ROLL, cfg["arms"]["right_shoulder_roll"].as<float>()));
            arm_positions.push_back(
                std::make_pair(ServoID::L_SHOULDER_ROLL, cfg["arms"]["left_shoulder_roll"].as<float>()));
            arm_positions.push_back(std::make_pair(ServoID::R_ELBOW, cfg["arms"]["right_elbow"].as<float>()));
            arm_positions.push_back(std::make_pair(ServoID::L_ELBOW, cfg["arms"]["left_elbow"].as<float>()));

            imu_reaction.enable(config.imu_active);
        });

        on<Startup, Trigger<KinematicsModel>>().then("Update Kinematics Model", [this](const KinematicsModel& model) {
            kinematicsModel = model;
            first_run       = true;
            current_orders.setZero();
            is_left_support  = true;
            falling          = false;
            last_update_time = NUClear::clock::now();
            walk_engine.reset();
        });

        on<Trigger<ExecuteGetup>>().then([this]() { falling = true; });

        on<Trigger<KillGetup>>().then([this]() { falling = false; });

        on<Trigger<StopCommand>>().then([this](const StopCommand& walkCommand) {
            subsumptionId = walkCommand.subsumption_id;
            current_orders.setZero();
        });

        on<Trigger<WalkCommand>>().then([this](const WalkCommand& walkCommand) {
            subsumptionId = walkCommand.subsumption_id;

            // the engine expects orders in [m] not [m/s]. We have to compute by dividing by step frequency which is
            // a double step factor 2 since the order distance is only for a single step, not double step
            const float factor             = (1.0 / (params.freq)) / 2.0;
            const Eigen::Vector3f& command = walkCommand.command.cast<float>() * factor;

            // Clamp velocity command
            Eigen::Vector3f orders =
                command.array().max(-config.max_step.array()).min(config.max_step.array()).matrix();

            // translational orders (x+y) should not exceed combined limit. scale if necessary
            if (config.max_step_xy != 0) {
                float scaling_factor = 1.0f / std::max(1.0f, (orders.x() + orders.y()) / config.max_step_xy);
                orders.cwiseProduct(Eigen::Vector3f(scaling_factor, scaling_factor, 1.0f));
            }

            // warn user that speed was limited
            if (command.x() != orders.x() || command.y() != orders.y() || command.z() != orders.z()) {
                log<NUClear::WARN>(
                    fmt::format("Speed command was x: {} y: {} z: {} xy: {} but maximum is x: {} y: {} z: {} xy: {}",
                                command.x(),
                                command.y(),
                                command.z(),
                                command.x() + command.y(),
                                config.max_step[0] / factor,
                                config.max_step[1] / factor,
                                config.max_step[2] / factor,
                                config.max_step_xy / factor));
            }

            // Update orders
            current_orders = orders;
        });

        on<Trigger<EnableWalkEngineCommand>>().then([this](const EnableWalkEngineCommand& command) {
            subsumptionId = command.subsumption_id;
            walk_engine.reset();
            update_handle.enable();
        });

        on<Trigger<DisableWalkEngineCommand>>().then([this](const DisableWalkEngineCommand& command) {
            subsumptionId = command.subsumption_id;
            update_handle.disable();
        });

        update_handle = on<Every<UPDATE_FREQUENCY, Per<std::chrono::seconds>>, Single>().then([this]() {
            const float dt = getTimeDelta();

            if (falling) {
                // We are falling, reset walk engine
                walk_engine.reset();
            }
            else {

                // see if the walk engine has new goals for us
                if (walk_engine.updateState(dt, current_orders)) {
                    calculateJointGoals();
                }
            }
        });
    }

    float QuinticWalk::getTimeDelta() {
        // compute time delta depended if we are currently in simulation or reality
        const auto current_time = NUClear::clock::now();
        float dt =
            std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_update_time).count() / 1000.0f;

        if (dt == 0.0f) {
            // log<NUClear::WARN>(fmt::format("dt was 0 ({})", time_diff_ms.count()));
            dt = 0.001f;
        }

        // time is wrong when we run it for the first time
        if (first_run) {
            first_run = false;
            dt        = 0.0001f;
        }

        last_update_time = current_time;
        return dt;
    }

    void QuinticWalk::calculateJointGoals() {
        /*
        This method computes the next motor goals and publishes them.
        */
        auto setRPY = [&](const float& roll, const float& pitch, const float& yaw) {
            const float halfYaw   = yaw * 0.5f;
            const float halfPitch = pitch * 0.5f;
            const float halfRoll  = roll * 0.5f;
            const float cosYaw    = std::cos(halfYaw);
            const float sinYaw    = std::sin(halfYaw);
            const float cosPitch  = std::cos(halfPitch);
            const float sinPitch  = std::sin(halfPitch);
            const float cosRoll   = std::cos(halfRoll);
            const float sinRoll   = std::sin(halfRoll);
            return Eigen::Quaternionf(cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw,  // formerly yzx
                                      sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw,  // x
                                      cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw,  // y
                                      cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw   // z
                                      )
                .normalized()
                .toRotationMatrix();
        };
        // Read the cartesian positions and orientations for trunk and fly foot
        std::tie(trunk_pos, trunk_axis, foot_pos, foot_axis, is_left_support) = walk_engine.computeCartesianPosition();

        // Change goals from support foot based coordinate system to trunk based coordinate system
        Eigen::Affine3f Hst;  // trunk_to_support_foot_goal
        Hst.linear()      = setRPY(trunk_axis.x(), trunk_axis.y(), trunk_axis.z()).transpose();
        Hst.translation() = -Hst.rotation() * trunk_pos;

        Eigen::Affine3f Hfs;  // support_to_flying_foot
        Hfs.linear()      = setRPY(foot_axis.x(), foot_axis.y(), foot_axis.z());
        Hfs.translation() = foot_pos;

        const Eigen::Affine3f Hft = Hfs * Hst;  // trunk_to_flying_foot_goal

        // Calculate leg joints
        const Eigen::Matrix4d left_foot =
            walk_engine.getFootstep().isLeftSupport() ? Hst.matrix().cast<double>() : Hft.matrix().cast<double>();
        const Eigen::Matrix4d right_foot =
            walk_engine.getFootstep().isLeftSupport() ? Hft.matrix().cast<double>() : Hst.matrix().cast<double>();

        const auto joints = calculateLegJoints(kinematicsModel,
                                               Eigen::Affine3f(left_foot.cast<float>()),
                                               Eigen::Affine3f(right_foot.cast<float>()));

        auto waypoints = motion(joints);
        emit(std::move(waypoints));
    }

    std::unique_ptr<ServoCommands> QuinticWalk::motion(const std::vector<std::pair<ServoID, float>>& joints) {
        auto waypoints = std::make_unique<ServoCommands>();
        waypoints->commands.reserve(joints.size() + arm_positions.size());

        const NUClear::clock::time_point time = NUClear::clock::now() + Per<std::chrono::seconds>(UPDATE_FREQUENCY);

        for (const auto& joint : joints) {
            waypoints->commands
                .emplace_back(subsumptionId, time, joint.first, joint.second, jointGains[joint.first], 100);
        }

        for (const auto& joint : arm_positions) {
            waypoints->commands
                .emplace_back(subsumptionId, time, joint.first, joint.second, jointGains[joint.first], 100);
        }

        return waypoints;
    }
}  // namespace module::motion

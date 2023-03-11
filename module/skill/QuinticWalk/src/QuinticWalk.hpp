#ifndef MODULE_SKILL_QUINTICWALK_HPP
#define MODULE_SKILL_QUINTICWALK_HPP

#include <map>
#include <memory>
#include <nuclear>
#include <vector>

#include "extension/Behaviour.hpp"
#include "extension/Configuration.hpp"

#include "message/actuation/KinematicsModel.hpp"
#include "message/behaviour/ServoCommand.hpp"
#include "message/behaviour/state/Stability.hpp"

#include "utility/input/ServoID.hpp"
#include "utility/motion/WalkEngine.hpp"

namespace module::skill {

    class QuinticWalk : public ::extension::behaviour::BehaviourReactor {

    public:
        /// @brief Called by the powerplant to build and setup the QuinticWalk reactor.
        explicit QuinticWalk(std::unique_ptr<NUClear::Environment> environment);

        static constexpr int UPDATE_FREQUENCY = 200;

    private:
        // Reaction handle for the imu reaction, disabling when not moving will save unnecessary CPU
        ReactionHandle imu_reaction{};

        void calculate_joint_goals();
        [[nodiscard]] float get_time_delta();
        [[nodiscard]] std::unique_ptr<message::behaviour::ServoCommands> motion(
            const std::vector<std::pair<utility::input::ServoID, float>>& joints);

        struct Config {
            Eigen::Vector3f max_step = Eigen::Vector3f::Zero();
            float max_step_xy        = 0.0f;

            bool imu_active           = true;
            float imu_pitch_threshold = 0.0f;
            float imu_roll_threshold  = 0.0f;

            utility::motion::WalkingParameter params{};

            std::vector<std::pair<utility::input::ServoID, float>> arm_positions{};
        } normal_cfg{}, goalie_cfg{};

        static void load_quintic_walk(const ::extension::Configuration& cfg, Config& config);

        /// @brief Stores the walking config
        Config& current_config = normal_config;

        bool first_config = true;

        Eigen::Vector3f current_orders = Eigen::Vector3f::Zero();
        bool is_left_support           = true;
        bool first_run                 = true;

        NUClear::clock::time_point last_update_time{};

        utility::motion::QuinticWalkEngine walk_engine{};

        message::actuation::KinematicsModel kinematicsModel{};
    };
}  // namespace module::skill

#endif  // MODULE_SKILL_QUINTICWALK_HPP

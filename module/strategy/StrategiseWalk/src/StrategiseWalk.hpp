#ifndef MODULE_STRATEGY_STRATEGISEWALK_HPP
#define MODULE_STRATEGY_STRATEGISEWALK_HPP

#include <nuclear>

#include "extension/Behaviour.hpp"

namespace module::strategy {

    class StrategiseWalk : public ::extension::behaviour::BehaviourReactor {
    private:
        /// @brief Stores configuration values
        struct Config {
            /// @brief Length of time before the ball detection is too old and we should search for the ball
            NUClear::clock::duration ball_search_timeout{};
            /// @brief Offset to align the ball with the robot's foot
            float ball_y_offset = 0.0;
        } cfg;

    public:
        /// @brief Called by the powerplant to build and setup the StrategiseWalk reactor.
        explicit StrategiseWalk(std::unique_ptr<NUClear::Environment> environment);
    };

}  // namespace module::strategy

#endif  // MODULE_STRATEGY_STRATEGISEWALK_HPP

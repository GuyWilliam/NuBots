#ifndef MODULE_STRATEGY_DEFEND_HPP
#define MODULE_STRATEGY_DEFEND_HPP

#include <Eigen/Core>
#include <nuclear>

#include "extension/Behaviour.hpp"

namespace module::strategy {

    class Defend : public ::extension::behaviour::BehaviourReactor {
    private:
        /// @brief Stores configuration values
        struct Config {

            /// @brief Ready position to walk to (x, y, theta)
            Eigen::Vector3f defender_position = Eigen::Vector3f::Zero();

            /// @brief Region to defend the ball
            Eigen::Vector4f defending_region = Eigen::Vector4f::Zero();

        } cfg;

    public:
        /// @brief Called by the powerplant to build and setup the Defend reactor.
        explicit Defend(std::unique_ptr<NUClear::Environment> environment);
    };

}  // namespace module::strategy

#endif  // MODULE_STRATEGY_DEFEND_HPP

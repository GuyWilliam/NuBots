#ifndef MODULE_STRATEGY_WALKTOBALL_HPP
#define MODULE_STRATEGY_WALKTOBALL_HPP

#include <nuclear>

#include "extension/Behaviour.hpp"

namespace module::strategy {

    class WalkToBall : public ::extension::behaviour::BehaviourReactor {
    private:
        /// @brief Stores configuration values
        struct Config {
            NUClear::clock::duration ball_search_timeout{};
        } cfg;

    public:
        /// @brief Called by the powerplant to build and setup the WalkToBall reactor.
        explicit WalkToBall(std::unique_ptr<NUClear::Environment> environment);
    };

}  // namespace module::strategy

#endif  // MODULE_STRATEGY_WALKTOBALL_HPP

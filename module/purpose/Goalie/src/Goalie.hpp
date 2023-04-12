#ifndef MODULE_PURPOSE_GOALIE_HPP
#define MODULE_PURPOSE_GOALIE_HPP

#include <nuclear>

#include "extension/Behaviour.hpp"

namespace module::purpose {

    class Goalie : public ::extension::behaviour::BehaviourReactor {
    private:
        /// @brief Calls Tasks to play soccer as a goalie
        void play();

    public:
        /// @brief Called by the powerplant to build and setup the Goalie reactor.
        explicit Goalie(std::unique_ptr<NUClear::Environment> environment);
    };

}  // namespace module::purpose

#endif  // MODULE_PURPOSE_GOALIE_HPP

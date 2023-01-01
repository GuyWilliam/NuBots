#ifndef MODULE_PLANNING_PLANWALKPATH_HPP
#define MODULE_PLANNING_PLANWALKPATH_HPP

#include <nuclear>

#include "extension/Behaviour.hpp"

namespace module::planning {

class PlanWalkPath : public ::extension::behaviour::BehaviourReactor {
private:
    /// @brief Stores configuration values
    struct Config {
    } cfg;

public:
    /// @brief Called by the powerplant to build and setup the PlanWalkPath reactor.
    explicit PlanWalkPath(std::unique_ptr<NUClear::Environment> environment);
};

}  // namespace module::planning

#endif  // MODULE_PLANNING_PLANWALKPATH_HPP

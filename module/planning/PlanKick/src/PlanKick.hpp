#ifndef MODULE_PLANNING_PLANKICK_HPP
#define MODULE_PLANNING_PLANKICK_HPP

#include <extension/Behaviour.hpp>
#include <nuclear>
#include <string>

namespace module::planning {

    class PlanKick : public ::extension::behaviour::BehaviourReactor {
    private:
        /// @brief Stores configuration values
        struct Config {
            float ball_distance_threshold = 0.0;
            float ball_angle_threshold    = 0.0;
            bool align                    = false;
            float target_angle_threshold  = 0.0;
            std::string kick_leg          = "";
        } cfg;

    public:
        /// @brief Called by the powerplant to build and setup the PlanKick reactor.
        explicit PlanKick(std::unique_ptr<NUClear::Environment> environment);
    };

}  // namespace module::planning

#endif  // MODULE_PLANNING_PLANKICK_HPP

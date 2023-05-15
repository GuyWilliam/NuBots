#ifndef MODULE_NETWORK_ROBOTCOMMUNICATION_HPP
#define MODULE_NETWORK_ROBOTCOMMUNICATION_HPP

#include <nuclear>

namespace module::network {

    class RobotCommunication : public NUClear::Reactor {
    private:
        /// @brief Stores configuration values
        struct Config {
        } cfg;

    public:
        /// @brief Called by the powerplant to build and setup the RobotCommunication reactor.
        explicit RobotCommunication(std::unique_ptr<NUClear::Environment> environment);
    };

}  // namespace module::network

#endif  // MODULE_NETWORK_ROBOTCOMMUNICATION_HPP

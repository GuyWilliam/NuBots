#include "StandStill.hpp"

#include "extension/Behaviour.hpp"
#include "extension/Configuration.hpp"

#include "message/actuation/Limbs.hpp"
#include "message/behaviour/state/Stability.hpp"
#include "message/skill/Walk.hpp"
#include "message/strategy/StandStill.hpp"

#include "utility/motion/Script.hpp"

namespace module::strategy {

    using extension::Configuration;
    using message::actuation::LimbsSequence;
    using message::behaviour::state::Stability;
    using message::skill::Walk;
    using utility::motion::load_script;

    StandStill::StandStill(std::unique_ptr<NUClear::Environment> environment)
        : BehaviourReactor(std::move(environment)) {

        on<Configuration>("StandStill.yaml").then([this](const Configuration& config) {
            // Use configuration here from file StandStill.yaml
            this->log_level = config["log_level"].as<NUClear::LogLevel>();
        });

        on<Provide<StandStill>, With<Stability>>().then([this](const Stability& stability) {
            // If we are stable, then we can provide the StandStill command
            if (stability != Stability::STANDING) {
                emit<Task>(std::make_unique<Walk>(Eigen::Vector3f::Zero()));
            }
            else {
                emit<Task>(load_script<LimbsSequence>("Stand.yaml"));
            }
        });
    }

}  // namespace module::strategy

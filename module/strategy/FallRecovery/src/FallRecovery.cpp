#include "FallRecovery.hpp"

#include "extension/Behaviour.hpp"

#include "message/strategy/FallRecovery.hpp"

namespace module::strategy {

    using FallRecoveryTask = message::strategy::FallRecovery;

    FallRecovery::FallRecovery(std::unique_ptr<NUClear::Environment> environment)
        : BehaviourReactor(std::move(environment)) {

        on<Provide<FallRecoveryTask>>().then([this] {
            // Plan to relax when falling and get up when on the ground
            // We set the priority of PlanGetUp higher than PlanFallingRelax so that relax won't take over while we are
            // getting up
            emit<Task>(std::make_unique<PlanGetUp>(), 2);
            emit<Task>(std::make_unique<PlanFallingRelax>(), 1);
        });
    }

}  // namespace module::strategy

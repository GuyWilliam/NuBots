#ifndef MODULE_MOTION_SERVOS_HPP
#define MODULE_MOTION_SERVOS_HPP

#include <nuclear>

#include "extension/Behaviour.hpp"

#include "message/actuation/ServoTarget.hpp"
#include "message/input/Sensors.hpp"

#include "utility/actuation/ServoMap.hpp"
#include "utility/input/ServoID.hpp"

namespace module::actuation {
    using message::actuation::ServoTarget;
    using message::input::Sensors;
    using utility::input::ServoID;

    class Servos : public ::extension::behaviour::BehaviourReactor {
    private:
        /// @brief Creates a reaction that sends a given servo command as a servo target command for the platform module
        /// to use
        // TODO(ysims): add capability to be Done when the servo reaches the target position
        template <typename Servo, ServoID::Value ID>
        void add_servo_provider() {
            on<Provide<Servo>, Trigger<Sensors>>().then([this](const Servo& servo, const RunInfo& info) {
                if (info.run_reason == RunInfo::RunReason::NEW_TASK) {
                    emit(std::make_unique<ServoTarget>(servo.command.time,
                                                       ID,
                                                       servo.command.position,
                                                       servo.command.state.gain,
                                                       servo.command.state.torque));
                }
                // If the time to reach the position is over, then stop requesting the position
                else if (NUClear::clock::now() >= servo.command.time) {
                    emit<Task>(std::make_unique<Done>());
                }
            });
        }

        /// @brief Creates a reaction that takes a servo wrapper task (eg LeftLeg) and emits a task for each servo.
        /// Emits Done when all servo tasks are Done.
        /// @tparam Group is a servo wrapper task (eg LeftLeg)
        /// @tparam Elements is a template pack of Servos that Group uses
        template <typename Group, typename... Elements>
        void add_group_provider() {
            on<Provide<Group>, Needs<Elements>...>().then(
                [this](const Group& group, const RunInfo& info, const Uses<Elements>... elements) {
                    // Check if any subtask is Done
                    if (info.run_reason == RunInfo::RunReason::SUBTASK_DONE) {
                        // If every servo task is done then emit Done
                        if ((elements.done && ...)) {
                            emit<Task>(std::make_unique<Done>());
                            return;
                        }
                        // Emit Idle if all the servos are not Done yet
                        emit<Task>(std::make_unique<Idle>());
                        return;
                    }

                    // Runs an emit for each servo
                    NUClear::util::unpack((emit<Task>(std::make_unique<Elements>(
                                               group.servos.at(utility::actuation::ServoMap<Elements>::value))),
                                           0)...);
                });
        }

        /// @brief Keeps track of which Group in the Sequence in add_sequence_provider should be emitted next
        /// @details Should be used carefully and may be bug-prone.
        /// @tparam Sequence Used to have unique counts for each Sequence Provider.
        template <typename Sequence>
        struct Count {
            long unsigned int count = 0;
        };

        /// @brief Creates a reaction that takes a vector of servo wrapper tasks (eg LeftLegs) and emits each servo
        /// wrapper task after the previous one is Done. Emits Done when the last group is Done.
        /// @tparam Sequence is a vector of servo wrapper tasks (eg LeftLegSequence)
        /// @tparam Group is a servo wrapper task (eg LeftLeg)
        template <typename Sequence, typename Group>
        void add_sequence_provider() {
            // Message to keep track of which position in the vector is to be emitted
            // Make an initial count message
            emit<Scope::DIRECT>(std::make_unique<Count<Sequence>>(0));

            on<Provide<Sequence>, Needs<Group>, With<Count<Sequence>>>().then(
                [this](const Sequence& sequence, const RunInfo& info, const Count<Sequence>& count) {
                    // If the user gave us nothing then we are done
                    if (sequence.frames.empty()) {
                        emit<Task>(std::make_unique<Done>());
                    }
                    // If this is a new task, run the first pack of servos and increment the counter
                    else if (info.run_reason == RunInfo::RunReason::NEW_TASK) {
                        emit<Task>(std::make_unique<Group>(sequence.frames[0]));
                        emit<Scope::DIRECT>(std::make_unique<Count<Sequence>>(1));
                    }
                    // If the subtask is done, we are done if it is the last servo frames, otherwise use the count to
                    // determine the current frame to emit
                    else if (info.run_reason == RunInfo::RunReason::SUBTASK_DONE) {
                        if (count.count < sequence.frames.size()) {
                            emit<Task>(std::make_unique<Group>(sequence.frames[count.count]));
                            emit<Scope::DIRECT>(std::make_unique<Count<Sequence>>(count.count + 1));
                        }
                        else {
                            emit<Task>(std::make_unique<Done>());
                        }
                    }
                    // Any other run reason shouldn't happen
                    else {
                        emit<Task>(std::make_unique<Idle>());
                    }
                });
        }

    public:
        /// @brief Called by the powerplant to build and setup the Servos reactor.
        explicit Servos(std::unique_ptr<NUClear::Environment> environment);
    };

}  // namespace module::actuation

#endif  // MODULE_MOTION_SERVOS_HPP

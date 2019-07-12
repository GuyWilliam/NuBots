#ifndef EXTENSION_BEHAVIOUR_H
#define EXTENSION_BEHAVIOUR_H

#include <memory>
#include <nuclear>

namespace extension {
namespace behaviour {

    namespace commands {
        struct EnteringReaction {
            EnteringReaction(const std::shared_ptr<NUClear::threading::Reaction>& reaction, const std::type_index& type)
                : reaction(reaction), type(type) {}
            std::shared_ptr<NUClear::threading::Reaction> reaction;
            std::type_index type;
        };

        struct LeavingReaction {
            LeavingReaction(const std::shared_ptr<NUClear::threading::Reaction>& reaction, const std::type_index& type)
                : reaction(reaction), type(type) {}
            std::shared_ptr<NUClear::threading::Reaction> reaction;
            std::type_index type;
        };

        struct ProvidesReaction {
            ProvidesReaction(const std::shared_ptr<NUClear::threading::Reaction>& reaction, const std::type_index& type)
                : reaction(reaction), type(type) {}
            std::shared_ptr<NUClear::threading::Reaction> reaction;
            std::type_index type;
        };

        struct WhenExpression {
            WhenExpression(const std::shared_ptr<NUClear::threading::Reaction>& reaction,
                           const std::type_index& type,
                           bool (*expr)(const int&),
                           int (*current)())
                : reaction(reaction), type(type), expr(expr), current(current) {}
            std::shared_ptr<NUClear::threading::Reaction> reaction;
            std::type_index type;
            bool (*expr)(const int&);  // Expression to determine if the state is valid
            int (*current)();          // Function to get the current state from the global cache
        };

        struct CausingExpression {
            CausingExpression(const std::shared_ptr<NUClear::threading::Reaction>& reaction,
                              const std::type_index& type,
                              const int& resulting_state)
                : reaction(reaction), type(type), resulting_state(resulting_state) {}
            std::shared_ptr<NUClear::threading::Reaction> reaction;
            std::type_index type;
            int resulting_state;
        };

        struct DirectorTask {
            DirectorTask(std::type_index type,
                         uint64_t requester_id,
                         std::shared_ptr<void> command,
                         int priority,
                         const std::string& name)
                : type(type), requester_id(requester_id), command(command), priority(priority), name(name) {}
            std::type_index type;
            uint64_t requester_id;
            std::shared_ptr<void> command;
            int priority;
            std::string name;
        };
    }  // namespace commands

    /**
     * This DSL word used to define an entry transition into a provider.
     * It should normally be coupled with a Causing DSL word.
     * When coupled with a Causing DSL word and a provider that has a When condition,
     * this entry will run in place of the provider until the When condition is fulfilled.
     * If it is used without a Causing DSL word it will run once as a provider before the actual provider is executed.
     *
     * @tparam T the provider type that this transition function provides for
     */
    template <typename T>
    struct Entering {

        template <typename DSL>
        static inline void bind(const std::shared_ptr<NUClear::threading::Reaction>& reaction) {

            // Tell the director
            reaction->reactor.powerplant.emit(std::make_unique<commands::EnteringReaction>(reaction, typeid(T)));

            // Add our unbinder
            reaction->unbinders.emplace_back([](const NUClear::threading::Reaction& r) {
                r.reactor.emit<NUClear::dsl::word::emit::Direct>(
                    std::make_unique<NUClear::dsl::operation::Unbind<commands::ProvidesReaction>>(r.id));
            });
        }
    };

    /**
     * This DSL word is used to define a leaving transition from a provider.
     * It should normally be combined with a Causing DSL word.
     * In this case when another provider needs control and uses a When condition, this will run until that condition is
     * fulfilled.
     * A Leaving condition will be preferred over an Entering condition that provides the same Causing statement.
     *
     * @tparam T the provider type that this transition function provides for
     */
    template <typename T>
    struct Leaving {

        template <typename DSL>
        static inline void bind(const std::shared_ptr<NUClear::threading::Reaction>& reaction) {

            // Tell the director
            reaction->reactor.powerplant.emit(std::make_unique<commands::LeavingReaction>(reaction, typeid(T)));

            // Add our unbinder
            reaction->unbinders.emplace_back([](const NUClear::threading::Reaction& r) {
                r.reactor.emit<NUClear::dsl::word::emit::Direct>(
                    std::make_unique<NUClear::dsl::operation::Unbind<commands::ProvidesReaction>>(r.id));
            });
        }
    };

    /**
     * This DSL word is used to define a provider for a type.
     * It will execute in the same way as an on<Trigger<T>> statement except that when it executes will be determined
     * by the Director. This ensures that it will only run when it has permission to run and will transition between
     * providers in a sensible way.
     *
     * @tparam T the provider type that this transition function provides for
     */
    template <typename T>
    struct Provides {

        template <typename DSL>
        static inline void bind(const std::shared_ptr<NUClear::threading::Reaction>& reaction) {

            // Tell the director
            reaction->reactor.powerplant.emit(std::make_unique<commands::ProvidesReaction>(reaction, typeid(T)));

            // Add our unbinder
            reaction->unbinders.emplace_back([](const NUClear::threading::Reaction& r) {
                r.reactor.emit<NUClear::dsl::word::emit::Direct>(
                    std::make_unique<NUClear::dsl::operation::Unbind<commands::ProvidesReaction>>(r.id));
            });
        }

        template <typename DSL>
        static inline std::shared_ptr<T> get(NUClear::threading::Reaction& t) {

            // TODO get the instance of the task command data from the director
            return nullptr;
        }
    };

    /**
     * The When DSL word is used to limit access to a provider unless a condition is true.
     * This will prevent a provider from running if the state is false.
     * However if there is a Entering<T> or Leaving reaction that has a Causing relationship for this When, it will be
     * executed to ensure that the Provider can run.
     *
     * @tparam T the condition expression that must be true in order to execute this task.
     */
    template <typename State, template <typename> class expr, enum State::Value value>
    struct When {

        template <typename DSL>
        static inline void bind(const std::shared_ptr<NUClear::threading::Reaction>& reaction) {

            // Tell the director about this when condition
            reaction->reactor.powerplant.emit(std::make_unique<commands::WhenExpression>(
                reaction,
                typeid(State),
                [](const int& v) { return expr<int>()(v, value); },
                [] { return NUClear::dsl::operation::CacheGet<State>::get(); }));
        }
    };

    /**
     * This DSL word is used to create a promise that at some point when running this reaction the state indicated by T
     * will be true. It may not be on any particular run of the reaction, in which case it will continue to run that
     * reaction until it is true, or another graph jump becomes necessary.
     *
     * @tparam T the condition expression that this reaction will make true before leaving.
     */
    template <typename State, enum State::Value value>
    struct Causing {

        template <typename DSL>
        static inline void bind(const std::shared_ptr<NUClear::threading::Reaction>& reaction) {
            // Tell the director
            reaction->reactor.powerplant.emit(
                std::make_unique<commands::CausingExpression>(reaction, typeid(State), value));
        }
    };  // namespace behaviour

    /**
     * A Task to be executed by the director, and a priority with which to execute this task.
     * The scope of the priority is contained within the reaction that emitted it.
     * So if a provider emits two tasks it will execute the one with higher priority.
     * The exception to this rule is task that are emitted from reactions that are not providers.
     * When a task is emitted by one of these general reactions it exists in a pool along with all other non provider
     * reactions. This is used to provide a list of all the tasks that must be executed by the system.
     *
     * When emitting a task, there is always only a single task for a given provider per reaction.
     * This task will persist until a new task is emitted from the reaction for the same provider and then this
     * task will be used instead.
     *
     * If the provided priority is zero, this task will instead be removed from the task list.
     *
     * @tparam T the provider type that this task is for
     */
    template <typename T>
    struct Task {
        static void emit(NUClear::PowerPlant& powerplant,
                         std::shared_ptr<T> data,
                         int priority,
                         const std::string& name = "") {

            // Work out who is sending the task
            auto* task  = NUClear::threading::ReactionTask::get_current_task();
            uint64_t id = task ? task->parent.id : -1;

            NUClear::dsl::word::emit::Direct<T>::emit(
                powerplant, std::make_shared<commands::DirectorTask>(typeid(T), id, data, priority, name));
        }
    };

}  // namespace behaviour
}  // namespace extension

#endif  // EXTENSION_BEHAVIOUR_H

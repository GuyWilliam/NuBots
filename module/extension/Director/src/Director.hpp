/*
 * This file is part of the NUbots Codebase.
 *
 * The NUbots Codebase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The NUbots Codebase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the NUbots Codebase.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2022 NUbots <nubots@nubots.net>
 */

#ifndef MODULE_EXTENSION_DIRECTOR_HPP
#define MODULE_EXTENSION_DIRECTOR_HPP

#include <memory>
#include <mutex>
#include <nuclear>
#include <typeindex>
#include <vector>

#include "provider/ProviderGroup.hpp"

#include "extension/Behaviour.hpp"

namespace module::extension {

    class Director
        : public NUClear::Reactor
        , ::extension::behaviour::information::InformationSource {
    public:
        struct DirectorTask {
            BehaviourTask task;
            bool dying = false;
        };

        /// Behaviour task
        using BehaviourTask = ::extension::behaviour::commands::BehaviourTask;
        /// A task list holds a list of tasks
        using TaskList = std::vector<std::shared_ptr<BehaviourTask>>;
        /// A task pack is the result of a set of tasks emitted by a provider that should be run together
        using TaskPack = std::pair<std::shared_ptr<provider::Provider>, TaskList>;

    private:
        std::recursive_mutex director_mutex{};

        /**
         * Adds a Provider for a type
         *
         * @param p the ProvideReaction object that was generated by the Behaviour extension
         *
         * @throws std::runtime_error when there is more than one Provide in an on statement
         */
        void add_provider(const ::extension::behaviour::commands::ProvideReaction& provide);

        /**
         * Removes a Provider for a type
         *
         * @param id the id of the reaction we want to remove the Provider for
         *
         * @throws std::runtime_error when the reaction does not provide anything
         */
        void remove_provider(const uint64_t& id);

        /**
         * Finds or creates a root provider for a task and returns it
         *
         * @param root_type the root task type
         *
         * @return the root provider
         */
        std::shared_ptr<provider::Provider> get_root_provider(const std::type_index& root_type);

        /**
         * Add a when condition to an existing Provider
         *
         * @param when the description of the When expression from the Behaviour extension
         */
        void add_when(const ::extension::behaviour::commands::WhenExpression& when);

        /**
         * Add a causing condition to an existing Provider
         *
         * @param causing the description of the Causing expression from the behaviour extension
         */
        void add_causing(const ::extension::behaviour::commands::CausingExpression& causing);

        /**
         * Add a needs to an existing Provider
         *
         * @param needs the description of the Needs expression from the behaviour extension
         */
        void add_needs(const ::extension::behaviour::commands::NeedsExpression& needs);

        /**
         * Compares the challenge priorities of two director tasks and returns true if the challenger has priority over
         * the incumbent using challenge priority.
         *
         * i.e. incumbent < challenger
         *
         * The function requires that the challenger's precedence is strictly greater than the incumbent's.
         * This ensures that we don't change tasks unnecessarily when the priority is equal.
         *
         * @param incumbent     the task to compare which is currently the active running task
         * @param challenger    the task to compare which wants to run but is not currently running
         *
         * @return true     if the challenger has strictly higher priority than the incumbent
         * @return false    if the incumbent task has equal or higher priority
         *
         * @throws std::runtime_error if the director's provider ancestry is broken
         */
        [[nodiscard]] bool challenge_priority(const std::shared_ptr<BehaviourTask>& incumbent,
                                              const std::shared_ptr<BehaviourTask>& challenger);

        /**
         * Compares the direct priorities of two director tasks and returns true if the challenger has priority over the
         * incumbent.
         *
         * i.e. incumbent < challenger
         *
         * The function requires that the challenger's precedence is strictly greater than the incumbent's.
         * This ensures that we don't change tasks unnecessarily when the priority is equal.
         *
         * @param incumbent     the task to compare which is currently the active running task
         * @param challenger    the task to compare which wants to run but is not currently running
         *
         * @return true     if the challenger has strictly higher priority than the incumbent
         * @return false    if the incumbent task has equal or higher priority
         *
         * @throws std::runtime_error if the director's provider ancestry is broken
         */
        [[nodiscard]] static bool direct_priority(const std::shared_ptr<BehaviourTask>& incumbent,
                                                  const std::shared_ptr<BehaviourTask>& challenger);

        /**
         * Remove the provided task from the Director.
         *
         * This function also deals with all the consequences of removing the task including looking to see if a queued
         * task can now be run.
         *
         * @param task the task to remove from the Director
         */
        void remove_task(const std::shared_ptr<BehaviourTask>& task);

        /**
         * Reevaluates all of the tasks that are queued to execute on a provider group.
         *
         * Each of the tasks in the queue are waiting to use this provider group but cannot for some reason or another.
         * In order to determine if they can now run on the queue we need to look at each queued task and inspect if the
         * provider that created them can now run. We do this by simply trying to run them all again.
         *
         * @param group the group whose queue we want to reevaluate
         *
         * @return true if as a result of reevaluating we changed tasks
         */
        bool reevaluate_group(provider::ProviderGroup& group);

        /**
         * Runs a reevaluation on not only the passed group, but any groups that it is using
         *
         * It will run reevaluate_group on the passed group, and then if nothing changes in the group it will look at
         * each of the subtasks. If a subtask is currently the active task in another group it will recursively call on
         * that group. This will terminate once there are no more subtasks that are active in other groups or when
         * calling reevaluate_group results in a change of the currently running task.
         *
         * @param group the group whose queue we want to reevaluate along with all children
         */
        void reevaluate_children(provider::ProviderGroup& group);

        /**
         * An object which holds the possible solutions to running a task.
         * Each solution represents how a single task could be executed. As each provider group can have more than one
         * provider, there can be more than one option for how to run the task. For each option that we can choose, they
         * can themselves have their own requirements for provider groups they need to execute. Therefore each of these
         * requirements is themselves a solution to another task that could be emitted.
         */
        struct Solution {

            /**
             * A single option for how a provider could be executed
             */
            struct Option {
                /// The state of this provider if it is blocked along with a description of why it is blocked
                enum State {
                    /// This option is not blocked to us
                    OK,
                    /// This provider is blocked as the task given didn't have enough authority to run it
                    BLOCKED_PRIORITY,
                    /// This provider is blocked due to a when condition that cannot be met
                    BLOCKED_WHEN,
                    /// This provider is blocked as executing it would cause a loop of providers
                    BLOCKED_LOOP,
                };

                /// The provider we are executing in this
                std::shared_ptr<provider::Provider> provider;
                /// The set of solutions needed for this option to be executed
                std::vector<Solution> requirements;
                /// The state of this option, holds if it is blocked and if so why
                State state;
            };

            /// This value is true if this solution is a pushing solution. Meaning that the main solution cannot run but
            /// running solution can enable a when condition to be met
            bool pushed;
            /// The list of options for this solution
            std::vector<Option> options;
        };

        /**
         * Return the passed provider as an option for a solution along with its requirements.
         * It will recursively find Solutions for each of the requirements of the provider including its unmet when
         * conditions and needs relationships.
         *
         * @param provider  the provider that we are trying to find a solution for
         * @param authority the task that we are using as our authority token for permission checks
         * @param visited   the set of provider groups that have already been visited to prevent loops
         *
         * @return
         */
        Solution::Option solve_provider(const std::shared_ptr<provider::Provider>& provider,
                                        const std::shared_ptr<BehaviourTask>& authority,
                                        std::set<std::type_index> visited);

        /**
         * Finds the providers that can cause the passed when condition to be met.
         * It will then recursively find Solutions for each of the requirements of the provider potentially including
         * more when conditions that need to be met. The final solution for a task could end up with several steps
         * removed from the original task.
         *
         * @param when      the when condition we want to meet by finding causings that will allow a provider to run
         * @param authority the task that we are using as our authority token for permission checks
         * @param visited   the set of provider groups that have already been visited to prevent loops
         *
         * @return         the set of providers that when run can meet the provided when condition
         */
        Solution solve_when(const provider::Provider::WhenCondition& when,
                            const std::shared_ptr<BehaviourTask>& authority,
                            const std::set<std::type_index>& visited);

        /**
         * Creates options for each provider in a provider group specified by the passed type.
         * This function will check each provider in the group and make an option for it.
         *
         * @param type      the type of task we are trying to run
         * @param authority the task that we are using as our authority token for permission checks
         * @param visited   the set of provider groups that have already been visited to prevent loops
         *
         * @return the set of possible solution options for the provider group of the passed type
         */
        Solution solve_group(const std::type_index& type,
                             const std::shared_ptr<BehaviourTask>& authority,
                             const std::set<std::type_index>& visited);

        /**
         * Finds a solution for a given task. It will start with the provider group for the given task and recursively
         * work its way down to find all possible solutions. It will use the passed task as an authority token for the
         * permission checks on providers that it needs to run.
         *
         * @param task      the task we are finding solutions for, also used for authority checks
         *
         * @return the set of possible solution options for this task
         */
        Solution solve_task(const std::shared_ptr<BehaviourTask>& task);

        /**
         * Represents a solution that we can run now, i.e. we can run all of the providers in this solution.
         *
         * Or if there is no solution that can be run now, it will return that it is blocked and which groups we wanted
         * to run but weren't able to.
         */
        struct OkSolution {
            OkSolution(const bool& blocked_, std::set<std::type_index>&& blocking_groups_)
                : blocked(blocked_), blocking_groups(blocking_groups_) {}
            OkSolution(const bool& blocked_,
                       const std::shared_ptr<provider::Provider>& provider_,
                       std::vector<std::shared_ptr<provider::Provider>>&& requirements_,
                       std::set<std::type_index>&& used_,
                       std::set<std::type_index>&& blocking_groups_)
                : blocked(blocked_)
                , provider(provider_)
                , requirements(requirements_)
                , used(used_)
                , blocking_groups(blocking_groups_) {}

            /// If this entire path is blocked and unusable
            bool blocked;
            /// The main provider that this solution is for
            std::shared_ptr<provider::Provider> provider;
            /// The list of providers that are needed for each of the requirements
            std::vector<std::shared_ptr<provider::Provider>> requirements;
            /// Which groups have been used in this solution
            std::set<std::type_index> used;
            /// Groups which we wanted to use but were blocked to us
            std::set<std::type_index> blocking_groups;
        };

        /**
         * Joins all of the requirements of an option into a single OkSolution instance.
         *
         * @param option        the option we are joining the requirements of
         * @param used_types    the set of provider groups that have been used in this solution
         *
         * @return the OkSolution that represents the requirements of the passed option fused together
         */
        OkSolution find_ok_solution(const Solution::Option& option, std::set<std::type_index> used_types);

        /**
         * Finds the first available option for a solution that we can run now for this solution. Or if there is no
         * option that can be run now, it will return that it is blocked and which groups we wanted to run but wasn't
         * able to.
         *
         * @param requirement the requirement that we are searching for options we can execute
         * @param used_types  types that have already been used higher in the solution tree that are blocked to us
         *
         * @return An OKSolution instance that represents a solution we can run now or a blocked solution
         */
        OkSolution find_ok_solution(const Solution& requirement, const std::set<std::type_index>& used_types);

        /**
         * Solves each of a series of Solutions and returns a list of OkSolutions that represents the first solution
         * that can execute, or a blocked solution if none of the solutions can execute.
         *
         * @param solutions the set of solutions that we are trying to find an OkSolution for
         *
         * @return a list of OkSolutions that represents the first solution that can execute, or a blocked solution if
         *         none of the solutions can execute.
         */
        std::vector<OkSolution> find_ok_solutions(const std::vector<Solution>& solutions);

        /**
         * Runs the passed task on the passed provider.
         *
         * @param task          the task we are running
         * @param provider      the provider that we are running the task on
         * @param run_reason    the reason that we are running this task
         */
        void run_task_on_provider(const std::shared_ptr<BehaviourTask>& task,
                                  const std::shared_ptr<provider::Provider>& provider,
                                  const ::extension::behaviour::RunInfo::RunReason& run_reason);

        /**
         * The level of solution that we can run at
         */
        enum RunLevel {
            /// We can run all our tasks on providers now
            OK,
            /// We can't run all our tasks on providers now, but we can push to fix our requirements
            PUSH,
            /// We can't run all our tasks on providers now, and we can't push to fix our requirements
            BLOCKED
        };

        /**
         * Tries to execute tasks in the pack, but only up to the passed run level.
         *
         * The passed run level will limit what types of execution are open to us. For example, if the required pack was
         * only able to execute up to the PUSH level, then we will only execute up to the PUSH level for the optional
         * packs that come afterward.
         *
         * @param group     the provider group that created this pack
         * @param pack      the pack of tasks that we are trying to execute
         * @param run_level the level of execution that we are allowed to do
         *
         * @return the run level that we were able to execute up to
         */
        RunLevel run_tasks(provider::ProviderGroup& group, const TaskList& pack, const RunLevel& run_level);

        /**
         * Looks at all the tasks that are in the pack and determines if they should run, and if so runs them.
         *
         * This function will ensure that if this pack is to be executed it can be executed as a whole. That means that
         * all of the non optional tasks in it are able to run now given the state of the system. If the system is
         * unable to run due to priority or due to a when condition not met it will not run any of the tasks in this
         * pack, but enqueue them on the relevant providers.
         *
         * In the event that `When` conditions needs to be met by this pack and it has sufficient priority, it may also
         * place this provider group into a "Pushing" state whereby it forces another provider group into a state where
         * it will make the system reach the causing it requires.
         *
         * @param pack the task pack that represents the queued tasks
         */
        void run_task_pack(const TaskPack& pack);

        /**
         * A RunReasonLock instance will reset the current_run_reason back to OTHER_TRIGGER when it is destroyed.
         *
         * This is used so that the run reason can temporarily be set to something else, and then reset back to the
         * default if there is an exception or other error.
         */
        using RunReasonLock = std::unique_ptr<void, std::function<void(void*)>>;

        /**
         * Holds a run reason as the current run reason for the current thread.
         *
         * It returns a lock object that acts as an RAII object that will reset the current run reason when it is
         * destroyed.
         *
         * @param reason the reason to hold until the lock is destroyed
         *
         * @return a lock object that will reset `current_run_reason` to its default when destroyed
         */
        RunReasonLock hold_run_reason(const ::extension::behaviour::RunInfo::RunReason& reason);

    public:
        /// Called by the powerplant to build and setup the Director reactor.
        explicit Director(std::unique_ptr<NUClear::Environment> environment);
        virtual ~Director();

        /**
         * Provides the task data via the InformationSource interface so it can be accessed
         *
         * @param reaction_id the provider reaction that is requesting its information.
         *
         * @return the data that is stored in this reaction, or nullptr if it shouldn't be executing
         */
        std::shared_ptr<void> _get_task_data(const uint64_t& reaction_id) override;

        /**
         * Provides the RunInfo data via the InformationSource interface so it can be accessed
         *
         * @param reaction_id the provider reaction that is requesting its information.
         *
         * @return the information about why this task has been executed
         */
        ::extension::behaviour::RunInfo _get_run_info(const uint64_t& reaction_id) override;

        /**
         * Provides the ProviderGroup data via the InformationSource interface so it can be accessed
         *
         * @param reaction_id the provider reaction that is requesting its information.
         * @param type        the type of provider group that is being requested
         * @param root_type   the secondary type to use if this is a root task
         *
         * @return the information about the provider group that this task is running on
         */
        ::extension::behaviour::GroupInfo _get_group_info(const uint64_t& reaction_id,
                                                          const std::type_index& type,
                                                          const std::type_index& root_type) override;

    private:
        /// A list of Provider groups
        std::map<std::type_index, provider::ProviderGroup> groups;
        /// Maps reaction_id to the Provider which implements it
        std::map<uint64_t, std::shared_ptr<provider::Provider>> providers;

        /// A source for unique reaction ids when making root task providers. Starts at 0 and wraps around to maxvalue.
        uint64_t unique_id_source = 0;

        /// The current run reason this thread is executing for. Defaults to OTHER_TRIGGER as that will be what it
        /// is if a non Director execution occurs
        thread_local static ::extension::behaviour::RunInfo::RunReason current_run_reason;

        /// A list of reaction_task_ids to director_task objects, once the Provider has finished running it will emit
        /// all these as a pack so that the director can work out when Providers change which subtasks they emit
        std::multimap<uint64_t, std::shared_ptr<BehaviourTask>> pack_builder;

    public:
        friend class InformationSource;
    };

}  // namespace module::extension

#endif  // MODULE_EXTENSION_DIRECTOR_HPP

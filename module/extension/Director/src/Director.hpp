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
 * Copyright 2021 NUbots <nubots@nubots.net>
 */
#ifndef MODULE_EXTENSION_DIRECTOR_HPP
#define MODULE_EXTENSION_DIRECTOR_HPP

#include <memory>
#include <nuclear>
#include <typeindex>
#include <vector>

#include "provider/ProviderGroup.hpp"

#include "extension/Behaviour.hpp"

namespace module::extension {

    class Director : public NUClear::Reactor {
    public:
        /// A task queue holds tasks in a provider that are waiting to be executed by that group
        using TaskQueue = std::vector<std::shared_ptr<const ::extension::behaviour::commands::BehaviourTask>>;
        /// A task pack is the result of a set of tasks emitted by a provider that should be run together
        using TaskPack = std::vector<std::shared_ptr<const ::extension::behaviour::commands::BehaviourTask>>;

    private:
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
         * Compares the priorities of two director tasks and returns true if the challenger has priority over the
         * incumbent.
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
        [[nodiscard]] bool challenge_priority(
            const std::shared_ptr<const ::extension::behaviour::commands::BehaviourTask>& incumbent,
            const std::shared_ptr<const ::extension::behaviour::commands::BehaviourTask>& challenger);

        /**
         * Remove the provided task from the Director.
         *
         * This function also deals with all the consequences of removing the task including looking to see if a queued
         * task can now be run.
         *
         * @param task the task to remove from the Director
         */
        void remove_task(const std::shared_ptr<const ::extension::behaviour::commands::BehaviourTask>& task);

        /**
         * Reevaluates all of the tasks that are queued to execute on a provider group.
         *
         * Each of the tasks in the queue are waiting to use this provider group but cannot for some reason or another.
         * In order to determine if they can now run on the queue we need to look at each queued task and inspect if the
         * provider that created them can now run. We do this by simply trying to run them all again.
         *
         * @param group the group whose queue we want to reevaluate
         */
        void reevaluate_queue(provider::ProviderGroup& group);

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
                std::shared_ptr<const provider::Provider> provider;
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
         * @param visited   the set of providers that have already been visited to prevent loops
         *
         * @return
         */
        Solution::Option solve_provider(
            const std::shared_ptr<provider::Provider>& provider,
            const std::shared_ptr<const ::extension::behaviour::commands::BehaviourTask>& authority,
            std::set<std::shared_ptr<const provider::Provider>> visited);

        /**
         * Finds the providers that can cause the passed when condition to be met.
         * It will then recursively find Solutions for each of the requirements of the provider potentially including
         * more when conditions that need to be met. The final solution for a task could end up with several steps
         * removed from the original task.
         *
         * @param when      the when condition we want to meet by finding causings that will allow a provider to run
         * @param authority the task that we are using as our authority token for permission checks
         * @param visited   the set of providers that have already been visited to prevent loops
         *
         * @return         the set of providers that when run can meet the provided when condition
         */
        Solution solve_when(const provider::Provider::WhenCondition& when,
                            const std::shared_ptr<const ::extension::behaviour::commands::BehaviourTask>& authority,
                            const std::set<std::shared_ptr<const provider::Provider>>& visited);

        /**
         * Creates options for each provider in a provider group specified by the passed type.
         * This function will check each provider in the group and make an option for it.
         *
         * @param type      the type of task we are trying to run
         * @param authority the task that we are using as our authority token for permission checks
         * @param visited   the set of providers that have already been visited to prevent loops
         *
         * @return the set of possible solution options for the provider group of the passed type
         */
        Solution solve_group(const std::type_index& type,
                             const std::shared_ptr<const ::extension::behaviour::commands::BehaviourTask>& authority,
                             const std::set<std::shared_ptr<const provider::Provider>>& visited);

        /**
         * Finds a solution for a given task. It will start with the provider group for the given task and recursively
         * work its way down to find all possible solutions. It will use the passed task as an authority token for the
         * permission checks on providers that it needs to run.
         *
         * @param task      the task we are finding solutions for
         * @param authority the task that we are using as our authority token for permission checks
         *
         * @return the set of possible solution options for this task
         */
        Solution solve_task(const std::shared_ptr<const ::extension::behaviour::commands::DirectorTask>& task,
                            const std::shared_ptr<const ::extension::behaviour::commands::DirectorTask>& authority);

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

    public:
        /// Called by the powerplant to build and setup the Director reactor.
        explicit Director(std::unique_ptr<NUClear::Environment> environment);

    private:
        /// A list of Provider groups
        std::map<std::type_index, provider::ProviderGroup> groups;
        /// Maps reaction_id to the Provider which implements it
        std::map<uint64_t, std::shared_ptr<provider::Provider>> providers;

        /// A list of reaction_task_ids to director_task objects, once the Provider has finished running it will emit
        /// all these as a pack so that the director can work out when Providers change which subtasks they emit
        std::multimap<uint64_t, std::shared_ptr<const ::extension::behaviour::commands::BehaviourTask>> pack_builder;
    };

}  // namespace module::extension

#endif  // MODULE_EXTENSION_DIRECTOR_HPP

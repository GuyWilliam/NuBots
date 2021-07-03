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

#include <nuclear>
#include <typeindex>

#include "director/ProviderGroup.hpp"

#include "extension/Behaviour.hpp"

namespace module::extension {

    class Director : public NUClear::Reactor {
    private:
        /**
         * Adds a Provider for a type
         *
         * @param p the ProvidesReaction object that was generated by the Behavour extension
         *
         * @throws std::runtime_error when there is more than one Provides in an on statement
         */
        void add_provider(const ::extension::behaviour::commands::ProvidesReaction& provides);

        /**
         * Removes a Provider for a type
         *
         * @param id the id of the reaction we want to remove Providers for
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

    public:
        /// Called by the powerplant to build and setup the Director reactor.
        explicit Director(std::unique_ptr<NUClear::Environment> environment);

    private:
        /// A list of Provider groups
        std::map<std::type_index, ProviderGroup> groups;
        /// Maps reaction_id to the Provider which implements it
        std::map<uint64_t, std::shared_ptr<Provider>> providers;

        /// A list of reaction_task_ids to director_task objects, once the Provider has finished running it will emit
        /// all these as a pack so that the director can work out when Providers change which subtasks they emit
        std::multimap<uint64_t, std::shared_ptr<const ::extension::behaviour::commands::DirectorTask>> pack_builder;
    };

}  // namespace module::extension

#endif  // MODULE_EXTENSION_DIRECTOR_HPP

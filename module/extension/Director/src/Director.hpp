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
         * Adds a provider for a type
         *
         * @param data_type The type_index of the data type to provide for
         * @param r The reaction that will be run to provide
         * @throws std::runtime_error when there is more than one Provides in an on statement
         */
        void add_provider(const ::extension::behaviour::commands::ProvidesReaction& p);

        /**
         * Removes a provider for a type
         *
         * @param id the id of the reaction we want to remove providers for
         * @throws std::runtime_error when the reaction does not provide anything
         */
        void remove_provider(const uint64_t& id);

        /**
         * Add a when condition to an existing provider
         *
         */
        void add_when(const ::extension::behaviour::commands::WhenExpression& when);

        /**
         * Add a causing condition to an existing provider
         *
         */
        void add_causing(const ::extension::behaviour::commands::CausingExpression& causing);

    public:
        /// Called by the powerplant to build and setup the Director reactor.
        explicit Director(std::unique_ptr<NUClear::Environment> environment);

    private:
        /// A list of provider groups
        std::map<std::type_index, ProviderGroup> groups;
        /// Maps reaction_id to the provider which implements it
        std::map<uint64_t, std::shared_ptr<Provider>> providers;

        /// A list of reaction_task_ids to director_task objects, once the provider has finished running it will emit
        /// all these as a pack so that the director can work out when providers change which subtasks they emit
        std::multimap<uint64_t, std::shared_ptr<const ::extension::behaviour::commands::DirectorTask>> pack_builder;
    };

}  // namespace module::extension

#endif  // MODULE_EXTENSION_DIRECTOR_HPP

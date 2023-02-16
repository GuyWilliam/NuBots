#include "PlanLook.hpp"

#include "extension/Behaviour.hpp"
#include "extension/Configuration.hpp"
#include "extension/behaviour/Script.hpp"

#include "message/actuation/Limbs.hpp"
#include "message/input/Sensors.hpp"
#include "message/localisation/FilteredBall.hpp"
#include "message/planning/LookForFeatures.hpp"
#include "message/skill/Look.hpp"
#include "message/vision/Goal.hpp"

#include "utility/math/coordinates.hpp"
#include "utility/support/yaml_expression.hpp"

namespace module::planning {

    using extension::Configuration;
    using extension::behaviour::Script;
    using extension::behaviour::ScriptRequest;
    using message::actuation::LimbsSequence;
    using message::input::Sensors;
    using message::localisation::FilteredBall;
    using message::planning::LookForFeatures;
    using message::planning::LookSearch;
    using message::skill::Look;
    using message::vision::Goals;
    using utility::math::coordinates::sphericalToCartesian;
    using utility::support::Expression;

    PlanLook::PlanLook(std::unique_ptr<NUClear::Environment> environment) : BehaviourReactor(std::move(environment)) {

        on<Configuration>("PlanLook.yaml").then([this](const Configuration& config) {
            // Use configuration here from file PlanLook.yaml
            this->log_level = config["log_level"].as<NUClear::LogLevel>();

            cfg.ball_search_timeout = duration_cast<NUClear::clock::duration>(
                std::chrono::duration<double>(config["ball_search_timeout"].as<double>()));
            cfg.goal_search_timeout = duration_cast<NUClear::clock::duration>(
                std::chrono::duration<double>(config["goal_search_timeout"].as<double>()));
            cfg.field_search_timeout = duration_cast<NUClear::clock::duration>(
                std::chrono::duration<double>(config["field_search_timeout"].as<double>()));
            cfg.search_fixation_time = config["search_fixation_time"].as<float>();

            // Create vector of search positions
            for (const auto& search_position : config["search_positions"].config) {
                cfg.search_positions.push_back(search_position.as<Expression>());
            }
        });

        on<Startup>().then([this] {
            // emit<Script>(std::make_unique<BodySequence>(), ScriptRequest("Stand.yaml"));
            emit<Task>(std::make_unique<LookForFeatures>(true, false, true));
        });

        on<Provide<LookForFeatures>,
           Uses<LookSearch>,
           Optional<With<FilteredBall>>,
           Optional<With<Goals>>,
           With<Sensors>,
           Every<90, Per<std::chrono::seconds>>>()
            .then([this](const LookForFeatures& look_for_features,
                         const Uses<LookSearch>& look_search,
                         const std::shared_ptr<const FilteredBall>& ball,
                         const std::shared_ptr<const Goals>& goals,
                         const Sensors& sensors) {
                // If a search pattern has completed, set the last time we searched the field to now
                if (look_search.done) {
                    last_field_search = NUClear::clock::now();
                }

                // SEARCH ALGORITHM
                // if we have seen no ball or goals, search
                // if we haven't seen the ball or goals in a while, search
                // if it's been too long since a field search, search
                // if we're not looking for anything in particular, search
                // otherwise we've seen balls and goals and field, just keep looking at the ball
                // but if we don't care about the ball, look at the goals
                // but if we don't care about ball and goals, then just search

                // Do we need to look for the ball? if we want to look for the ball and haven't seen it in a while
                bool search_ball =
                    look_for_features.ball
                    && (ball == nullptr
                        || (ball && NUClear::clock::now() - ball->time_of_measurement > cfg.ball_search_timeout));

                // Do we need to look for the goals? if we want to look for goals and we haven't seen them in a while
                bool search_goals = look_for_features.goals
                                    && (goals == nullptr
                                        || (goals && !goals->goals.empty()
                                            && NUClear::clock::now() - goals->timestamp > cfg.goal_search_timeout));

                // Do we need to look at the field? if we want to look over the field and it's been long enough since a
                // full search of the field was conducted
                bool search_field =
                    look_for_features.field && (NUClear::clock::now() - last_field_search > cfg.field_search_timeout);

                // If we're not looking for anything in particular, just look around in general
                bool search_anything = !look_for_features.ball && !look_for_features.goals;

                // Log what we're searching for
                log<NUClear::TRACE>("Searching. Ball",
                                    search_ball,
                                    ": goals",
                                    search_goals,
                                    ": field",
                                    search_field,
                                    ": any",
                                    search_anything);

                // Either we need to search for the ball, goals, field or we're not looking for anything in particular
                // so we search around
                if (search_ball || search_goals || search_field || search_anything) {
                    emit<Task>(std::make_unique<LookSearch>());
                }
                else if (look_for_features.ball && ball) {  // keep watching the ball if we aren't searching
                    emit<Task>(std::make_unique<Look>(ball->rBCt.cast<double>(), true));
                }
                else if (look_for_features.goals && goals) {  // if we don't want the ball, then watch the goals
                    // Convert goal measurement to cartesian coordinates
                    Eigen::Vector3d rGCc = sphericalToCartesian(goals->goals[0].measurements[0].srGCc.cast<double>());
                    // Convert to torso space
                    Eigen::Vector3d rGCt = Eigen::Isometry3d(sensors.Htw * goals->Hcw.inverse()).rotation() * rGCc;
                    emit<Task>(std::make_unique<Look>(rGCt, true));
                }
                else {  // if there isn't anything to focus on, just search
                    emit<Task>(std::make_unique<LookSearch>());
                }
            });

        on<Provide<LookSearch>>().then([this] {
            // Reset the search position index if at end of list
            // And tell the parent that a full search has been completed
            if (search_idx == cfg.search_positions.size()) {
                search_idx = 0;
                emit<Task>(std::make_unique<Done>());
                return;
            }

            // How long the look has lingered - will move to the next position if long enough
            float time_since_last_search_moved =
                std::chrono::duration_cast<std::chrono::duration<float>>(NUClear::clock::now() - search_last_moved)
                    .count();

            // Robot will move through the search positions, and linger for fixation_time. Once
            // fixation_time time has passed, send a new head command for the next position in the list
            // of cfg.search_positions
            if (time_since_last_search_moved > cfg.search_fixation_time) {
                // Send command for look position
                double yaw   = cfg.search_positions[search_idx][0];
                double pitch = cfg.search_positions[search_idx][1];

                // Make a vector pointing straight forwards and rotate it by the pitch and yaw
                // Pitch is negative since down is positive in servo space
                Eigen::Vector3d uPCt = (Eigen::AngleAxisd(yaw, Eigen::Vector3d::UnitZ())
                                        * Eigen::AngleAxisd(-pitch, Eigen::Vector3d::UnitY()))
                                           .toRotationMatrix()
                                       * Eigen::Vector3d::UnitX();

                emit<Task>(std::make_unique<Look>(uPCt, false));

                // Move to next search position in list
                search_last_moved = NUClear::clock::now();
                search_idx++;
            }
        });

        // When starting a LookSearch, start from the first look position
        on<Start<LookSearch>>().then([this] { search_idx = 0; });
    }

}  // namespace module::planning

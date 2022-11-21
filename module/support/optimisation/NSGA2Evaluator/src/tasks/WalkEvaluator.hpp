#ifndef MODULE_SUPPORT_OPTIMISATION_WALKEVALUATOR_H
#define MODULE_SUPPORT_OPTIMISATION_WALKEVALUATOR_H

#include <Eigen/Core>
#include <nuclear>
#include <vector>

#include "EvaluatorTask.hpp"

#include "message/platform/RawSensors.hpp"
#include "message/support/optimisation/NSGA2Evaluator.hpp"
#include "message/support/optimisation/NSGA2Optimiser.hpp"

namespace module {
    namespace support {
        namespace optimisation {
            using message::platform::RawSensors;
            using message::support::optimisation::NSGA2EvaluationRequest;
            using message::support::optimisation::NSGA2FitnessScores;

            class WalkEvaluator : public EvaluatorTask {
            public:
                // Implementing the EvaluatorTask interface
                void processRawSensorMsg(const RawSensors& sensors, NSGA2Evaluator* evaluator);
                void processOptimisationRobotPosition(const OptimisationRobotPosition& position);
                void setUpTrial(const NSGA2EvaluationRequest& request);
                void resetSimulation();
                void evaluatingState(size_t subsumptionId, NSGA2Evaluator* evaluator);
                std::unique_ptr<NSGA2FitnessScores> calculateFitnessScores(bool earlyTermination,
                                                                           double sim_time,
                                                                           int generation,
                                                                           int individual);

                // Task-specific functions
                std::vector<double> calculateScores();
                std::vector<double> calculateConstraints(double sim_time);
                std::vector<double> constraintsNotViolated();
                bool checkForFall(const RawSensors& sensors);
                void updateMaxFieldPlaneSway(const RawSensors& sensors);
                bool checkOffCourse();

            private:
                /// @brief Robot state for this evaluation, used during fitness and constraint calculation
                bool initial_position_set              = false;
                Eigen::Vector3d initial_robot_position = Eigen::Vector3d::Zero();
                Eigen::Vector3d robot_position         = Eigen::Vector3d::Zero();
                double max_field_plane_sway            = 0.0;

                /// @brief The amount of time to run a single trial, in seconds.
                std::chrono::seconds trial_duration_limit = std::chrono::seconds(0);

                /// @brief Keep track of when the trial started
                double trial_start_time = 0.0;

                /// @brief The walk command velocity.
                Eigen::Vector2d walk_command_velocity = Eigen::Vector2d(0.0, 0.0);

                /// @brief The walk command rotation.
                double walk_command_rotation = 0.0;

                /// @brief Configuration Min and Max values
                float gravity_Max = 0.0;
                float gravity_Min = 0.0;
            };

        }  // namespace optimisation
    }      // namespace support
}  // namespace module

#endif  // MODULE_SUPPORT_OPTIMISATION_WALKEVALUATOR_H

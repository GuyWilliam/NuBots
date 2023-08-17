#ifndef MODULE_SUPPORT_OPTIMISATION_STANDEVALUATOR_HPP
#define MODULE_SUPPORT_OPTIMISATION_STANDEVALUATOR_HPP

#include <Eigen/Core>
#include <nuclear>
#include <vector>

#include "EvaluatorTask.hpp"

#include "extension/Script.hpp"

#include "message/platform/RawSensors.hpp"
#include "message/input/Sensors.hpp"
#include "message/support/optimisation/NSGA2Evaluator.hpp"
#include "message/support/optimisation/NSGA2Optimiser.hpp"

namespace module::support::optimisation {
    using message::platform::RawSensors;
    using message::input::Sensors;
    using message::support::optimisation::NSGA2EvaluationRequest;
    using message::support::optimisation::NSGA2FitnessScores;

    class StandEvaluator : public EvaluatorTask {
    public:
        // Implementing the EvaluatorTask interface
        void process_optimisation_robot_position(const OptimisationRobotPosition& position);
        void set_up_trial(const NSGA2EvaluationRequest& request);
        void reset_simulation();
        void evaluating_state(size_t subsumption_id, NSGA2Evaluator* evaluator);
        std::unique_ptr<NSGA2FitnessScores> calculate_fitness_scores(bool early_termination,
                                                                     double sim_time,
                                                                     int generation,
                                                                     int individual);

        // Task-specific functions
        std::vector<double> calculate_scores(double trialDuration);
        std::vector<double> calculate_constraints(bool early_termination);
        bool check_for_fall(const Sensors& sensors);
        bool has_fallen(const Sensors& sensors);
        void update_max_field_plane_sway(const Sensors& sensors);

    private:
        /// @brief Robot state for this evaluation, used during fitness and constraint calculation
        Eigen::Vector3d robot_position = Eigen::Vector3d::Zero();
        double max_field_plane_sway    = 0.0;
        RawSensors current_sensors;

        float fallen_angle = 0.0;

        /// @brief The amount of time to run a single trial, in seconds.
        std::chrono::seconds trial_duration_limit = std::chrono::seconds(0);

        /// @brief Keep track of when the trial started
        double trial_start_time = 0.0;

        void load_script(std::string script_path);
        void save_script(std::string script_path);
        void run_script(size_t subsumption_id, NSGA2Evaluator* evaluator);

        /// @brief The script object we are using
        ::extension::Script script;
    };

}  // namespace module::support::optimisation

#endif  // MODULE_SUPPORT_OPTIMISATION_STANDEVALUATOR_HPP

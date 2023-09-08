#ifndef MODULE_SUPPORT_OPTIMISATION_EVALUATORTASK_HPP
#define MODULE_SUPPORT_OPTIMISATION_EVALUATORTASK_HPP

#include <nuclear>
#include <vector>

#include "NSGA2Evaluator.hpp"

#include "extension/Behaviour.hpp"

#include "message/input/Sensors.hpp"
#include "message/platform/RawSensors.hpp"
#include "message/platform/webots/messages.hpp"
#include "message/support/optimisation/NSGA2Evaluator.hpp"
#include "message/support/optimisation/NSGA2Optimiser.hpp"

namespace module::support::optimisation {
    using message::input::Sensors;
    using message::platform::RawSensors;
    using message::platform::webots::OptimisationRobotPosition;
    using message::support::optimisation::NSGA2EvaluationRequest;
    using message::support::optimisation::NSGA2FitnessScores;

    using ::extension::behaviour::Task;

    class NSGA2Evaluator;  // forward declaration

    class EvaluatorTask {
    public:
        virtual bool has_fallen(const Sensors& sensors)                                             = 0;
        virtual void process_optimisation_robot_position(const OptimisationRobotPosition& position) = 0;
        virtual void set_up_trial(const NSGA2EvaluationRequest& request)                            = 0;
        virtual void reset_simulation()                                                             = 0;
        virtual void evaluating_state(NSGA2Evaluator* evaluator)                                    = 0;
        virtual std::unique_ptr<NSGA2FitnessScores> calculate_fitness_scores(bool early_termination,
                                                                             double simTime,
                                                                             int generation,
                                                                             int individual)        = 0;
    };

}  // namespace module::support::optimisation

#endif  // MODULE_SUPPORT_OPTIMISATION_EVALUATORTASK_HPP

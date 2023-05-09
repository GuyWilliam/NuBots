#ifndef MODULE_SUPPORT_OPTIMISATION_STANDOPTIMISER_H
#define MODULE_SUPPORT_OPTIMISATION_STANDOPTIMISER_H

#include <nuclear>
#include <vector>

#include "OptimiserTask.hpp"
#include "nsga2/NSGA2.hpp"

#include "extension/Configuration.hpp"

#include "message/support/optimisation/NSGA2Evaluator.hpp"
#include "message/support/optimisation/NSGA2Optimiser.hpp"

namespace module::support::optimisation {
    using message::support::optimisation::NSGA2EvaluationRequest;

    class StandOptimiser : public OptimiserTask {
    public:
        void SetupNSGA2(const ::extension::Configuration& config, nsga2::NSGA2& nsga2_algorithm);
        std::unique_ptr<NSGA2EvaluationRequest> MakeEvaluationRequest(const int id,
                                                                      const int generation,
                                                                      std::vector<double> reals);

    private:
        std::string script_path;
        int trial_duration_limit;
    };

}  // namespace module::support::optimisation

#endif  // MODULE_SUPPORT_OPTIMISATION_STANDOPTIMISER_H

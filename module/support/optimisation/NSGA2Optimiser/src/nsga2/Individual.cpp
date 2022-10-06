#include "Individual.hpp"

#include <cmath>
#include <fmt/format.h>
#include <nuclear>

namespace nsga2 {
    Individual::Individual(const IndividualConfigurator& _config) : config(_config) {
        id              = 0;
        rank            = 0;
        constrViolation = 0.0;
        crowdDist       = 0.0;
        evaluated       = false;
        generation      = -1;
        reals.clear();
        gene.clear();
        bins.clear();
        objScore.clear();
        constr.clear();

        reals.resize(config.realVars, 0);
        bins.resize(config.binVars, 0);
        gene.resize(config.binVars);

        if (int(config.binBits.size()) != config.binVars) {
            NUClear::log<NUClear::WARN>(
                fmt::format("binBits size ({}) != binVars ({})", config.binBits.size(), config.binVars));
        }

        for (int j = 0; j < config.binVars; j++) {
            gene[j].resize(config.binBits[j], 0);
        }
        objScore.resize(config.objectives, 0);
        constr.resize(config.constraints, 0);
    }

    void Individual::Initialize(const int& _id) {  // My entry point to init my parent pop to the one I want
        id = _id;
        NUClear::log<NUClear::INFO>("ID", id);

        if (id == 0) {
            // First individual gets real vars from config
            // initialise real vars
            for (int i = 0; i < config.realVars; i++) {
                reals[i] = config.initialRealVars[i];
            }
        }
        else {
            // initialise real vars
            for (int i = 0; i < config.realVars; i++) {
                reals[i] = config.randGen->Real(config.realLimits[i].first, config.realLimits[i].second);
            }
        }
        // initialise bin vars
        for (int i = 0; i < config.binVars; i++) {
            for (int j = 0; j < config.binBits[i]; j++) {
                gene[i][j] = config.randGen->Realu() <= 0.5 ? 0 : 1;
            }
        }

        for (auto r : reals) {
            NUClear::log<NUClear::INFO>("Init Vars", r);
        }
    }

    void Individual::Decode() {
        int sum;
        for (int i = 0; i < config.binVars; i++) {
            sum = 0;
            for (int j = 0; j < config.binBits[i]; j++) {
                sum += (1 << (config.binBits[i] - 1 - j));
            }

            bins[i] = config.binLimits[i].first
                      + (double) sum * (config.binLimits[i].second - config.binLimits[i].first)
                            / (double) ((1 << (config.binBits[i])) - 1);
        }
    }

    void Individual::CheckConstraints() {
        if (config.constraints) {
            constrViolation = 0.0;
            for (int i = 0; i < config.constraints; i++) {
                if (constr[i] < 0.0)
                    constrViolation += constr[i];
            }
        }
        else
            constrViolation = 0.0;

        evaluated = true;
    }

    // returns:  1 if this < _b (this dominates _b),
    //          -1 if this > _b (this is dominated by _b),
    //           0 if they are nondominated
    int Individual::CheckDominance(const Individual& _b) const {
        if (constrViolation < 0.0 && _b.constrViolation < 0.0) {
            // both have constraint violations
            if (constrViolation > _b.constrViolation)
                return 1;  // this violates less
            else if (constrViolation < _b.constrViolation)
                return -1;  // _b violates less
            else
                return 0;  // they both violate equally
        }
        else if (constrViolation < 0 && _b.constrViolation == 0) {
            // this violates and _b doesn't => _b dominates
            return -1;
        }
        else if (constrViolation == 0 && _b.constrViolation < 0) {
            // this doesn't violate and _b does => this dominates
            return 1;
        }
        else {
            // no constraint violations
            int thisDoms = 0;  // to check if this has a smaller objective
            int thatDoms = 0;  // to check if _b    has a smaller objective

            for (int i = 0; i < config.objectives; i++) {
                if (config.objectives > 1) {
                    // Normal multi objective comparison
                    if (objScore[i] < _b.objScore[i])
                        thisDoms = 1;
                    else if (objScore[i] > _b.objScore[i])
                        thatDoms = 1;
                }
                else {
                    // mono objective comparison with an epsilon
                    double objFabs = std::fabs(objScore[i] - _b.objScore[i]);
                    if (objScore[i] < _b.objScore[i] && objFabs > config.epsC)
                        thisDoms = 1;
                    else if (objScore[i] > _b.objScore[i] && objFabs > config.epsC)
                        thatDoms = 1;
                }
            }

            // there is at least one smaller objective for this and none for _b
            if (thisDoms == 1 && thatDoms == 0)
                return 1;
            // there is at least one smaller objective for _b and none for this
            else if (thisDoms == 0 && thatDoms == 1)
                return -1;
            // no smaller objective or both have one smaller
            else
                return 0;
        }
    }

    std::pair<int, int> Individual::Mutate() {
        std::pair<int, int> mutationCount = std::make_pair(0, 0);
        if (config.realVars) {
            mutationCount.first += realMutate();
        }
        if (config.binVars) {
            mutationCount.second += binMutate();
        }
        return mutationCount;
    }

    int Individual::realMutate() {
        int mutationCount = 0;
        for (int i = 0; i < config.realVars; i++) {
            if (config.randGen->Realu() <= config.realMutProb) {
                double realI      = reals[i];
                double realILower = config.realLimits[i].first;
                double realIUpper = config.realLimits[i].second;
                double delta1     = (realI - realILower) / (realIUpper - realILower);
                double delta2     = (realIUpper - realI) / (realIUpper - realILower);
                double randReal   = config.randGen->Realu();
                double mutPower   = 1.0 / (config.etaM + 1.0);

                double deltaInverse, value, deltaq;
                if (randReal <= 0.5) {
                    deltaInverse = 1.0 - delta1;
                    value  = 2.0 * randReal + (1.0 - 2.0 * randReal) * (std::pow(deltaInverse, (config.etaM + 1.0)));
                    deltaq = std::pow(value, mutPower) - 1.0;
                }
                else {
                    deltaInverse = 1.0 - delta2;
                    value =
                        2.0 * (1.0 - randReal) + 2.0 * (randReal - 0.5) * (std::pow(deltaInverse, (config.etaM + 1.0)));
                    deltaq = 1.0 - (std::pow(value, mutPower));
                }

                realI = realI + deltaq * (realIUpper - realILower);

                if (realI < realILower)
                    realI = realILower;
                if (realI > realIUpper)
                    realI = realIUpper;
                reals[i] = realI;

                mutationCount++;
            }
        }
        return mutationCount;
    }

    int Individual::binMutate() {
        int mutationCount = 0;
        for (int i = 0; i < config.binVars; i++) {
            for (int j = 0; j < config.binBits[i]; j++) {
                double probability = config.randGen->Realu();
                if (probability <= config.binMutProb) {
                    if (gene[i][j] == 0)
                        gene[i][j] = 1;
                    else
                        gene[i][j] = 0;
                    mutationCount++;
                }
            }
        }
        return mutationCount;
    }

    void Individual::Report(std::ostream& os, int population_generation) const {
        os << population_generation << "," << generation << "," << id << "," << constrViolation << "," << rank << ","
           << crowdDist;

        for (int i = 0; i < config.objectives; i++) {
            os << "," << objScore[i];
        }

        for (int i = 0; i < config.constraints; i++) {
            os << "," << constr[i];
        }

        for (int i = 0; i < config.realVars; i++) {
            os << "," << reals[i];
        }

        for (int i = 0; i < config.binVars; i++) {
            for (int j = 0; j < config.binBits[i]; j++) {
                os << "," << gene[i][j];
            }
        }

        os << std::endl;
    }
}  // namespace nsga2

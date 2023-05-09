#include "Population.hpp"

#include <nuclear>

namespace nsga2 {
    Population::Population(const int& size_,
                           const int& real_vars_,
                           const int& bin_vars_,
                           const int& constraints_,
                           const std::vector<int>& bin_bits_,
                           const std::vector<std::pair<double, double>>& real_limits_,
                           const std::vector<std::pair<double, double>>& bin_limits_,
                           const int& objectives_,
                           const double& real_mut_prob_,
                           const double& bin_mut_prob_,
                           const double& eta_m_,
                           const double& eps_c_,
                           std::shared_ptr<RandomGenerator<>> rand_gen_,
                           const std::vector<double>& initial_real_vars_)
        : ind_config({real_vars_,
                      real_limits_,
                      real_mut_prob_,
                      bin_vars_,
                      bin_bits_,
                      bin_limits_,
                      bin_mut_prob_,
                      objectives_,
                      constraints_,
                      eta_m_,
                      eps_c_,
                      rand_gen_,
                      initial_real_vars_})
        , size(size_) {

        for (int i = 0; i < size_; i++) {
            inds.emplace_back(ind_config);
        }
    }

    void Population::Initialize() {
        for (int i = 0; i < size; i++) {
            inds[i].Initialize(i);
        }
    }
    void Population::Decode() {
        for (auto& ind : inds) {
            ind.Decode();
        }
    }

    void Population::SetIndividualsGeneration(const int generation_) {
        for (auto& ind : inds) {
            ind.generation = generation_;
        }
    }

    void Population::SetIds() {
        for (std::size_t i = 0; i < inds.size(); i++) {
            inds[i].id = i;
        }
    }

    void Population::resetCurrentIndividualIndex() {
        current_ind = 0;
    }

    std::optional<Individual> Population::GetNextIndividual() {
        if (!initialised || current_ind >= inds.size()) {
            return std::nullopt;
        }
        else {
            return std::optional<Individual>{inds[current_ind++]};
        }
    }

    bool Population::AreAllEvaluated() const {
        for (auto& ind : inds) {
            if (!ind.evaluated) {
                return false;
            }
        }
        return true;
    }

    void Population::SetEvaluationResults(const int& _id,
                                          const std::vector<double>& obj_score_,
                                          const std::vector<double>& constraints_) {
        inds[_id].obj_score = obj_score_;
        inds[_id].constr    = constraints_;
        inds[_id].CheckConstraints();
    }

    // Fast Non-Dominated Sort. This calculates the fronts in the population.
    void Population::FastNDS() {
        // Reset Front
        fronts.resize(1);
        fronts[0].clear();

        // Compare each individual `p` to each other individual `q` (also compares p to itself, but doesn't matter)
        for (std::size_t p = 0; p < inds.size(); p++) {
            auto& ind_p = inds[p];
            ind_p.domination_list.clear();   // Reset the set of individuals that P dominates
            ind_p.dominated_by_counter = 0;  // Reset the count of individuals that dominate P

            for (std::size_t q = 0; q < inds.size(); q++) {
                const auto& ind_q = inds[q];

                int comparison = ind_p.CheckDominance(ind_q);
                if (comparison == 1) {
                    // If P dominates Q, Add Q to the solutions that P dominates
                    ind_p.domination_list.push_back(q);
                }
                else if (comparison == -1) {
                    // If Q dominates P, Increment the number of individuals that dominate P
                    ind_p.dominated_by_counter++;
                }
            }

            if (ind_p.dominated_by_counter == 0) {
                // If no other individuals dominate P, then P must be in the first Front (i.e. Rank 1)
                ind_p.rank = 1;
                fronts[0].push_back(p);
            }
        }

        // Sort the first front (sorting by index, since front is `std::vector<std::vector<int>>`)
        std::sort(fronts[0].begin(), fronts[0].end());

        std::size_t front_index = 1;
        while (fronts[front_index - 1].size() > 0) {  // While we haven't encountered an empty front
            std::vector<int>& front_i = fronts[front_index - 1];
            std::vector<int> next_front;  // Known as Q in the original paper, this holds members of the next front
            for (std::size_t p = 0; p < front_i.size(); p++) {
                Individual& ind_p = inds[front_i[p]];

                // For each of the individuals in the domination list, check if it's part of the next front
                for (std::size_t q = 0; q < ind_p.domination_list.size(); q++) {
                    auto& ind_q = inds[ind_p.domination_list[q]];
                    ind_q.dominated_by_counter--;  // Reduce the counter, as we are no longer considering P vs this Q

                    if (ind_q.dominated_by_counter == 0) {
                        // If no other individuals outside of current front dominate Q, then Q must be in the next Front
                        ind_q.rank = front_index + 1;
                        next_front.push_back(ind_p.domination_list[q]);
                    }
                }
            }
            fronts.push_back(next_front);
            front_index++;
        }
    }

    void Population::CrowdingDistanceAll() {
        for (std::size_t i = 0; i < fronts.size(); i++) {
            CrowdingDistance(i);
        }
    }

    // Calculate how close the next nearest solution is. Boundary solutions have infinite distance.
    // This allows us to prioritise boundary solutions over solutions crowded together.
    void Population::CrowdingDistance(const int& front_index_) {
        std::vector<int>& F          = fronts[front_index_];
        const std::size_t front_size = F.size();
        if (front_size == 0) {
            return;  // Don't do anything with an empty front
        }

        for (std::size_t i = 0; i < front_size; i++) {
            inds[F[i]].crowd_dist = 0;  // Initialise crowding distance
        }

        for (int i = 0; i < ind_config.objectives; i++) {  // For each objective
            // Sort the front by objective value
            std::sort(F.begin(), F.end(), [&](const int& a, const int& b) {
                return inds[a].obj_score[i] < inds[b].obj_score[i];
            });

            // Give the bondary solutions infinite distance
            inds[F[0]].crowd_dist              = std::numeric_limits<double>::infinity();
            inds[F[front_size - 1]].crowd_dist = std::numeric_limits<double>::infinity();

            // Calculate the crowding distance of non-boundary solutions
            for (std::size_t j = 1; j < front_size - 1; j++) {
                if (inds[F[j]].crowd_dist != std::numeric_limits<double>::infinity()) {
                    if (inds[F[front_size - 1]].obj_score[i] != inds[F[0]].obj_score[i]) {
                        inds[F[j]].crowd_dist += (inds[F[j + 1]].obj_score[i] - inds[F[j - 1]].obj_score[i])
                                                 / (inds[F[front_size - 1]].obj_score[i] - inds[F[0]].obj_score[i]);
                    }
                }
            }
        }
    }

    void Population::Merge(const Population& pop_1_, const Population& pop_2_) {
        if (GetSize() < pop_1_.GetSize() + pop_2_.GetSize()) {
            NUClear::log<NUClear::WARN>("Merge: target population not big enough");
            inds.reserve(pop_1_.GetSize() + pop_2_.GetSize());
        }

        std::copy(pop_1_.inds.begin(), pop_1_.inds.end(), inds.begin());
        std::copy(pop_2_.inds.begin(), pop_2_.inds.end(), inds.begin() + pop_1_.GetSize());
    }


    std::pair<int, int> Population::Mutate() {
        std::pair<int, int> mut_count     = std::make_pair(0, 0);
        std::pair<int, int> ind_mut_count = std::make_pair(0, 0);
        std::vector<Individual>::iterator it;
        for (it = inds.begin(); it != inds.end(); it++) {
            ind_mut_count = it->Mutate();
            mut_count.first += ind_mut_count.first;
            mut_count.second += ind_mut_count.second;
        }
        return mut_count;
    }

    void Population::Report(std::ostream& os, int current_gen) const {
        for (auto it = inds.begin(); it != inds.end(); it++) {
            it->Report(os, current_gen);
        }
    }
}  // namespace nsga2

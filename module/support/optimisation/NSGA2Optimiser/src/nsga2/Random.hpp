#ifndef MODULE_SUPPORT_OPTIMISATION_NSGA2OPTIMISER_RANDOM_HPP
#define MODULE_SUPPORT_OPTIMISATION_NSGA2OPTIMISER_RANDOM_HPP

#include <random>

namespace nsga2 {
    template <typename IntType = int, typename RealType = double>
    class RandomGenerator {
    public:
        RandomGenerator(const uint32_t& seed_ = 0) : seed(seed_), generator(random_device()), u01d(0, 1), uintd(0, 1) {
            generator.seed(seed_);
        }
        RealType Realu() {
            return u01d(generator);
        }
        RealType Real(const RealType& low, const RealType& high) {
            return (low + (high - low) * Realu());
        }
        IntType Integer(const IntType& low, const IntType& high) {
            uintd.param(typename std::uniform_int_distribution<IntType>::param_type(low, high));
            return uintd(generator);
        }
        void set_seed(const uint32_t& seed_) {
            seed = seed_;
            generator.seed(seed_);
        }
        uint32_t get_seed() const {
            return seed;
        }

    private:
        uint32_t seed;
        std::random_device random_device;
        std::mt19937 generator;
        std::uniform_real_distribution<RealType> u01d;
        std::uniform_int_distribution<IntType> uintd;
    };
}  // namespace nsga2

#endif  // MODULE_SUPPORT_OPTIMISATION_NSGA2OPTIMISER_RANDOM_HPP

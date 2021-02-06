#ifndef MODULE_LOCALISATION_ROBOTPARTICLELOCALISATION_HPP
#define MODULE_LOCALISATION_ROBOTPARTICLELOCALISATION_HPP

#include <nuclear>

#include "RobotModel.hpp"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include "message/support/FieldDescription.hpp"
#include "message/vision/Goal.hpp"

#include "utility/math/filter/eigen/ParticleFilter.hpp"

namespace module {
namespace localisation {

    class RobotParticleLocalisation : public NUClear::Reactor {
    private:
        utility::math::filter::ParticleFilter<double, RobotModel> filter;
        NUClear::clock::time_point last_time_update_time;
        NUClear::clock::time_point last_measurement_update_time;

        static constexpr int TIME_UPDATE_FREQUENCY     = 10;
        static constexpr int PARTICLE_UPDATE_FREQUENCY = 1;
        int draw_particles                             = 10;
        int n_particles;

        /*std::vector<Eigen::VectorXd> getPossibleFieldPositions(const message::vision::Goal& goal,
                                                               const message::support::FieldDescription& fd) const;*/

        Eigen::VectorXd getFieldPosition(const message::vision::Goal& goal,
                                                               const message::support::FieldDescription& fd) const;

    public:
        /// @brief Called by the powerplant to build and setup the RobotParticleLocalisation reactor.
        explicit RobotParticleLocalisation(std::unique_ptr<NUClear::Environment> environment);
    };
}  // namespace localisation
}  // namespace module

#endif  // MODULE_LOCALISATION_ROBOTPARTICLELOCALISATION_HPP

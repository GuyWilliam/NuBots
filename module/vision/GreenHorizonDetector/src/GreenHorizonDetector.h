#ifndef MODULE_VISION_GREENHORIZONDETECTOR_H
#define MODULE_VISION_GREENHORIZONDETECTOR_H

#include <Eigen/Core>
#include <nuclear>
#include <vector>

namespace module {
namespace vision {

    class GreenHorizonDetector : public NUClear::Reactor {

    public:
        /// @brief Called by the powerplant to build and setup the GreenHorizonDetector reactor.
        explicit GreenHorizonDetector(std::unique_ptr<NUClear::Environment> environment);

    private:
        struct {
            float seed_confidence;
            float end_confidence;
            float cluster_points;
            bool draw_convex_hull;
        } config;
    };

}  // namespace vision
}  // namespace module

#endif  // MODULE_VISION_GREENHORIZONDETECTOR_H

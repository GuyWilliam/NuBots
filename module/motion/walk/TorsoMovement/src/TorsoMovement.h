#ifndef MODULE_MOTION_WALK_TORSOMOVEMENT_H
#define MODULE_MOTION_WALK_TORSOMOVEMENT_H

#include <Eigen/Core>
#include <nuclear>

namespace module {
namespace motion {
    namespace walk {

        class TorsoMovement : public NUClear::Reactor {
        private:
            double time_horizon;
            NUClear::clock::duration offset_time;

        public:
            /// @brief Called by the powerplant to build and setup the TorsoMovement reactor.
            explicit TorsoMovement(std::unique_ptr<NUClear::Environment> environment);
        };
    }  // namespace walk
}  // namespace motion
}  // namespace module

#endif  // MODULE_MOTION_WALK_TORSOMOVEMENT_H

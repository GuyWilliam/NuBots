#ifndef MODULE_INPUT_CAMERA_HPP
#define MODULE_INPUT_CAMERA_HPP

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <aravis-0.8/arv.h>
#include <map>
#include <mutex>

#include "CameraContext.hpp"

#include "extension/Configuration.hpp"

namespace module::input {

    class Camera : public NUClear::Reactor {

    public:
        /// @brief Called by the powerplant to build and setup the Camera reactor.
        explicit Camera(std::unique_ptr<NUClear::Environment> environment);

        static void emit_image(ArvStream* stream, CameraContext* context);
        static void control_lost(ArvGvDevice* device, CameraContext* context);

    private:
        std::mutex sensors_mutex;
        std::vector<std::pair<NUClear::clock::time_point, Eigen::Transform<double, 3, Eigen::Affine, Eigen::DontAlign>>>
            Hwps;

        std::map<std::string, CameraContext> cameras;
    };

}  // namespace module::input

#endif  // MODULE_INPUT_CAMERA_HPP

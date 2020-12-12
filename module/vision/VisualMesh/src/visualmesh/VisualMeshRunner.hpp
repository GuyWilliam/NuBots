#ifndef MODULE_VISION_VISUALMESH_VISUALMESHRUNNER_HPP
#define MODULE_VISION_VISUALMESH_VISUALMESHRUNNER_HPP

#define CL_TARGET_OPENCL_VERSION 120
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <functional>
#include <memory>

#include "message/input/Image.hpp"

namespace module::vision::visualmesh {

struct VisualMeshResults {
    Eigen::Matrix<float, Eigen::Dynamic, 2> pixel_coordinates;
    Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> neighbourhood;
    Eigen::VectorXi global_indices;
    Eigen::MatrixXf classifications;
};

class VisualMeshRunner {
private:
    int n_neighbours;
    std::function<VisualMeshResults(const message::input::Image&, const Eigen::Affine3f&)> runner;

public:
    VisualMeshRunner(const std::string& engine,
                     const double& min_height,
                     const double& max_height,
                     const double& max_distance,
                     const double& intersection_tolerance,
                     const std::string& path);
    VisualMeshResults operator()(const message::input::Image& image, const Eigen::Affine3f& Htc);

    std::unique_ptr<std::atomic<bool>> active;
};

}  // namespace module::vision::visualmesh

#endif  // MODULE_VISION_VISUALMESH_VISUALMESHRUNNER_HPP

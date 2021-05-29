#include "VisualMesh.hpp"

#include <Eigen/Geometry>

#include "geometry/Circle.hpp"
#include "geometry/Cylinder.hpp"
#include "geometry/Sphere.hpp"

#include "extension/Configuration.hpp"

#include "message/input/Image.hpp"
#include "message/input/Sensors.hpp"
#include "message/support/FieldDescription.hpp"
#include "message/vision/VisualMesh.hpp"

#include "utility/nusight/NUhelpers.hpp"
#include "utility/support/Timer.hpp"

namespace module::vision {

    using extension::Configuration;

    using message::input::Image;
    using message::input::Sensors;
    using message::support::FieldDescription;

    using VisualMeshMsg = message::vision::VisualMesh;

    VisualMesh::VisualMesh(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        on<Configuration>("VisualMesh.yaml").then([this](const Configuration& config) {
            // Load our weights and biases
            std::vector<std::vector<std::pair<std::vector<std::vector<float>>, std::vector<float>>>> network;

            bool first_loop = true;

            for (const auto& conv : config["network"].config) {

                // New conv layer
                network.emplace_back();
                auto& net_conv = network.back();

                for (const auto& layer : conv) {

                    // New network layer
                    net_conv.emplace_back();
                    auto& net_layer = net_conv.back();

                    // Copy across our weights
                    for (unsigned int i = 0; i < layer["weights"].size(); i++) {
                        const auto& l = layer["weights"][i];

                        net_layer.first.emplace_back();
                        auto& weight = net_layer.first.back();

                        for (const auto& v : l) {
                            weight.push_back(v.as<float>());
                        }
                        if (first_loop && i % 3 == 2) {
                            net_layer.first.emplace_back();
                            net_layer.first.back().resize(net_layer.first.back().size());
                            std::fill(net_layer.first.back().begin(), net_layer.first.back().end(), 0.0f);
                        }
                    }

                    // Copy across our biases
                    for (const auto& v : layer["biases"]) {
                        net_layer.second.push_back(v.as<float>());
                    }

                    first_loop = false;
                }
            }

            if (config["geometry"]["shape"].as<std::string>() == "SPHERE") {
                auto shape = visualmesh::geometry::Sphere<float>(config["geometry"]["radius"].as<float>(),
                                                                 config["geometry"]["intersections"].as<float>(),
                                                                 config["geometry"]["max_distance"].as<float>());
                mesh       = std::make_unique<VM>(shape,
                                            config["height"]["minimum"].as<float>(),
                                            config["height"]["maximum"].as<float>(),
                                            config["height"]["steps"].as<int>());
            }
            else if (config["geometry"]["shape"].as<std::string>() == "CIRCLE") {
                auto shape = visualmesh::geometry::Circle<float>(config["geometry"]["radius"].as<float>(),
                                                                 config["geometry"]["intersections"].as<uint>(),
                                                                 config["geometry"]["max_distance"].as<float>());
                mesh       = std::make_unique<VM>(shape,
                                            config["height"]["minimum"].as<float>(),
                                            config["height"]["maximum"].as<float>(),
                                            config["height"]["steps"].as<int>());
            }
            else if (config["geometry"]["shape"].as<std::string>() == "CYLINDER") {
                auto shape = visualmesh::geometry::Cylinder<float>(config["geometry"]["height"].as<float>(),
                                                                   config["geometry"]["radius"].as<float>(),
                                                                   config["geometry"]["intersections"].as<uint>(),
                                                                   config["geometry"]["max_distance"].as<float>());
                mesh       = std::make_unique<VM>(shape,
                                            config["height"]["minimum"].as<float>(),
                                            config["height"]["maximum"].as<float>(),
                                            config["height"]["steps"].as<int>());
            }

            classifier = std::make_unique<Classifier>(mesh->make_classifier(network));
        });

        on<Trigger<Image>, Buffer<2>>().then([this](const Image& img) {
            // Get our camera to world matrix
            Eigen::Affine3f Hcw(img.Hcw.cast<float>());

            // Transpose and store in our row major array
            std::array<std::array<float, 4>, 4> Hoc;
            Eigen::Map<Eigen::Matrix<float, 4, 4, Eigen::RowMajor>>(Hoc[0].data()) = Hcw.inverse().matrix();

            // Build our lens object
            visualmesh::Lens<float> lens;
            lens.dimensions   = {int(img.dimensions[0]), int(img.dimensions[1])};
            lens.focal_length = img.lens.focal_length * float(img.dimensions[0]);
            lens.fov          = img.lens.fov;
            lens.centre       = {img.lens.centre[0] * float(img.dimensions[0]),
                           img.lens.centre[1] * float(img.dimensions[0])};
            switch (img.lens.projection.value) {
                case Image::Lens::Projection::EQUIDISTANT: lens.projection = visualmesh::EQUIDISTANT; break;
                case Image::Lens::Projection::EQUISOLID: lens.projection = visualmesh::EQUISOLID; break;
                case Image::Lens::Projection::RECTILINEAR: lens.projection = visualmesh::RECTILINEAR; break;
                default: throw std::runtime_error("Unknown lens projection");
            }

            // Get the mesh that was used so we can make our message
            const auto& m = mesh->height(Hoc[2][3]);

            // Perform the classification
            auto results = (*classifier)(m, img.data.data(), img.format, Hoc, lens);

            // Copy the data into the message
            auto msg = std::make_unique<VisualMeshMsg>();
            msg->id  = img.id;

            // Get all the rays
            msg->rays.resize(3, Eigen::Index(results.global_indices.size()));
            int col = 0;
            for (const auto& i : results.global_indices) {
                msg->rays.col(col++) =
                    Eigen::Vector3f(m.nodes[size_t(i)].ray[0], m.nodes[size_t(i)].ray[1], m.nodes[size_t(i)].ray[2]);
            }

            for (const auto& r : m.rows) {
                msg->mesh.emplace_back(r.phi, r.end - r.begin);
            }

            msg->coordinates = Eigen::Map<const Eigen::Matrix<float, 2, Eigen::Dynamic>>(
                reinterpret_cast<float*>(results.pixel_coordinates.data()),
                2,
                Eigen::Index(results.pixel_coordinates.size()));
            msg->indices       = std::move(results.global_indices);
            msg->neighbourhood = Eigen::Map<const Eigen::Matrix<int, 6, Eigen::Dynamic>>(
                reinterpret_cast<int*>(results.neighbourhood.data()),
                6,
                Eigen::Index(results.neighbourhood.size()));
            msg->classifications = Eigen::Map<const Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic>>(
                results.classifications.data(),
                Eigen::Index(results.classifications.size() / results.neighbourhood.size()),
                Eigen::Index(results.neighbourhood.size()));

            msg->Hcw       = img.Hcw;
            msg->timestamp = img.timestamp;

            emit(msg);
        });
    }
}  // namespace module::vision

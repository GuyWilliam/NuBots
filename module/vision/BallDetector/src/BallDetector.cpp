/*
 * This file is part of NUbots Codebase.
 *
 * The NUbots Codebase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The NUbots Codebase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the NUbots Codebase.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2013 NUBots <nubots@nubots.net>
 */

#include "BallDetector.h"


#include "extension/Configuration.h"

#include "message/input/CameraParameters.h"
#include "message/support/FieldDescription.h"
#include "message/vision/ClassifiedImage.h"
#include "message/vision/LookUpTable.h"
#include "message/vision/VisionObjects.h"
#include "message/vision/VisualMesh.h"

#include "utility/math/comparison.h"
#include "utility/math/coordinates.h"
#include "utility/math/geometry/Line.h"
#include "utility/math/geometry/Plane.h"
#include "utility/math/matrix/Transform3D.h"
#include "utility/math/ransac/Ransac.h"
#include "utility/math/ransac/RansacConeModel.h"
#include "utility/math/ransac/RansacVisualMeshModel.h"
#include "utility/math/vision.h"
#include "utility/nusight/NUhelpers.h"
#include "utility/support/eigen_armadillo.h"
#include "utility/support/yaml_armadillo.h"
#include "utility/support/yaml_expression.h"
#include "utility/vision/ClassifiedImage.h"
#include "utility/vision/Vision.h"

#include "utility/math/geometry/Cone.h"

namespace module {
namespace vision {

    using extension::Configuration;

    using message::input::CameraParameters;

    using message::input::Image;
    using message::support::FieldDescription;
    using message::vision::Ball;
    using message::vision::ClassifiedImage;
    using message::vision::LookUpTable;
    using message::vision::VisualMesh;

    using Plane = utility::math::geometry::Plane<3>;

    using ServoID = utility::input::ServoID;
    using utility::math::geometry::Circle;
    using utility::math::geometry::Cone;
    using utility::math::geometry::Line;
    using utility::math::matrix::Transform3D;
    using utility::math::vision::getCamFromImage;
    using utility::math::vision::getCamFromScreen;
    using utility::math::vision::getImageFromCam;
    using utility::math::vision::getParallaxAngle;
    using utility::math::vision::imageToScreen;
    using utility::math::vision::projectCamSpaceToScreen;
    using utility::math::vision::projectCamToPlane;
    using utility::math::vision::screenToImage;
    using utility::math::vision::screenToImageCts;
    using utility::math::vision::widthBasedDistanceToCircle;

    using utility::math::coordinates::cartesianToSpherical;
    using utility::nusight::graph;

    using utility::math::ransac::Ransac;
    using utility::math::ransac::RansacConeModel;
    using utility::math::ransac::RansacVisualMeshModel;
    using utility::nusight::drawVisionLines;
    using utility::support::Expression;

    using FOURCC = utility::vision::FOURCC;
    using Colour = utility::vision::Colour;


    float BallDetector::approximateCircleGreenRatio(const RansacConeModel& cone,
                                                    const Image& image,
                                                    const LookUpTable& lut,
                                                    const CameraParameters& params) {
        std::vector<std::pair<Eigen::Vector2i, Eigen::Vector2i>> debug;
        float r           = 0;
        int numGreen      = 0;
        int actualSamples = 0;
        for (int i = 0; i < green_radial_samples; r = (++i) * cone.gradient / float(green_radial_samples)) {
            float theta = 0;
            if (r == 0) {
                arma::vec2 pos   = projectCamSpaceToScreen(cone.unit_axis, params);
                arma::ivec2 ipos = screenToImage(pos, convert<uint, 2>(params.imageSizePixels));
                if (ipos[0] >= 0 && ipos[0] < int(image.dimensions[0]) && ipos[1] >= 0
                    && ipos[1] < int(image.dimensions[1])) {
                    debug.push_back(std::make_pair(convert<int, 2>(ipos), convert<int, 2>(ipos + arma::ivec2{1, 1})));
                    char c =
                        static_cast<char>(utility::vision::getPixelColour(lut,
                                                                          getPixel(ipos[0],
                                                                                   ipos[1],
                                                                                   image.dimensions[0],
                                                                                   image.dimensions[1],
                                                                                   image.data,
                                                                                   static_cast<FOURCC>(image.format))));

                    if (c == Colour::GREEN) {
                        numGreen++;
                    }
                    actualSamples++;
                }
                continue;
            }
            for (int j = 0; j < green_angular_samples; theta = (++j) * 2 * M_PI / float(green_angular_samples)) {
                arma::vec2 pos   = projectCamSpaceToScreen(cone.getPoint(r, theta), params);
                arma::ivec2 ipos = screenToImage(pos, convert<uint, 2>(params.imageSizePixels));
                if (ipos[0] >= 0 && ipos[0] < int(image.dimensions[0]) && ipos[1] >= 0
                    && ipos[1] < int(image.dimensions[1])) {
                    debug.push_back(std::make_pair(convert<int, 2>(ipos), convert<int, 2>(ipos + arma::ivec2{1, 1})));
                    char c =
                        static_cast<char>(utility::vision::getPixelColour(lut,
                                                                          getPixel(ipos[0],
                                                                                   ipos[1],
                                                                                   image.dimensions[0],
                                                                                   image.dimensions[1],
                                                                                   image.data,
                                                                                   static_cast<FOURCC>(image.format))));

                    if (c == Colour::GREEN) {
                        numGreen++;
                    }
                    actualSamples++;
                }
            }
            // sample point in lut and check if == Colour::GREEN
        }

        emit(drawVisionLines(debug));

        float greenRatio = actualSamples == 0 ? 1 : (numGreen / float(actualSamples));
        return greenRatio;
    }

    std::vector<std::vector<arma::vec4>> BallDetector::findClusters(const VisualMesh& mesh,
                                                                    const CameraParameters& cam) {
        // Alias visual mesh parameters
        int dim = mesh.classifications.back().dimensions;

        // Create container for our clusters
        std::vector<std::vector<arma::vec4>> clusters;

        // Create container to mark off used points
        std::set<int> visited_indices;

        // Loop through each coordinate
        for (auto i = 1; i < int(mesh.coordinates.size()) && visited_indices.size() < mesh.coordinates.size(); ++i) {
            // Check if our current coordinate is above threshold
            if (mesh.classifications.back().values[i * dim] >= mesh_seed_confidence_threshold) {
                // Check if our current seed point has already been visited
                if (visited_indices.empty() or visited_indices.find(i) == visited_indices.end()) {
                    std::vector<arma::vec4> cluster;

                    if (print_mesh_debug) {
                        log("New cluster seeded at:", i);
                    }

                    arma::ivec2 seed_coord = convert<int, 2>(mesh.coordinates[i - 1]);
                    // Transform our point into cam space
                    auto seed_cam = getCamFromImage(seed_coord, cam);

                    // Add our seed point for current cluster with confidence
                    cluster.push_back(arma::vec4(
                        {seed_cam[0], seed_cam[1], seed_cam[2], double(mesh.classifications.back().values[i * dim])}));

                    // Add seed point to the set of visited points
                    visited_indices.insert(i);

                    // Create queue for BFS
                    std::queue<int> search_queue;

                    // Add our first seed point onto end of queue
                    search_queue.push(i);

                    int curr_index;

                    while (!search_queue.empty()) {
                        // Set current index and remove from queue as we branch out from seed point
                        curr_index = search_queue.front();
                        search_queue.pop();

                        if (print_mesh_debug) {
                            log("\tSeed        :", curr_index);
                            log("\tPts visited :", visited_indices.size());
                            log("\tSearch items:", search_queue.size());
                        }

                        // Populate the neighbours
                        arma::ivec6 n = convert<int, 6>(mesh.neighbourhood[curr_index]);

                        // Loop through current index and add neighbours onto queue if they are above confidence
                        // threshold
                        for (auto j = 0; j < 6; ++j) {
                            if (print_mesh_debug) {
                                log("\t\tNeighbour : ", n[j]);
                            }
                            if (print_mesh_debug) {
                                log("\t\t\tConfidence: ", mesh.classifications.back().values[n[j] * dim]);
                            }

                            // Make sure our confidence is above the threshold
                            // Make sure we haven't visited the point before
                            if ((mesh.classifications.back().values[n[j] * dim] >= mesh_branch_confidence_threshold)
                                && (n[j] != mesh.coordinates.size())
                                && (n[j] != 0)) {
                                if (visited_indices.find(n[j]) == visited_indices.end()) {
                                    search_queue.push(n[j]);  // Add to our BFS queue
                                }

                                // Try to determine if we are on an edge point
                                bool edge           = true;
                                arma::ivec6 local_n = convert<int, 6>(mesh.neighbourhood[n[j]]);
                                for (const auto& l : local_n) {
                                    if ((visited_indices.find(l) == visited_indices.end())
                                        && (mesh.classifications.back().values[l * dim]
                                            >= mesh_branch_confidence_threshold)
                                        && (l != mesh.coordinates.size())
                                        && (l != 0)) {
                                        edge = false;
                                    }
                                }

                                if (edge) {
                                    arma::ivec2 point_coord = convert<int, 2>(mesh.coordinates[n[j] - 1]);

                                    auto point_cam = getCamFromImage(point_coord, cam);
                                    cluster.push_back(arma::vec4(
                                        {point_cam[0],
                                         point_cam[1],
                                         point_cam[2],
                                         mesh.classifications.back().values[n[j] * dim]}));  // Add to our cluster
                                }
                            }
                            visited_indices.insert(n[j]);  // Add to our list of visited points
                        }
                    }
                    clusters.push_back(cluster);
                }
            }
        }

        return clusters;
    }

    BallDetector::BallDetector(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment))
        , MINIMUM_POINTS_FOR_CONSENSUS(0)
        , MAXIMUM_ITERATIONS_PER_FITTING(0)
        , MAXIMUM_FITTED_MODELS(0)
        , CONSENSUS_ERROR_THRESHOLD(0.0)
        , MAXIMUM_DISAGREEMENT_RATIO(0.0)
        , maximum_relative_seed_point_distance(0.0)
        , measurement_distance_variance_factor(0.0)
        , measurement_bearing_variance(0.0)
        , measurement_elevation_variance(0.0)
        , green_ratio_threshold(0.0)
        , green_radial_samples(0.0)
        , green_angular_samples(0.0)
        , kmeansClusterer()
        , lastFrame()
        , print_throwout_logs(false) {


        on<Configuration>("BallDetector.yaml").then([this](const Configuration& config) {
            MINIMUM_POINTS_FOR_CONSENSUS = config["ransac"]["minimum_points_for_consensus"].as<uint>();
            CONSENSUS_ERROR_THRESHOLD    = config["ransac"]["consensus_error_threshold"].as<Expression>();

            MAXIMUM_ITERATIONS_PER_FITTING = config["ransac"]["maximum_iterations_per_fitting"].as<uint>();
            MAXIMUM_FITTED_MODELS          = config["ransac"]["maximum_fitted_models"].as<uint>();
            MAXIMUM_DISAGREEMENT_RATIO     = config["maximum_disagreement_ratio"].as<Expression>();

            mesh_branch_confidence_threshold = config["visual_mesh"]["mesh_branch_confidence_threshold"];
            mesh_seed_confidence_threshold   = config["visual_mesh"]["mesh_seed_confidence_threshold"];


            maximum_relative_seed_point_distance = config["maximum_relative_seed_point_distance"].as<double>();

            measurement_distance_variance_factor = config["measurement_distance_variance_factor"].as<Expression>();
            measurement_bearing_variance         = config["measurement_bearing_variance"].as<Expression>();
            measurement_elevation_variance       = config["measurement_elevation_variance"].as<Expression>();

            green_ratio_threshold = config["green_ratio_threshold"].as<Expression>();
            green_radial_samples  = config["green_radial_samples"].as<Expression>();
            green_angular_samples = config["green_angular_samples"].as<Expression>();

            ball_angular_cov = config["ball_angular_cov"].as<arma::vec>();

            kmeansClusterer.configure(config["clustering"]);

            print_throwout_logs = config["print_throwout_logs"].as<bool>();
            print_mesh_debug    = config["print_mesh_debug"].as<bool>();
            draw_cluster        = config["draw_cluster"].as<bool>();

            lastFrame.time = NUClear::clock::now();
        });

        on<Trigger<VisualMesh>, With<FieldDescription>, With<CameraParameters>>().then(
            "Visual Mesh", [this](const VisualMesh& mesh, const FieldDescription& field, const CameraParameters& cam) {
                // We need to gather all points which have a confidence prediction of over MAX_PREDICT_THRESH
                // Then BFS to all neighbouring points which have a confidence prediction of at least MIN_PREDICT_THRESH
                // We then need to create ransac models for each of these 'clusters' to fit a circle

                // Get our coordinate clusters in camera space
                std::vector<std::vector<arma::vec4>> clusters = findClusters(mesh, cam);

                if (print_mesh_debug) {
                    log("Number of clusters found:", clusters.size());
                }

                auto balls = std::make_unique<std::vector<Ball>>();
                balls->reserve(clusters.size());

                for (auto i = 0; i < int(clusters.size()); ++i) {
                    Ball b;

                    // Average all the points in the cluster to find the center
                    arma::vec3 center(arma::fill::zeros);
                    double max_x = -1.0, min_x = 1.0;
                    double max_y = -1.0, min_y = 1.0;
                    double max_z = -1.0, min_z = 1.0;
                    for (const auto& point : clusters[i]) {
                        center += point.head(3);
                        min_x = std::min(min_x, point[0]);
                        max_x = std::max(max_x, point[0]);
                        min_y = std::min(min_y, point[1]);
                        max_y = std::max(max_y, point[1]);
                        min_z = std::min(min_z, point[2]);
                        max_z = std::max(max_z, point[2]);
                    }

                    center /= clusters[i].size();
                    center = arma::normalise(center);

                    // Use the average of the extreme coordinates to determine the radius
                    double radius = (std::abs(max_x - min_x) + std::abs(max_y - min_y) + std::abs(max_z - min_z)) / 6.0;

                    // Work out the width distance
                    arma::vec3 topCam   = arma::normalise(center + arma::vec3({0, 0, radius}));
                    arma::vec3 baseCam  = arma::normalise(center - arma::vec3({0, 0, radius}));
                    arma::vec3 leftCam  = arma::normalise(center + arma::vec3({0, radius, 0}));
                    arma::vec3 rightCam = arma::normalise(center - arma::vec3({0, radius, 0}));

                    arma::vec2 top   = projectCamSpaceToScreen(topCam, cam);
                    arma::vec2 base  = projectCamSpaceToScreen(baseCam, cam);
                    arma::vec2 left  = projectCamSpaceToScreen(leftCam, cam);
                    arma::vec2 right = projectCamSpaceToScreen(rightCam, cam);

                    // https://en.wikipedia.org/wiki/Angular_diameter
                    double delta    = std::acos(arma::dot(topCam, baseCam));
                    double distance = field.ball_radius / std::sin(delta * 0.5);

                    // Work out how far away the ball must be to be at the distance it is from the camera
                    arma::vec3 rBCc = center * distance;

                    // Attach the measurement to the object
                    b.measurements.push_back(Ball::Measurement());
                    b.measurements.back().rBCc       = convert<double, 3, 1>(rBCc);
                    b.measurements.back().covariance = convert<double, 3>(ball_angular_cov).asDiagonal();

                    // Ball cam space info
                    b.cone.axis     = convert<double, 3>(center);
                    b.cone.gradient = -std::numeric_limits<double>::max();

                    for (const auto& point : clusters[i]) {
                        // Check our cluster pointer for the maximum gradient
                        b.cone.gradient = std::tan(std::acos(arma::dot(center, point.head(3))));

                        // Add our points
                        b.edgePoints.push_back(convert<double, 3>(point.head(3)));
                    }

                    // Angular positions from the camera
                    b.visObject.screenAngular = convert<double, 2>(cartesianToSpherical(center).rows(1, 2));
                    b.visObject.angularSize << getParallaxAngle(left, right, cam), getParallaxAngle(top, base, cam);

                    b.visObject.timestamp = NUClear::clock::now();

                    if (print_mesh_debug) {
                        std::cout << "Gradient " << b.cone.gradient << " Center " << center.t() << " Radius " << radius
                                  << " Distance " << distance << " rBCc " << rBCc.t() << " screenAngular "
                                  << b.visObject.screenAngular.transpose() << " angularSize "
                                  << b.visObject.angularSize.transpose() << std::endl;
                    }

                    balls->push_back(std::move(b));
                }

                if (draw_cluster) {
                    std::vector<std::tuple<Eigen::Vector2i, Eigen::Vector2i, Eigen::Vector4d>,
                                Eigen::aligned_allocator<std::tuple<Eigen::Vector2i, Eigen::Vector2i, Eigen::Vector4d>>>
                        lines;
                    for (size_t i = 0; i < clusters.size(); ++i) {

                        Eigen::Vector2i center = convert<int, 2>(
                            screenToImage(projectCamSpaceToScreen(convert<double, 3>(balls->at(i).cone.axis), cam),
                                          convert<uint, 2>(cam.imageSizePixels)));

                        for (size_t j = 0; j < (clusters[i].size()); ++j) {
                            Eigen::Vector4d colour(clusters[i][j][3] >= 0.5, 0.50, clusters[i][j][3] < 0.5, 1);

                            Eigen::Vector2i point =
                                convert<int, 2>(screenToImage(projectCamSpaceToScreen(clusters[i][j].head(3), cam),
                                                              convert<uint, 2>(cam.imageSizePixels)));

                            lines.emplace_back(center.cast<int>(), point.cast<int>(), colour);
                        }
                    }

                    emit(utility::nusight::drawVisionLines(lines));
                }

                emit(std::move(balls));

                // This is the ransac model for visual mesh
                /*
                // For each cluster, we want to ransac the points
                for (auto i = 0; i < int(clusters.size()); ++i) {
                    log("Points in cluster:", clusters[i].size());
                    for (const auto& val : clusters[i]) {
                        log(val);
                    }
                    auto ransacResults = Ransac<RansacVisualMeshModel>::fitModels(clusters[i].begin(),
                                                                                  clusters[i].end(),
                                                                                  MINIMUM_POINTS_FOR_CONSENSUS,
                                                                                  MAXIMUM_ITERATIONS_PER_FITTING,
                                                                                  MAXIMUM_FITTED_MODELS,
                                                                                  CONSENSUS_ERROR_THRESHOLD);

                    auto balls = std::make_unique<std::vector<Ball>>();
                    balls->reserve(ransacResults.size());

                    log("Ransac results:", ransacResults.size());

                    for (auto& result : ransacResults) {
                        Ball b;

                        // Get the 4 points around our circle
                        arma::vec2 top   = projectCamSpaceToScreen(result.model.getTopVector(), cam);
                        arma::vec2 base  = projectCamSpaceToScreen(result.model.getBottomVector(), cam);
                        arma::vec2 left  = projectCamSpaceToScreen(result.model.getLeftVector(), cam);
                        arma::vec2 right = projectCamSpaceToScreen(result.model.getRightVector(), cam);

                        double widthDistance = widthBasedDistanceToCircle(
                            field.ball_radius, result.model.getTopVector(), result.model.getBottomVector(), cam);

                        // Work out how far away the ball must be to be at the distance it is from the camera
                        arma::vec3 width_rBCc = result.model.unit_axis * widthDistance;

                        arma::vec3 rBCc = (width_rBCc);

                        // Attach the measurement to the object
                        b.measurements.push_back(Ball::Measurement());
                        b.measurements.back().rBCc       = convert<double, 3, 1>(rBCc);
                        b.measurements.back().covariance = convert<double, 3>(ball_angular_cov).asDiagonal();

                        // Ball cam space info
                        b.cone.axis     = convert<double, 3>(result.model.unit_axis);
                        b.cone.gradient = result.model.gradient;

                        // Angular positions from the camera
                        b.visObject.screenAngular =
                            convert<double, 2>(cartesianToSpherical(result.model.unit_axis).rows(1, 2));
                        b.visObject.angularSize << getParallaxAngle(left, right, cam), getParallaxAngle(top, base, cam);

                        // Add our points
                        for (auto& point : result) {
                            b.edgePoints.push_back(convert<double, 3>(point));
                        }
                        b.visObject.timestamp = NUClear::clock::now();

                        balls->push_back(std::move(b));
                    }
                    emit(std::move(balls));
                }*/
            });

        // Old ball detector
        // on<Trigger<ClassifiedImage>,
        //    With<CameraParameters>,
        //    With<FieldDescription>,
        //    With<LookUpTable>,
        //    Single,
        //    Priority::LOW>()
        //     .then(
        //         "Ball Detector",
        //         [this](std::shared_ptr<const ClassifiedImage> rawImage,
        //                const CameraParameters& cam,
        //                const FieldDescription& field,
        //                const LookUpTable& lut) {
        //             const auto& image   = *rawImage;
        //             const auto& sensors = *image.sensors;


        //             // This holds our points that may be a part of the ball
        //             std::vector<arma::vec3> ballPoints;
        //             ballPoints.reserve(image.ballPoints.size());

        //             for (const auto& point : image.ballPoints) {
        //                 arma::vec2 pt = imageToScreen(convert<int, 2>(point), convert<uint, 2>(cam.imageSizePixels));
        //                 ballPoints.push_back(getCamFromScreen(pt, cam));
        //             }

        //             // Use ransac to find the ball
        //             auto ransacResults = Ransac<RansacConeModel>::fitModels(ballPoints.begin(),
        //                                                                     ballPoints.end(),
        //                                                                     MINIMUM_POINTS_FOR_CONSENSUS,
        //                                                                     MAXIMUM_ITERATIONS_PER_FITTING,
        //                                                                     MAXIMUM_FITTED_MODELS,
        //                                                                     CONSENSUS_ERROR_THRESHOLD);

        //             auto balls = std::make_unique<std::vector<Ball>>();
        //             balls->reserve(ransacResults.size());

        //             if (print_throwout_logs)
        //                 log("Ransac : ", ransacResults.size(), "results (MAX = ", MAXIMUM_FITTED_MODELS, ")");

        //             arma::mat44 camToGround = convert<double, 4, 4>(sensors.camToGround);

        //             for (auto& result : ransacResults) {

        //                 // Transform our centre into kinematics coordinates
        //                 arma::vec3 axis = result.model.unit_axis;

        //                 // Get the 4 points around our circle
        //                 arma::vec2 top   = projectCamSpaceToScreen(result.model.getTopVector(), cam);
        //                 arma::vec2 base  = projectCamSpaceToScreen(result.model.getBottomVector(), cam);
        //                 arma::vec2 left  = projectCamSpaceToScreen(result.model.getLeftVector(), cam);
        //                 arma::vec2 right = projectCamSpaceToScreen(result.model.getRightVector(), cam);

        //                 double cameraHeight = camToGround(2, 3);

        //                 // Get a unit vector pointing to the centre of the ball
        //                 arma::vec3 ballCentreRay = axis;

        //                 /************************************************
        //                  *                  THROWOUTS                   *
        //                  ************************************************/

        //                 if (print_throwout_logs) {
        //                     log("Ball model: g =  ", result.model.gradient, " axis =   ",
        //                     result.model.unit_axis.t());
        //                 }

        //                 // CENTRE OF BALL IS ABOVE THE HORIZON
        //                 arma::ivec2 centre_im = getImageFromCam(axis, cam);
        //                 if (utility::vision::visualHorizonAtPoint(image, centre_im[0]) > centre_im[1]
        //                     || arma::dot(convert<double, 3>(image.horizon_normal), ballCentreRay) > 0) {
        //                     if (print_throwout_logs) {
        //                         log("Ball discarded: arma::dot(image.horizon_normal,ballCentreRay) > 0 ");
        //                         log("Horizon normal = ", image.horizon_normal.transpose());
        //                         log("Ball centre ray = ", ballCentreRay.t());
        //                     }
        //                     continue;
        //                 }

        //                 // DOES HAVE INTERNAL GREEN
        //                 float greenRatio = approximateCircleGreenRatio(result.model, *(image.image), lut, cam);
        //                 if (greenRatio > green_ratio_threshold) {
        //                     if (print_throwout_logs) log("Ball discarded: greenRatio > green_ratio_threshold");
        //                     continue;
        //                 }

        //                 // DOES NOT TOUCH 3 SEED POINTS
        //                 arma::vec3 sDist({std::numeric_limits<double>::max(),
        //                                   std::numeric_limits<double>::max(),
        //                                   std::numeric_limits<double>::max()});

        //                 // Loop through our seed points and find the minimum distance one
        //                 for (uint i = 0; i < 3; ++i) {
        //                     for (auto& s : image.ballSeedPoints[i].points) {
        //                         arma::ivec2 s_   = convert<int, 2>(s);
        //                         arma::vec3 s_cam = getCamFromImage(s_, cam);
        //                         // Angle error
        //                         double dist = std::acos(result.model.dotDistanceToPoint(s_cam));

        //                         if (sDist[i] > dist) {
        //                             sDist[i] = dist;
        //                         }
        //                     }
        //                 }
        //                 // Check if our largest one is too far away
        //                 if (arma::max(sDist) > maximum_relative_seed_point_distance) {
        //                     if (print_throwout_logs)
        //                         log("Ball discarded: arma::max(sDist) / result.model.radius > "
        //                             "maximum_relative_seed_point_distance");
        //                     if (print_throwout_logs)
        //                         log("arma::max(sDist) = ",
        //                             arma::max(sDist),
        //                             " > ",
        //                             maximum_relative_seed_point_distance);
        //                     continue;
        //                 }

        //                 // BALL IS CLOSER THAN 1/2 THE HEIGHT OF THE ROBOT BY WIDTH
        //                 double widthDistance = widthBasedDistanceToCircle(
        //                     field.ball_radius, result.model.getTopVector(), result.model.getBottomVector(), cam);

        //                 if (widthDistance < cameraHeight * 0.5) {
        //                     if (print_throwout_logs) {
        //                         log("Ball discarded: widthDistance < cameraHeight * 0.5");
        //                         log("widthDistance =", widthDistance, "cameraHeight =", cameraHeight);
        //                     }
        //                     continue;
        //                 }

        //                 // IF THE DISAGREEMENT BETWEEN THE WIDTH AND PROJECTION BASED DISTANCES ARE TOO LARGE
        //                 // Project this vector to a plane midway through the ball
        //                 Plane ballBisectorPlane({0, 0, 1}, {0, 0, field.ball_radius});
        //                 arma::vec3 ballCentreGroundProj =
        //                     projectCamToPlane(ballCentreRay, camToGround, ballBisectorPlane);
        //                 double ballCentreGroundProjDistance = arma::norm(ballCentreGroundProj);

        //                 if (std::abs((widthDistance - ballCentreGroundProjDistance)
        //                              / std::max(ballCentreGroundProjDistance, widthDistance))
        //                     > MAXIMUM_DISAGREEMENT_RATIO) {
        //                     if (print_throwout_logs)
        //                         log("Ball discarded: Width and proj distance disagree too much: width =",
        //                             widthDistance,
        //                             "proj =",
        //                             ballCentreGroundProjDistance);
        //                     continue;
        //                 }

        //                 /************************************************
        //                  *                 MEASUREMENTS                 *
        //                  ************************************************/

        //                 Ball b;
        //                 b.visObject.sensors = image.sensors;

        //                 // Work out how far away the ball must be to be at the distance it is from the camera
        //                 arma::vec3 width_rBCc = ballCentreRay * widthDistance;

        //                 arma::vec3 rBCc = (width_rBCc);

        //                 // Attach the measurement to the object
        //                 b.measurements.push_back(Ball::Measurement());
        //                 b.measurements.back().rBCc       = convert<double, 3, 1>(rBCc);
        //                 b.measurements.back().covariance = convert<double, 3>(ball_angular_cov).asDiagonal();

        //                 // Ball cam space info
        //                 b.cone.axis     = convert<double, 3>(ballCentreRay);
        //                 b.cone.gradient = result.model.gradient;

        //                 // Angular positions from the camera
        //                 b.visObject.screenAngular = convert<double, 2>(cartesianToSpherical(ballCentreRay).rows(1,
        //                 2));
        //                 b.visObject.angularSize << getParallaxAngle(left, right, cam), getParallaxAngle(top, base,
        //                 cam);

        //                 // Add our points
        //                 for (auto& point : result) {
        //                     b.edgePoints.push_back(convert<double, 3>(
        //                         getCamFromScreen(imageToScreen(point, convert<uint, 2>(image.dimensions)), cam)));
        //                 }
        //                 b.visObject.timestamp       = NUClear::clock::now();
        //                 b.visObject.classifiedImage =
        //                 const_cast<ClassifiedImage*>(rawImage.get())->shared_from_this();

        //                 balls->push_back(std::move(b));
        //             }

        //             for (auto a = balls->begin(); a != balls->end(); ++a) {
        //                 Cone<3> acone(convert<double, 3>(a->cone.axis), a->cone.gradient);

        //                 for (auto b = a + 1; b != balls->end();) {
        //                     Cone<3> bcone(convert<double, 3>(b->cone.axis), b->cone.gradient);

        //                     // If our balls overlap
        //                     if (acone.overlaps(bcone)) {
        //                         // Pick the better ball
        //                         if (acone.gradient < bcone.gradient) {
        //                             // Throw-out b
        //                             b = balls->erase(b);
        //                         }
        //                         else {
        //                             a = balls->erase(a);

        //                             if (a == b) {
        //                                 ++b;
        //                             }
        //                         }
        //                     }
        //                     else {
        //                         ++b;
        //                     }
        //                 }
        //             }
        //             // emit(std::move(balls));
        //             lastFrame.time = sensors.timestamp;
        //         });
    }

}  // namespace vision
}  // namespace module

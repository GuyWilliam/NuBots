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
 * Copyright 2022 NUbots <nubots@nubots.net>
 */

#include "BallDetector.hpp"

#include <Eigen/Geometry>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <numeric>

#include "extension/Configuration.hpp"

#include "message/support/FieldDescription.hpp"
#include "message/vision/Ball.hpp"
#include "message/vision/GreenHorizon.hpp"

#include "utility/math/coordinates.hpp"
#include "utility/support/yaml_expression.hpp"
#include "utility/vision/visualmesh/VisualMesh.hpp"

namespace module::vision {

    using extension::Configuration;

    using message::support::FieldDescription;
    using message::vision::Ball;
    using message::vision::Balls;
    using message::vision::GreenHorizon;

    using utility::math::coordinates::cartesianToReciprocalSpherical;
    using utility::math::coordinates::cartesianToSpherical;
    using utility::support::Expression;

    BallDetector::BallDetector(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        on<Configuration>("BallDetector.yaml").then([this](const Configuration& cfg) {
            log_level = cfg["log_level"].as<NUClear::LogLevel>();

            config.confidence_threshold  = cfg["confidence_threshold"].as<float>();
            config.cluster_points        = cfg["cluster_points"].as<int>();
            config.minimum_ball_distance = cfg["minimum_ball_distance"].as<float>();
            config.distance_disagreement = cfg["distance_disagreement"].as<float>();
            config.maximum_deviation     = cfg["maximum_deviation"].as<float>();
            config.ball_angular_cov      = Eigen::Vector3f(cfg["ball_angular_cov"].as<Expression>());
        });

        on<Trigger<GreenHorizon>, With<FieldDescription>, Buffer<2>>().then(
            "Visual Mesh",
            [this](const GreenHorizon& horizon, const FieldDescription& field) {
                // Convenience variables
                const auto& cls                                     = horizon.mesh->classifications;
                const auto& neighbours                              = horizon.mesh->neighbourhood;
                const Eigen::Matrix<float, 3, Eigen::Dynamic>& rays = horizon.mesh->rays;
                const int BALL_INDEX                                = horizon.class_map.at("ball");

                // Get some indices to partition
                std::vector<int> indices(horizon.mesh->indices.size());
                std::iota(indices.begin(), indices.end(), 0);

                // Partition the indices such that the ball points that dont have ball surrounding them are removed,
                // and then resize the vector to remove those points
                auto boundary = utility::vision::visualmesh::partition_points(
                    indices.begin(),
                    indices.end(),
                    neighbours,
                    [&](const int& idx) {
                        return idx == int(indices.size()) || (cls(BALL_INDEX, idx) >= config.confidence_threshold);
                    });
                indices.resize(std::distance(indices.begin(), boundary));

                log<NUClear::DEBUG>(fmt::format("Partitioned {} points", indices.size()));

                // Cluster all points into ball candidates
                // Points are clustered based on their connectivity to other ball points
                // Clustering is down in two steps
                // 1) Take the set of ball points found above and partition them into potential clusters by
                //    a) Add the first point and its ball neighbours to a cluster
                //    b) Find all other ball points who are neighbours of the points in the cluster
                //    c) Partition all of the indices that are in the cluster
                //    d) Repeat a-c for all points that were not partitioned
                //    e) Delete all partitions smaller than a given threshold
                // 2) Discard all clusters are entirely above the green horizon
                std::vector<std::vector<int>> clusters;
                utility::vision::visualmesh::cluster_points(indices.begin(),
                                                            indices.end(),
                                                            neighbours,
                                                            config.cluster_points,
                                                            clusters);

                log<NUClear::DEBUG>(fmt::format("Found {} clusters", clusters.size()));

                // Partition the clusters such that clusters above the green horizons are removed,
                // and then resize the vector to remove them
                auto green_boundary = utility::vision::visualmesh::check_green_horizon_side(clusters.begin(),
                                                                                            clusters.end(),
                                                                                            horizon.horizon.begin(),
                                                                                            horizon.horizon.end(),
                                                                                            rays,
                                                                                            false,
                                                                                            true);
                clusters.resize(std::distance(clusters.begin(), green_boundary));

                log<NUClear::DEBUG>(fmt::format("Found {} clusters below green horizon", clusters.size()));

                // Create the Balls message, which will contain a Ball for every cluster that is a valid ball
                auto balls = std::make_unique<Balls>();

                // Balls is still emitted even if there are no balls, to indicate to other modules that no balls are
                // currently visible
                if (clusters.empty()) {
                    log<NUClear::DEBUG>("Found no balls.");
                    emit(std::move(balls));
                    return;
                }

                // Reserve the memory prematurely for efficiency
                balls->balls.reserve(clusters.size());

                balls->id        = horizon.id;         // subsumption id
                balls->timestamp = horizon.timestamp;  // time when the image was taken
                balls->Hcw       = horizon.Hcw;        // world to camera transform at the time the image was taken

                // Check each cluster to see if it's a valid ball
                for (auto& cluster : clusters) {
                    Ball b;

                    // Average the cluster to get the cones axis
                    Eigen::Vector3f axis = Eigen::Vector3f::Zero();
                    // Add up all the unit vectors of each point (camera to point in world space) in the cluster
                    for (const auto& idx : cluster) {
                        axis += rays.col(idx);
                    }
                    axis /= cluster.size();  // can this line be skipped if it's being normalised in the next line?
                    axis.normalize();

                    // Find the ray with the greatest distance from the axis
                    float radius = 1.0f;
                    for (const auto& idx : cluster) {
                        const Eigen::Vector3f& ray(rays.col(idx));
                        // arccos of the dot product gives the angle between the rays
                        // the smaller arccos is, the bigger the angle - if between 0 and pi
                        radius = axis.dot(ray) < radius ? axis.dot(ray) : radius;
                    }

                    // Set cone information for the ball
                    // The rays are in world space, multiply by Rcw to get the axis in camera space
                    b.cone.axis = horizon.Hcw.topLeftCorner<3, 3>().cast<float>()
                                  * axis;    // Hcw is not an affine transform type (cannot use linear())
                    b.cone.radius = radius;  // arccos(radius) is the angle between the furthest vectors

                    // https://en.wikipedia.org/wiki/Angular_diameter
                    // From the link, the formula with arcsin is used since a ball is a spherical object
                    // The variables are:
                    //      delta: arccos(radius)
                    //      d_act: field.ball_radius
                    //      D: distance
                    // Rearranging the equation gives
                    //      distance = field.ball_radius / (2 * sin(arccos(radius)/2))
                    // Using sin(arccos(x)) = sqrt(1 - x^2)
                    //      distance = field.ball_radius / ( 2 * sqrt( 1 - (radius/2)^2 ) )
                    // !!
                    float distance = field.ball_radius / std::sqrt(1.0f - radius * radius);

                    // Attach the measurement to the object (distance from camera to ball)
                    b.measurements.emplace_back();  // Emplaces default constructed object

                    // b.cone.axis is the unit vector from the camera to the center of the ball in camera space (uBCc)
                    // Convert this unit vector into a position vector and then convert it into Spherical Reciprocal
                    // Coordinates (1/distance, phi, theta)
                    b.measurements.back().srBCc      = cartesianToReciprocalSpherical(b.cone.axis * distance);
                    b.measurements.back().covariance = config.ball_angular_cov.asDiagonal();

                    // Angular positions from the camera, doesn't consider depth - x is removed and unit vector is used
                    b.screen_angular = cartesianToSpherical(axis).tail<2>();
                    // acos(radius) is the angular diameter - ie the radius from the perspective of the camera
                    // used for both directions since it's a ball
                    b.angular_size = Eigen::Vector2f::Constant(std::acos(radius));

                    /***********************************************
                     *                  THROWOUTS                  *
                     ***********************************************/

                    // For this particular ball, see if it should be thrown out
                    // For debugging purposes, go through each check
                    log<NUClear::DEBUG>("**************************************************");
                    log<NUClear::DEBUG>("*                    THROWOUTS                   *");
                    log<NUClear::DEBUG>("**************************************************");
                    bool keep = true;
                    b.colour.fill(1.0f);  // a valid ball has a white colour in NUsight

                    // CALCULATE DEGREE OF FIT - DISCARD IF STANDARD DEVIATION OF ANGLES IS TOO LARGE
                    // Degree of fit defined as the standard deviation of angle between every rays on the
                    // cluster / and the cone axis. If the standard deviation exceeds a given threshold then
                    // it is a bad fit
                    std::vector<float> angles;
                    float mean             = 0.0f;
                    const float max_radius = std::acos(radius);  // largest angle between vectors
                    // Get mean of all the angles in the cluster to then find the standard deviation
                    for (const auto& idx : cluster) {
                        const float angle = std::acos(axis.dot(rays.col(idx))) / max_radius;
                        angles.emplace_back(angle);
                        mean += angle;
                    }
                    mean /= angles.size();
                    // Calculate standard deviation of angles in cluster
                    float deviation = 0.0f;
                    for (const auto& angle : angles) {
                        deviation += (mean - angle) * (mean - angle);
                    }
                    deviation = std::sqrt(deviation / (angles.size() - 1));

                    // If the deviation is more than the maximum allowed, then discard this ball
                    if (deviation > config.maximum_deviation) {

                        log<NUClear::DEBUG>(fmt::format("Ball discarded: deviation ({}) > maximum_deviation ({})",
                                                        deviation,
                                                        config.maximum_deviation));
                        log<NUClear::DEBUG>("--------------------------------------------------");
                        // Balls that violate degree of fit will show as green in NUsight
                        b.colour = keep ? message::conversion::math::fvec4(0.0f, 1.0f, 0.0f, 1.0f) : b.colour;
                        keep     = false;
                    }

                    // DISCARD IF THE DISTANCE FROM THE BALL TO THE ROBOT IS TOO CLOSE
                    // Prevents the robot itself being classed as a ball, ie its arms/hands
                    if (distance < config.minimum_ball_distance) {

                        log<NUClear::DEBUG>(fmt::format("Ball discarded: distance ({}) < minimum_ball_distance ({})",
                                                        distance,
                                                        config.minimum_ball_distance));
                        log<NUClear::DEBUG>("--------------------------------------------------");
                        // Balls that violate this but not previous checks will show as red in NUsight
                        b.colour = keep ? message::conversion::math::fvec4(1.0f, 0.0f, 0.0f, 1.0f) : b.colour;
                        keep     = false;
                    }

                    // DISCARD IF THE DISAGREEMENT BETWEEN THE ANGULAR AND PROJECTION BASED DISTANCES ARE TOO LARGE
                    // Intersect cone axis vector with a plane midway through the ball with normal vector (0, 0, 1)
                    // 1) Do this in world space, not camera space!
                    // https://en.wikipedia.org/wiki/Line%E2%80%93plane_intersection#Algebraic_form
                    // Plane normal = (0, 0, 1)
                    // Point in plane = (0, 0, field.ball_radius)
                    // Line direction = axis
                    // Point on line = camera = Hwc.translation.z() = rCWw.z()
                    Eigen::Affine3f Hcw(horizon.Hcw.cast<float>());
                    // Since the plane normal zeros out x and y, only consider z
                    // why is this not (ball_radius - rCWw.z()) / axis.z()?
                    const float d = (Hcw.inverse().translation().z() - field.ball_radius) / std::abs(axis.z());
                    const Eigen::Vector3f srBCc     = axis * d;  // used later, so save it in a variable
                    const float projection_distance = srBCc.norm();
                    const float max_distance        = std::max(projection_distance, distance);

                    // Discard this ball if projection_distance and distance are too far apart
                    if ((std::abs(projection_distance - distance) / max_distance) > config.distance_disagreement) {

                        log<NUClear::DEBUG>(
                            fmt::format("Ball discarded: Width and proj distance disagree too much: width = "
                                        "{}, proj = {}",
                                        distance,
                                        projection_distance));
                        log<NUClear::DEBUG>("--------------------------------------------------");
                        // Balls that violate this but not previous checks will show as blue in NUsight
                        b.colour = keep ? message::conversion::math::fvec4(0.0f, 0.0f, 1.0f, 1.0f) : b.colour;
                        keep     = false;
                    }

                    // DISCARD IF THE BALL IS FURTHER THAN THE LENGTH OF THE FIELD
                    if (distance > field.dimensions.field_length) {

                        log<NUClear::DEBUG>(
                            fmt::format("Ball discarded: Distance to ball greater than field length: distance = "
                                        "{}, field length = {}",
                                        distance,
                                        field.dimensions.field_length));
                        log<NUClear::DEBUG>("--------------------------------------------------");
                        // Balls that violate this but not previous checks will show as yellow in NUsight
                        b.colour = keep ? message::conversion::math::fvec4(1.0f, 0.0f, 1.0f, 1.0f) : b.colour;
                        keep     = false;
                    }

                    log<NUClear::DEBUG>(fmt::format("Camera {}", balls->id));
                    log<NUClear::DEBUG>(fmt::format("radius {}", b.cone.radius));
                    log<NUClear::DEBUG>(fmt::format("Axis {}", b.cone.axis.transpose()));
                    log<NUClear::DEBUG>(
                        fmt::format("Distance {} - srBCc {}", distance, b.measurements.back().srBCc.transpose()));
                    log<NUClear::DEBUG>(fmt::format("screen_angular {} - angular_size {}",
                                                    b.screen_angular.transpose(),
                                                    b.angular_size.transpose()));
                    log<NUClear::DEBUG>(fmt::format("Projection Distance {}", projection_distance));
                    log<NUClear::DEBUG>(
                        fmt::format("Distance Throwout {}", std::abs(projection_distance - distance) / max_distance));
                    log<NUClear::DEBUG>("**************************************************");

                    // If this ball didn't pass the checks, don't keep it
                    if (!keep) {
                        b.measurements.clear();
                    }
                    // If the ball passed the checks, add it to the Balls message to be emitted
                    // If it didn't pass the checks, but we're debugging, then emit the ball to see throwouts in NUsight
                    if (keep || log_level <= NUClear::DEBUG) {
                        balls->balls.push_back(std::move(b));
                    }
                }

                // Emit the Balls message, even if no balls were found
                emit(std::move(balls));
            });
    }
}  // namespace module::vision


/*****************************************************************************
 * Cone fitting ... kind of broken
                    // Get axis unit vector from 3 unit vectors
                    auto cone_from_points = [&](const int& a, const int& b, const int& c) {
                        Eigen::Matrix3f A;
                        A.row(0) = rays.row(a);
                        A.row(1) = rays.row(b);
                        A.row(2) = rays.row(c);
                        return A.householderQr().solve(Eigen::Vector3f::Ones()).normalized();
                    };

                    // Determine if point d is contained within the cone made from a, b, and c
                    // Returns which of the points a (0), b (1), c (2), or d (3) is contained in the cone formed by the
                    // other 3 points
                    // Returns -1 if none of the cones formed by any permutation is valid
                    auto point_in_cone = [&](const int& a, const int& b, const int& c, const int& d) {
                        std::array<int, 4> perms = {a, b, c, d};
                        for (int i = 0; i < 4; ++i) {
                            const Eigen::Vector3f& p0 = rays.row(perms[i]);
                            const Eigen::Vector3f& p1 = rays.row(perms[(i + 1) % 4]);
                            const Eigen::Vector3f& p2 = rays.row(perms[(i + 2) % 4]);
                            const Eigen::Vector3f& p3 = rays.row(perms[(i + 3) % 4]);
                            Eigen::Vector3f x = cone_from_points(perms[i], perms[(i + 1) % 4], perms[(i + 2) % 4]);

                            // Cone is valid
                            if (utility::math::almost_equal(p0.dot(x), p1.dot(x), 2)
                                && utility::math::almost_equal(p0.dot(x), p2.dot(x), 2)) {
                                // Point is contained within the cone of a, b, c
                                if (x.dot(p0) > config.maximum_cone_radius) {
                                    if (x.dot(p3) >= p0.dot(x)) {
                                        return 3 - i;
                                    }
                                }
                            }
                        }
                        // Everything is just horribly bad, pick on the new guy
                        return 3;
                    };

                    for (auto& cluster : clusters) {
                        Ball b;

                        // Shuffle to be random
                        std::random_shuffle(cluster.begin(), cluster.end());
                        while (cluster.size() > 3) {
                            // Consider changing this to use only 2 vectors to form the axis for the cone
                            cluster.erase(std::next(cluster.begin(),
                                                    point_in_cone(cluster[0], cluster[1], cluster[2], cluster[3])));
                        }

                        // The first (only) 3 points left in the cluster are used to form the cone
                        Eigen::Vector3f axis = cone_from_points(cluster[0], cluster[1], cluster[2]);
                        double radius        = axis.dot(rays.row(cluster[0]));

                        /// DO MEASUREMENTS AND THROWOUTS
                    }

 *****************************************************************************/

/*
 * This file is part of the NUbots Codebase.
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

#ifndef MODULES_LOCALISATION_ROBOTMODEL_HPP
#define MODULES_LOCALISATION_ROBOTMODEL_HPP

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <nuclear>
#include <random>
#include <utility>
#include <vector>

#include "FieldLineSampling.hpp"

#include "message/input/Sensors.hpp"
#include "message/support/FieldDescription.hpp"
#include "message/vision/Goal.hpp"

#include "utility/input/ServoID.hpp"
#include "utility/localisation/transform.hpp"
#include "utility/math/angle.hpp"
#include "utility/math/coordinates.hpp"
#include "utility/math/geometry/Circle.hpp"
#include "utility/math/geometry/Line.hpp"

namespace module::localisation {

    using message::input::Sensors;
    using message::support::FieldDescription;
    using message::vision::Goal;
    using utility::input::ServoID;
    using utility::localisation::fieldStateToTransform3D;
    using utility::math::angle::normalizeAngle;
    using utility::math::coordinates::cartesianToReciprocalSpherical;
    using utility::math::geometry::Arc;
    using utility::math::geometry::Circle;
    using utility::math::geometry::LineSegment;

    template <typename Scalar>
    class RobotModel {
    public:
        static constexpr size_t size = 3;

        using StateVec = Eigen::Matrix<Scalar, size, 1>;
        using StateMat = Eigen::Matrix<Scalar, size, size>;


        enum Components : int {
            // Field center in world space
            kX = 0,
            kY = 1,
            // Angle is the angle from the robot world forward direction to the field forward direction
            kAngle = 2
        };

        // Local variables for this model. Set their values from config file
        // Number of reset particles
        int n_rogues = 0;
        // Number of particles
        int n_particles = 100;
        // Range of reset particles
        Eigen::Matrix<Scalar, 3, 1> resetRange;
        // Diagonal for 3x3 noise matrix (which is diagonal)
        Eigen::Matrix<Scalar, 3, 1> processNoiseDiagonal;

        RobotModel()
            : n_rogues(0)
            , resetRange(Eigen::Matrix<Scalar, 3, 1>::Zero())
            , processNoiseDiagonal(Eigen::Matrix<Scalar, 3, 1>::Ones()) {}

        StateVec time(const StateVec& state, double /*deltaT*/) {
            return state;
        }

        Eigen::Matrix<Scalar, Eigen::Dynamic, 1> predict(const StateVec& state,
                                                         const Eigen::Matrix<Scalar, 3, 1>& rGFf,  // true goal position
                                                         const Eigen::Matrix<Scalar, 4, 4>& Hcw) {

            // Create a transform from the field state
            Eigen::Transform<Scalar, 3, Eigen::Affine> Hfw;
            Hfw.translation() = Eigen::Matrix<Scalar, 3, 1>(state.x(), state.y(), 0);
            Hfw.linear() = Eigen::AngleAxis<Scalar>(state.z(), Eigen::Matrix<Scalar, 3, 1>::UnitZ()).toRotationMatrix();

            const Eigen::Transform<Scalar, 3, Eigen::Affine> Hcf(Hcw * Hfw.inverse().matrix());

            const Eigen::Matrix<Scalar, 3, 1> rGCc(Hcf * rGFf);
            return cartesianToReciprocalSpherical(rGCc);
        }

        auto predict(const StateVec& state,
                     const Eigen::Transform<Scalar, 3, Eigen::Affine>& Hcw,
                     const FieldDescription& fd,
                     const std::string& field_element,
                     const Scalar& fov) {

            // Get the center circle and cut out the visible arc
            if (field_element == "center_circle") {
            }
            // Get the requested field line segment(s) and cut out the visible segment(s)
            else {
            }
            std::map<std::string, std::vector<LineSegment<Scalar, 3, 1>>> field_lines =
                get_field_lines<Scalar>(fd, point_density)[field_element];

            // Create a transform from the field state
            Eigen::Transform<Scalar, 3, Eigen::Affine> Hfw;
            Hfw.translation() = Eigen::Matrix<Scalar, 3, 1>(state.x(), state.y(), 0);
            Hfw.linear() = Eigen::AngleAxis<Scalar>(state.z(), Eigen::Matrix<Scalar, 3, 1>::UnitZ()).toRotationMatrix();

            const Eigen::Transform<Scalar, 3, Eigen::Affine> Hfc = Hcw * Hfw.inverse();

            // Partition all field points into those that are visible and those that are not
            const Scalar cos_theta = std::cos(fov * Scalar(0.5));
            std::vector<int> indexes(rLFf.rows(), 0);
            std::iota(indexes.begin(), indexes.end(), 0);
            const auto it = std::partition(indexes.begin(), indexes.end(), [&](const int& p) {
                return Hfc.rotation().template leftCols<1>().dot(rLFf.row(p)) > cos_theta;
            });

            // Create output matrix of all rLFf points that are onscreen
            // Transform these points into camera space
            const Eigen::Transform<Scalar, 3, Eigen::Affine> Hcf = Hfc.inverse();
            Eigen::Matrix<Scalar, Eigen::Dynamic, 3> uLCc(std::distance(indexes.begin(), it), 3);
            for (int point = 0; point < uLCc.rows(); ++point) {
                uLCc.row(point) = Hcf * rLFf.row(indexes[point]).normalized();
            }

            return uLCc;
        }

        Eigen::Matrix<Scalar, Eigen::Dynamic, 3> predict(const StateVec& state,
                                                         const Eigen::Transform<Scalar, 3, Eigen::Affine>& Hcw,
                                                         const FieldDescription& fd,
                                                         const Scalar& point_density,
                                                         const std::string& field_element,
                                                         const Scalar& fov) {

            // Calculate all of the ground truth points
            Eigen::Matrix<Scalar, Eigen::Dynamic, 3> rLFf =
                sample_field_points<Scalar>(fd, point_density)[field_element];

            // Create a transform from the field state
            Eigen::Transform<Scalar, 3, Eigen::Affine> Hfw;
            Hfw.translation() = Eigen::Matrix<Scalar, 3, 1>(state.x(), state.y(), 0);
            Hfw.linear() = Eigen::AngleAxis<Scalar>(state.z(), Eigen::Matrix<Scalar, 3, 1>::UnitZ()).toRotationMatrix();

            const Eigen::Transform<Scalar, 3, Eigen::Affine> Hfc = Hcw * Hfw.inverse();

            // Partition all field points into those that are visible and those that are not
            const Scalar cos_theta = std::cos(fov * Scalar(0.5));
            std::vector<int> indexes(rLFf.rows(), 0);
            std::iota(indexes.begin(), indexes.end(), 0);
            const auto it = std::partition(indexes.begin(), indexes.end(), [&](const int& p) {
                return Hfc.rotation().template leftCols<1>().dot(rLFf.row(p)) > cos_theta;
            });

            // Create output matrix of all rLFf points that are onscreen
            // Transform these points into camera space
            const Eigen::Transform<Scalar, 3, Eigen::Affine> Hcf = Hfc.inverse();
            Eigen::Matrix<Scalar, Eigen::Dynamic, 3> uLCc(std::distance(indexes.begin(), it), 3);
            for (int point = 0; point < uLCc.rows(); ++point) {
                uLCc.row(point) = Hcf * rLFf.row(indexes[point]).normalized();
            }

            return uLCc;
        }

        StateVec limit(const StateVec& state) {
            return state;
        }

        StateMat noise(const Scalar& deltaT) {
            return processNoiseDiagonal.asDiagonal() * deltaT;
        }

        template <typename T, typename U>
        static auto difference(const T& a, const U& b) {
            return a - b;
        }

        // uLCc_a are the predictions
        // uLCc_b are the measurements
        Eigen::Matrix<Scalar, Eigen::Dynamic, 1> difference(const Eigen::Matrix<Scalar, Eigen::Dynamic, 3>& uLCc_a,
                                                            const Eigen::Matrix<Scalar, Eigen::Dynamic, 3>& uLCc_b) {

            // It is possible to have no predictions, in this instance we return a vector of 1s
            // It should not be possible to have no measurements
            if (uLCc_a.rows() == 0) {
                return Eigen::Matrix<Scalar, Eigen::Dynamic, 1>::Ones(srLCc_b.rows(), 3);
            }

            // Take the dot product of every vector in uRLCc_b with every vector in uLCc_a
            const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> distances = uLCc_b * uLCc_a.transpose();

            // All the rows of differences correspond to the measurement vectors
            // All of the columns correspond to the prediction vectors
            // The maximum coefficient in each row will indicate how close the closest onscreen prediction is to that
            // measurement
            // These will be cos(theta) differences in the range [-1, 1], where 1 is closest and -1 is furthest
            // Rescale to [0, 1], where 0 is closest and 1 is furthest away
            return Scalar(1) - (distances.rowwise().maxCoeff() + Scalar(1)) / Scalar(2);
        }

        // Getters
        int getRogueCount() const {
            return n_rogues;
        }
        [[nodiscard]] StateVec get_rogue() const {
            return resetRange;
        }

        [[nodiscard]] int getParticleCount() const {
            return n_particles;
        }

        Eigen::Matrix<Scalar, 3, 1> getCylindricalPostCamSpaceNormal(
            const message::vision::Goal::MeasurementType& type,
            const Eigen::Matrix<Scalar, 3, 1>& post_centre,
            const Eigen::Transform<Scalar, 3, Eigen::Affine>& Hcf,
            const message::support::FieldDescription& fd) {
            if (!(type == Goal::MeasurementType::LEFT_NORMAL || type == Goal::MeasurementType::RIGHT_NORMAL))
                return Eigen::Matrix<Scalar, 3, 1>::Zero();
            // rZFf = field vertical
            const Eigen::Matrix<Scalar, 4, 1> rZFf = Hcf * Eigen::Matrix<Scalar, 4, 1>::UnitZ();

            const Eigen::Matrix<Scalar, 3, 1> rZCc(rZFf.template head<3>());

            // The vector direction across the field perpendicular to the camera view vector
            const Eigen::Matrix<Scalar, 3, 1> rLRf = rZCc.cross(Eigen::Matrix<Scalar, 3, 1>::UnitX()).normalized();

            const float dir                           = (type == Goal::MeasurementType::LEFT_NORMAL) ? 1.0f : -1.0f;
            const Eigen::Matrix<Scalar, 3, 1> rG_blCc = post_centre + 0.5 * dir * fd.dimensions.goalpost_width * rLRf;
            const Eigen::Matrix<Scalar, 3, 1> rG_tlCc = rG_blCc + fd.dimensions.goal_crossbar_height * rZCc;

            // creating the normal vector (following convention stipulated in VisionObjects)
            return (type == Goal::MeasurementType::LEFT_NORMAL) ? rG_blCc.cross(rG_tlCc).normalized()
                                                                : rG_tlCc.cross(rG_blCc).normalized();
        }

        Eigen::Matrix<Scalar, 3, 1> getSquarePostCamSpaceNormal(const message::vision::Goal::MeasurementType& type,
                                                                const Eigen::Matrix<Scalar, 3, 1>& post_centre,
                                                                const Eigen::Transform<Scalar, 3, Eigen::Affine>& Hcf,
                                                                const message::support::FieldDescription& fd) {
            if (!(type == Goal::MeasurementType::LEFT_NORMAL || type == Goal::MeasurementType::RIGHT_NORMAL))
                return Eigen::Matrix<Scalar, 3, 1>::Zero();

            // Finding 4 corners of goalpost and centre (4 corners and centre)
            Eigen::Matrix<Scalar, 4, 5> goalBaseCorners;
            goalBaseCorners.colwise() =
                Eigen::Matrix<Scalar, 4, 1>(post_centre.x(), post_centre.y(), post_centre.z(), 1);
            //
            goalBaseCorners.col(1) += Eigen::Matrix<Scalar, 4, 1>(0.5 * fd.dimensions.goalpost_depth,
                                                                  0.5 * fd.dimensions.goalpost_width,
                                                                  0,
                                                                  0);
            goalBaseCorners.col(2) += Eigen::Matrix<Scalar, 4, 1>(0.5 * fd.dimensions.goalpost_depth,
                                                                  -0.5 * fd.dimensions.goalpost_width,
                                                                  0,
                                                                  0);
            goalBaseCorners.col(3) += Eigen::Matrix<Scalar, 4, 1>(-0.5 * fd.dimensions.goalpost_depth,
                                                                  0.5 * fd.dimensions.goalpost_width,
                                                                  0,
                                                                  0);
            goalBaseCorners.col(4) += Eigen::Matrix<Scalar, 4, 1>(-0.5 * fd.dimensions.goalpost_depth,
                                                                  -0.5 * fd.dimensions.goalpost_width,
                                                                  0,
                                                                  0);

            Eigen::Matrix<Scalar, 4, 5> goalTopCorners = goalBaseCorners;
            goalTopCorners.colwise() += Eigen::Matrix<Scalar, 4, 1>(0, 0, fd.dimensions.goal_crossbar_height, 0);

            // Transform to robot camera space
            const Eigen::Matrix<Scalar, 4, 5> goalBaseCornersCam = Hcf * goalBaseCorners;
            const Eigen::Matrix<Scalar, 4, 5> goalTopCornersCam  = Hcf * goalTopCorners;

            // Get widest lines
            Eigen::Matrix<Scalar, 3, 1> widestBase(goalBaseCornersCam.col(0).template head<3>());
            Eigen::Matrix<Scalar, 3, 1> widestTop(goalTopCornersCam.col(0).template head<3>());
            float largest_angle = 0;

            for (int i = 1; i < goalBaseCornersCam.cols(); ++i) {
                const Eigen::Matrix<Scalar, 4, 1> baseCorner(goalBaseCornersCam.col(i));
                const Eigen::Matrix<Scalar, 3, 1> baseCorner3(baseCorner.template head<3>());
                const Eigen::Matrix<Scalar, 4, 1> topCorner(goalTopCornersCam.col(i));
                const float angle(std::acos(baseCorner.dot(goalBaseCornersCam.col(0))));

                // Left side will have cross product point in neg field z direction
                const Eigen::Matrix<Scalar, 3, 1> goalBaseCorner0(goalBaseCornersCam.col(0).template head<3>());
                const Eigen::Matrix<Scalar, 3, 1> crossResult(baseCorner3.cross(goalBaseCorner0));
                const Eigen::Matrix<Scalar, 3, 1> topBaseDifference((topCorner - baseCorner).template head<3>());
                const bool left_side = crossResult.dot(topBaseDifference) < 0;

                // If its the largest angle so far for that side, then update our results so far
                if ((left_side && (type == Goal::MeasurementType::LEFT_NORMAL))
                    || (!left_side && (type == Goal::MeasurementType::RIGHT_NORMAL))) {
                    if (angle > largest_angle) {
                        widestBase    = goalBaseCornersCam.col(i).template head<3>();
                        widestTop     = goalTopCornersCam.col(i).template head<3>();
                        largest_angle = angle;
                    }
                }
            }

            // Returning the normal vector (following convention stipulated in VisionObjects)
            // Normals point into the goal centre
            return (type == Goal::MeasurementType::LEFT_NORMAL) ? (widestBase.cross(widestTop)).normalized()
                                                                : (widestTop.cross(widestBase)).normalized();
        }
    };
}  // namespace module::localisation
#endif

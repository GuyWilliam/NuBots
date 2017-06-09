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
#ifndef UTILITY_MATH_VISION_H
#define UTILITY_MATH_VISION_H

#include <cmath>
#include <nuclear>
#include <Eigen/Core>

#include "message/support/FieldDescription.h"
#include "message/localisation/FieldObject.h"
#include "message/input/Sensors.h"

#include "utility/input/ServoID.h"
#include "utility/math/matrix/Transform3D.h"
#include "utility/math/matrix/Transform2D.h"
#include "utility/math/geometry/Plane.h"
#include "utility/math/geometry/ParametricLine.h"
#include "utility/math/angle.h"

namespace utility {
namespace math {
namespace vision {

    using ServoID = utility::input::ServoID;

    inline double getParallaxAngle(const Eigen::Vector2d& screen1, const Eigen::Vector2d& screen2, const double& camFocalLengthPixels){
        Eigen::Vector3d camSpaceP1 = {camFocalLengthPixels, screen1[0], screen1[1]};
        Eigen::Vector3d camSpaceP2 = {camFocalLengthPixels, screen2[0], screen2[1]};

        return utility::math::angle::acos_clamped(camSpaceP1.dot(camSpaceP2) / (camSpaceP1.norm() * camSpaceP2.norm()));
    }

    inline double widthBasedDistanceToCircle(const double& radius, const Eigen::Vector2d& s1, const Eigen::Vector2d& s2, const double& camFocalLengthPixels){
        double parallaxAngle = getParallaxAngle(s1, s2, camFocalLengthPixels);
        double correctionForClosenessEffect = radius * std::sin(parallaxAngle / 2.0);

        return radius / std::tan(parallaxAngle / 2.0) + correctionForClosenessEffect;
    }

    /*! @param separation - Known distance between points in camera space
        @param s1,s2 - Measured screen coordinates in pixels of points
        @param camFocalLengthPixels - Distance to the virtual camera screen in pixels
    */
    inline double distanceToEquidistantPoints(const double& separation, const Eigen::Vector2d& s1, const Eigen::Vector2d& s2, const double& camFocalLengthPixels){
        double parallaxAngle = getParallaxAngle(s1, s2, camFocalLengthPixels);
        return (separation / 2) / std::tan(parallaxAngle / 2);
    }

    /*! @brief
        @param cam - coordinates in camera space of the pixel (cam[0] = y coordinate pixels, cam[1] = z coordinate pixels)
        @return im - coordinates on the screen in image space measured x across, y down, zero at top left
    */
    inline Eigen::Vector2i screenToImage(const Eigen::Vector2d& screen, const Eigen::Matrix<unsigned int, 2, 1>& imageSize){
        Eigen::Vector2d v = Eigen::Vector2d( double(imageSize[0] - 1) * 0.5, double(imageSize[1] - 1) * 0.5 ) - screen;
        return Eigen::Vector2i({ int(lround(v[0])), int(lround(v[1])) });
    }
    inline Eigen::Vector2d imageToScreen(const Eigen::Vector2i& im, const Eigen::Matrix<unsigned int, 2, 1>& imageSize){
        return Eigen::Vector2d( double(imageSize[0] - 1) * 0.5, double(imageSize[1] - 1) * 0.5 ) - im;
    }
    inline Eigen::Vector2d imageToScreen(const Eigen::Vector2d& im, const Eigen::Matrix<unsigned int, 2, 1>& imageSize){
        return Eigen::Vector2d( double(imageSize[0] - 1) * 0.5, double(imageSize[1] - 1) * 0.5 ) - im;
    }

    /*! @brief uses pinhole cam model
        @param point - Point in camera space (x along view axis, y to left of screen, z up along screen)
    */
    inline Eigen::Vector2d projectCamSpaceToScreen(const Eigen::Vector3d& point, const double& camFocalLengthPixels){
        return {camFocalLengthPixels * point[1] / point[0], camFocalLengthPixels * point[2] / point[0]};
    }

    inline Eigen::Vector2d projectWorldPointToScreen(const Eigen::Vector4d& point, const utility::math::matrix::Transform3D& camToGround, const double& camFocalLengthPixels){
        Eigen::Vector4d camSpacePoint = camToGround.inverse() * point;
        return projectCamSpaceToScreen(camSpacePoint.rows(0,2), camFocalLengthPixels);
    }
    inline Eigen::Vector2d projectWorldPointToScreen(const Eigen::Vector3d& point, const utility::math::matrix::Transform3D& camToGround, const double& camFocalLengthPixels){
        Eigen::Vector4d point_ = arma::ones(4);
        point_.rows(0,2) = point;
        return projectWorldPointToScreen(point_, camToGround, camFocalLengthPixels);
    }

    inline Eigen::Vector3d getCamFromScreen(const Eigen::Vector2d& screen, const double& camFocalLengthPixels){
        return Eigen::Vector3d{camFocalLengthPixels, screen[0], screen[1]};
    }

    inline Eigen::Vector3d projectCamToPlane(const Eigen::Vector3d& cam, const utility::math::matrix::Transform3D& camToGround, const utility::math::geometry::Plane<3>& plane){
        Eigen::Vector3d lineDirection = camToGround.submat(0,0,2,2) * cam;
        Eigen::Vector3d linePosition = camToGround.submat(0,3,2,3);

        utility::math::geometry::ParametricLine<3> line;
        line.setFromDirection(lineDirection, linePosition);

        return plane.intersect(line);
    }

    inline Eigen::Vector3d getGroundPointFromScreen(const Eigen::Vector2d& screenPos, const utility::math::matrix::Transform3D& camToGround, const double& camFocalLengthPixels){
        return projectCamToPlane(getCamFromScreen(screenPos, camFocalLengthPixels), camToGround, utility::math::geometry::Plane<3>({ 0, 0, 1 }, { 0, 0, 0 }));
    }

    inline double distanceToVerticalObject(const Eigen::Vector2d& top, const Eigen::Vector2d& base, const double& objectHeight, const double& robotHeight, const double& camFocalLengthPixels) {

        // Parallax from top to base
        double theta = getParallaxAngle(top, base, camFocalLengthPixels);

        // The following equation comes from the dot product identity a*b = |a||b|cos(theta)
        // As we can calculate theta and |a||b| in terms of perpendicular distance to the object we can solve this equation
        // for an inverse equation. It may not be pretty but it will get the job done

        // Cos theta
        const double c = cos(theta);
        const double c2 = c * c;
        // Object height
        const double H = objectHeight;
        const double H2 = H * H;
        // Robot Height
        const double h = robotHeight;
        const double h2 = h * h;

        double innerExpr = std::abs(c)*sqrt(H2*(4.0*H*h + H2*c2 + 4.0*h2*c2 - 4.0*h2 - 4.0*H*h*c2));
        double divisor = 2*std::abs(std::sin(theta));
        return M_SQRT2*sqrt(2.0*H*h + H2*c2 + 2.0*h2*c2 + innerExpr - 2.0*h2 - 2.0*H*h*c2)/divisor;

    }

    inline Eigen::VectorXd objectDirectionFromScreenAngular(const arma::vec& screenAngular){
        if(std::fmod(std::fabs(screenAngular[0]),M_PI) == M_PI_2 || std::fmod(std::fabs(screenAngular[1]),M_PI) == M_PI_2){
            return {0,0,0};
        }
        double tanTheta = std::tan(screenAngular[0]);
        double tanPhi = std::tan(screenAngular[1]);
        double x = 0;
        double y = 0;
        double z = 0;
        double denominator_sqr = 1+tanTheta*tanTheta+tanPhi*tanPhi;
        //Assume facing forward st x>0 (which is fine for screen angular)
        x = 1 / std::sqrt(denominator_sqr);
        y = x * tanTheta;
        z = x * tanPhi;

        return {x,y,z};
    }

    inline Eigen::VectorXd screenAngularFromObjectDirection(const arma::vec& v){
        return {std::atan2(v[1],v[0]),std::atan2(v[2],v[0])};
    }

    inline utility::math::matrix::Transform3D getFieldToCam (
                    const utility::math::matrix::Transform2D& Tft,
                    //f = field
                    //t = torso
                    //c = camera
                    const utility::math::matrix::Transform3D& Htc

                ) {

        // Eigen::Vector3d rWFf;
        // rWFf.rows(0,1) = -Twf.rows(0,1);
        // rWFf[2] = 0.0;
        // // Hwf = rWFw * Rwf
        // utility::math::matrix::Transform3D Hwf =
        //     utility::math::matrix::Transform3D::createRotationZ(-Twf[2])
        //     * utility::math::matrix::Transform3D::createTranslation(rWFf);

        utility::math::matrix::Transform3D Htf = utility::math::matrix::Transform3D(Tft).inverse();

        return Htc.inverse() * Htf;
    }

    inline Eigen::Matrix<double, 3, 4> cameraSpaceGoalProjection(
            const Eigen::Vector3d& robotPose,
            const Eigen::Vector3d& goalLocation,
            const message::support::FieldDescription& field,
            const utility::math::matrix::Transform3D& camToGround,
            const bool& failIfNegative = true) //camtoground is either camera to ground or camera to world, depending on application
    {
        utility::math::matrix::Transform3D Hcf = getFieldToCam(robotPose,camToGround);
        //NOTE: this code assumes that goalposts are boxes with width and high of goalpost_diameter
        //make the base goal corners
        arma::mat goalBaseCorners(4,4);
        goalBaseCorners.row(3).fill(1.0);
        goalBaseCorners.submat(0,0,2,3).each_col() = goalLocation;
        goalBaseCorners.submat(0,0,1,3) -= 0.5*field.dimensions.goalpost_diameter;
        goalBaseCorners.submat(0,0,1,0) += field.dimensions.goalpost_diameter;
        goalBaseCorners.submat(1,1,2,1) += field.dimensions.goalpost_diameter;
        //make the top corner points
        arma::mat goalTopCorners = goalBaseCorners;


        //We create camera world by using camera-torso -> torso-world -> world->field
        //transform the goals from field to camera
        goalBaseCorners = arma::mat(Hcf * goalBaseCorners).rows(0,2);



        //if the goals are not in front of us, do not return valid normals
        Eigen::Matrix<double, 3, 4> prediction;
        if (failIfNegative and arma::any(goalBaseCorners.row(0) < 0.0)) {
            prediction.fill(0);
            return prediction;
        }

        goalTopCorners.row(2).fill(field.goalpost_top_height);
        goalTopCorners = arma::mat(Hcf * goalTopCorners).rows(0,2);

        //Select the (tl, tr, bl, br) corner points for normals
        Eigen::Vector4i cornerIndices;
        cornerIndices.fill(0);


        Eigen::VectorXd pvals = goalBaseCorners.t() * goalBaseCorners.col(0).cross(goalTopCorners.col(0));
        Eigen::Matrix<unsigned int, Eigen::Dynamic, 1> baseIndices = arma::sort_index(pvals);
        cornerIndices[2] = baseIndices[0];
        cornerIndices[3] = baseIndices[3];


        pvals = goalTopCorners.t() * goalBaseCorners.col(0).cross(goalTopCorners.col(0));
        Eigen::Matrix<unsigned int, Eigen::Dynamic, 1> topIndices = arma::sort_index(pvals);
        cornerIndices[0] = topIndices[0];
        cornerIndices[1] = topIndices[3];


        //Create the quad normal predictions. Order is Left, Right, Top, Bottom

        prediction.col(0) = goalBaseCorners.col(cornerIndices[2]).cross(goalTopCorners.col(cornerIndices[0])).normalize();
        prediction.col(1) = goalBaseCorners.col(cornerIndices[1]).cross(goalTopCorners.col(cornerIndices[3])).normalize();

        //for the top and bottom, we check the inner lines in case they are a better match (this stabilizes observations and reflects real world)
        if (goalBaseCorners(2,baseIndices[0]) > goalBaseCorners(2,baseIndices[1])) {
            cornerIndices[2] = baseIndices[1];
        }
        if (goalBaseCorners(2,baseIndices[3]) > goalBaseCorners(2,baseIndices[2])) {
            cornerIndices[3] = baseIndices[2];
        }
        if (goalTopCorners(2,topIndices[0]) > goalTopCorners(2,topIndices[1])) {
            cornerIndices[0] = topIndices[1];
        }
        if (goalTopCorners(2,topIndices[3]) > goalTopCorners(2,topIndices[2])) {
            cornerIndices[1] = topIndices[2];
        }


        prediction.col(2) = goalTopCorners.col(cornerIndices[0]).cross(goalTopCorners.col(cornerIndices[1])).normalize();
        prediction.col(3) = goalBaseCorners.col(cornerIndices[3]).cross(goalBaseCorners.col(cornerIndices[2])).normalize();

        return prediction;

    }


}
}
}

#endif

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

#include "Transform.h"

#include <nuclear>

#include "utility/math/angle.h"
#include "utility/math/matrix.h"

namespace utility {
namespace math {

    Transform::Transform() {
        eye(); // identity matrix by default
    }

    Transform::Transform(arma::vec4 q) {
        // quaternion to rotation conversion
        // http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/
        // http://en.wikipedia.org/wiki/Rotation_group_SO(3)#Quaternions_of_unit_norm
        *this << 1 - 2 * q[2] * q[2] - 2 * q[3] * q[3] << 2     * q[1] * q[2] - 2 * q[3] * q[0] << 2     * q[1] * q[3] + 2 * q[2] * q[0] << 0 << arma::endr
              << 2     * q[1] * q[2] + 2 * q[3] * q[0] << 1 - 2 * q[1] * q[1] - 2 * q[3] * q[3] << 2     * q[2] * q[3] - 2 * q[1] * q[0] << 0 << arma::endr
              << 2     * q[1] * q[3] - 2 * q[2] * q[0] << 2     * q[2] * q[3] + 2 * q[1] * q[0] << 1 - 2 * q[1] * q[1] - 2 * q[2] * q[2] << 0 << arma::endr
              << 0                                     << 0                                     << 0                                     << 1;
    }

    Transform& Transform::translate(const arma::vec3& translation) {
        Transform transform;
        transform.col(3).rows(0,2) = translation;
        *this *= transform;
        return *this;
    }

    Transform& Transform::translateX(double translation) {
        translate({translation, 0, 0});
        return *this;
    }

    Transform& Transform::translateY(double translation) {
        translate({0, translation, 0});
        return *this;
    }

    Transform& Transform::translateZ(double translation) {
        translate({0, 0, translation});
        return *this;
    }

    Transform& Transform::rotateX(double radians) {
        double c = cos(radians);
        double s = sin(radians);
        Transform transform;
        // http://en.wikipedia.org/wiki/Rotation_matrix#Basic_rotations
        transform << 1 << 0 <<  0 << 0 << arma::endr
                  << 0 << c << -s << 0 << arma::endr
                  << 0 << s <<  c << 0 << arma::endr
                  << 0 << 0 <<  0 << 1;
        *this *= transform;
        return *this;
    }

    Transform& Transform::rotateY(double radians) {
        double c = cos(radians);
        double s = sin(radians);
        Transform transform;
        // http://en.wikipedia.org/wiki/Rotation_matrix#Basic_rotations
        transform <<  c << 0 << s << 0 << arma::endr
                  <<  0 << 1 << 0 << 0 << arma::endr
                  << -s << 0 << c << 0 << arma::endr
                  <<  0 << 0 << 0 << 1;
        *this *= transform;
        return *this;
    }

    Transform& Transform::rotateZ(double radians) {
        double c = cos(radians);
        double s = sin(radians);
        Transform transform;
        // http://en.wikipedia.org/wiki/Rotation_matrix#Basic_rotations
        transform << c << -s << 0 << 0 << arma::endr
                  << s <<  c << 0 << 0 << arma::endr
                  << 0 <<  0 << 1 << 0 << arma::endr
                  << 0 <<  0 << 0 << 1;
        *this *= transform;
        return *this;
    }

    Transform& Transform::worldToLocal(const Transform& reference) {
        // http://en.wikipedia.org/wiki/Change_of_basis
        *this = reference.i() * (*this);
        return *this;
    }

    Transform& Transform::localToWorld(const Transform& reference) {
        // http://en.wikipedia.org/wiki/Change_of_basis
        *this = reference * (*this);
        return *this;
    }

    Transform Transform::i() const {
        // Create a new transform
        Transform inverseTransform;
        // Transpose the rotation submatrix (top-left 3x3), this is equivalent to taking the inverse of the rotation matrix
        inverseTransform.submat(0,0,2,2) = submat(0,0,2,2).t();
        // Multiply translation vector (top-right column vector) by the negated inverse rotation matrix
        inverseTransform.submat(0,3,2,3) = -inverseTransform.submat(0,0,2,2) * submat(0,3,2,3);
        /*if (arma::norm(inverseTransform * (*this) - arma::eye(4,4)) > 1e-10){
            NUClear::log<NUClear::WARN>("Inverse failed! Matrix is singular");
        }*/
        return inverseTransform;
    }

}
}

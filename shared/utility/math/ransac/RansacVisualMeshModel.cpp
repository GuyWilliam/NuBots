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

#include "RansacVisualMeshModel.h"

namespace utility {
namespace math {
    namespace ransac {

        bool RansacVisualMeshModel::regenerate(const std::array<DataPoint, REQUIRED_POINTS>& points) {
            if (points.size() == REQUIRED_POINTS && !arma::all(points[0] == points[1])
                && !arma::all(points[0] == points[2])
                && !arma::all(points[1] == points[2])) {
                Matrix X = arma::eye(3, 3);
                X.col(0) = arma::normalise(points[0]);
                X.col(1) = arma::normalise(points[1]);
                X.col(2) = arma::normalise(points[2]);
                return setFromPoints(X);
            }
            else {
                return false;
            }
        }

        double RansacVisualMeshModel::calculateError(const DataPoint& p) const {
            // If cone error is:
            //  > 0 (inside cone)  : error = 1 - confidence,
            //  < 0 (outside cone) : error = confidence
            double error = (std::acos(dotDistanceToPoint(p)) < 0) ? p[4] : 1 - p[4];
            return error * error;
        }

        RansacVisualMeshModel::Vector RansacVisualMeshModel::getTopVector() const {
            Vector cone_up = arma::normalise(arma::cross(unit_axis, Vector({0, 1, 0})));
            return unit_axis + gradient * cone_up;
        }

        RansacVisualMeshModel::Vector RansacVisualMeshModel::getBottomVector() const {
            Vector cone_up = arma::normalise(arma::cross(unit_axis, Vector({0, 1, 0})));
            return unit_axis + gradient * (-cone_up);
        }

        RansacVisualMeshModel::Vector RansacVisualMeshModel::getLeftVector() const {
            Vector cone_left = arma::normalise(arma::cross(Vector({0, 0, 1}), unit_axis));
            return unit_axis + gradient * cone_left;
        }

        RansacVisualMeshModel::Vector RansacVisualMeshModel::getRightVector() const {
            Vector cone_left = arma::normalise(arma::cross(unit_axis, Vector({0, 1, 0})));
            return unit_axis + gradient * (-cone_left);
        }

        RansacVisualMeshModel::Vector RansacVisualMeshModel::getPoint(float g, float theta) const {
            Vector perp      = arma::vec3({0, std::sin(theta + M_PI_2), std::cos(theta + M_PI_2)});
            Vector direction = arma::normalise(arma::cross(unit_axis, perp));
            return unit_axis + g * direction;
        }
    }  // namespace ransac
}  // namespace math
}  // namespace utility

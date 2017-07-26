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
 * Copyright 2013 NUbots <nubots@nubots.net>
 */

#include "RansacGoalModel.h"

namespace module {
namespace vision {

    bool RansacGoalModel::regenerate(const std::array<DataPoint, REQUIRED_POINTS>& pts) {

        if (pts.size() == REQUIRED_POINTS && !arma::all(pts[0].left == pts[1].left)
            && !arma::all(pts[0].right == pts[1].right)) {
            NUClear::log(__FILE__, __LINE__, pts[0].left.t());
            NUClear::log(__FILE__, __LINE__, pts[1].left.t());
            leftPlane.setFrom3Points(pts[0].left, pts[1].left, arma::vec3({0, 0, 0}));
            rightPlane.setFrom3Points(pts[0].right, pts[1].right, arma::vec3({0, 0, 0}));
            NUClear::log(__FILE__, __LINE__, leftPlane.normal.t());
            NUClear::log(__FILE__, __LINE__, rightPlane.normal.t());

            return true;
        }
        else {
            return false;
        }
    }

    double RansacGoalModel::calculateError(const DataPoint& p) const {

        double l = leftPlane.distanceToPoint(p.left);
        double r = rightPlane.distanceToPoint(p.right);

        return l * l + r * r;
    }
}  // namespace vision
}  // namespace module

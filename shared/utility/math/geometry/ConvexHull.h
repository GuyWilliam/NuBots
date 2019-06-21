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
 * Copyright 2013 NUbots <nubots@nubots.net>
 */

#ifndef UTILITY_MATH_GEOMETRY_CONVEXHULL_H
#define UTILITY_MATH_GEOMETRY_CONVEXHULL_H

#include <Eigen/Core>
#include <vector>

namespace utility {
namespace math {
    namespace geometry {

        // Convert a vec3 to a vec2
        // vec2.x = vec3.x / vec3.z
        // vec2.y = vec3.y / vec3.y
        Eigen::Vector2f project_vector(const Eigen::Vector3f& v) {
            return Eigen::Vector2f(v.x() / v.z(), v.y() / v.z());
        }

        // Sort a list of indices by increasing theta order
        template <typename Iterator>
        void sort_by_theta(Iterator first,
                           Iterator last,
                           const Eigen::Matrix<float, Eigen::Dynamic, 3>& rays,
                           const float& world_offset) {
            std::sort(first, last, [&](const int& a, const int& b) {
                const Eigen::Vector3f& p0(rays.row(a));
                const Eigen::Vector3f& p1(rays.row(b));

                float theta0 =
                    std::fmod(std::atan2(p0.y(), p0.x()) + M_PI - world_offset, static_cast<float>(2.0 * M_PI));
                float theta1 =
                    std::fmod(std::atan2(p1.y(), p1.x()) + M_PI - world_offset, static_cast<float>(2.0 * M_PI));
                return theta0 < theta1;
            });
        }

        // Calculates the turning direction of 3 points
        // Returns -1 indicating an anti-clockwise turn
        // Returns  0 indicating a colinear set of points (no turn)
        // Returns  1 indicating a clockwise turn
        template <int N>
        int8_t turn_direction(const Eigen::Matrix<float, N, 1>& p0,
                              const Eigen::Matrix<float, N, 1>& p1,
                              const Eigen::Matrix<float, N, 1>& p2) {
            // Compute z-coordinate of the cross product of P0->P1 and P0->P2
            float cross_z = (p1.y() - p0.y()) * (p2.x() - p1.x()) - (p1.x() - p0.x()) * (p2.y() - p1.y());

            // Clockwise turn
            if (cross_z < 0.0f) {
                return 1;
            }
            // Anti-clockwise turn
            else if (cross_z > 0.0f) {
                return -1;
            }
            // Colinear
            else {
                return 0;
            }
        }

        // A point is in the convex hull if, for every pair of points in the hull, an anti-clockwise turn is made with
        // the given point
        // If the point is allowed to be on the boundary the, for every pair of points in the convex hull, the given
        // point is allowed to be colinear
        bool point_in_convex_hull(const Eigen::Vector2f& point,
                                  const std::vector<int>& hull_indices,
                                  const Eigen::Matrix<float, Eigen::Dynamic, 2>& coords,
                                  const bool& allow_boundary = false) {
            const int8_t threshold = (allow_boundary) ? -1 : 0;
            for (auto it = std::next(hull_indices.begin()); it != hull_indices.end(); it = std::next(it)) {
                Eigen::Vector2f p0(coords.row(*std::prev(it)));
                Eigen::Vector2f p1(coords.row(*it));
                if (turn_direction(p0, p1, point) > threshold) {
                    return false;
                }
            }

            return true;
        }

        bool point_under_hull(const Eigen::Vector3f& point,
                              const std::vector<int>& hull_indices,
                              const Eigen::Matrix<float, Eigen::Dynamic, 3>& rays,
                              const bool& allow_boundary = false) {

            std::vector<int> local_indices(hull_indices.begin(), hull_indices.end());

            auto comp = [&](const int& a, const int& b) {
                const Eigen::Vector3f& p0(rays.row(a));
                const Eigen::Vector3f& p1(rays.row(b));

                float theta0 = std::atan2(p0.y(), p0.x());
                float theta1 = std::atan2(p1.y(), p1.x());
                return theta0 < theta1;
            };

            // Make sure the indices are sorted
            if (!std::is_sorted(local_indices.begin(), local_indices.end(), comp)) {
                std::sort(local_indices.begin(), local_indices.end(), comp);
            }

            const float theta = std::atan2(point.y(), point.x());
            auto it =
                std::lower_bound(local_indices.begin(), local_indices.end(), theta, [&](const int& a, const float& b) {
                    const Eigen::Vector3f& p0(rays.row(a));
                    float theta0 = std::atan2(p0.y(), p0.x());
                    return theta0 < b;
                });

            if (it == local_indices.end()) {
                return false;
            }
            else {
                const Eigen::Vector2f p0 = project_vector(*std::prev(it));
                const Eigen::Vector2f p1 = project_vector(*it);
                const Eigen::Vector2f p2 = project_vector(point);

                // Point should make a clockwise turn if it is under the convex hull.
                // It should be colinear if it is on the convex hull
                int threhold = allow_boundary ? -1 : 0;
                return (turn_direction(p0, p1, p2) > threhold);
            }
        }

        std::vector<int> upper_convex_hull(const std::vector<int>& indices,
                                           const Eigen::Matrix<float, Eigen::Dynamic, 2>& coords,
                                           const bool& cycle = false) {
            // We need a minimum of 3 non-colinear points to calculate the convex hull
            if (indices.size() < 3) {
                return std::vector<int>();
            }

            // The convex hull indices
            std::vector<int> hull_indices;

            // Make a local copy of indices so we can mutate it
            std::vector<int> local_indices(indices.begin(), indices.end());

            // Sort by increasing x, then by decreasing y
            std::sort(local_indices.begin(), local_indices.end(), [&coords](const int& a, const int& b) {
                const Eigen::Vector2f p0(coords.row(a));
                const Eigen::Vector2f p1(coords.row(b));

                return ((p0.x() < p1.x()) || ((p0.x() == p1.x()) && (p0.y() > p1.y())));
            });

            // Remove all colinear points
            for (auto it = std::next(local_indices.begin(), 2); it != local_indices.end();) {
                const Eigen::Vector2f p0(coords.row(*std::prev(it, 2)));
                const Eigen::Vector2f p1(coords.row(*std::prev(it, 1)));
                Eigen::Vector2f p2(coords.row(*it));

                while (turn_direction(p0, p1, p2) == 0) {
                    it = local_indices.erase(it);
                    p2 = coords.row(*it);
                }
                it = std::next(it);
            }

            // We need a minimum of 3 non-colinear points to calculate the convex hull
            if (local_indices.size() < 3) {
                return std::vector<int>();
            }

            // Add the initial points to the convex hull
            hull_indices.push_back(local_indices[0]);
            hull_indices.push_back(local_indices[1]);

            // Add first point on to the end of the list to make a complete cycle
            if (cycle) {
                local_indices.push_back(local_indices[0]);
            }

            // Now go through the rest of the points and add them to the convex hull if each triple makes an
            // clockwise turn
            for (auto it = std::next(local_indices.begin(), 2); it != local_indices.end(); it = std::next(it)) {
                // Triple does not make an clockwise turn, replace the last element in the list
                Eigen::Vector2f p0(coords.row(*std::prev(hull_indices.end(), 2)));
                Eigen::Vector2f p1(coords.row(*std::prev(hull_indices.end(), 1)));
                const Eigen::Vector2f p2(coords.row(*it));
                while ((hull_indices.size() > 1) && (turn_direction(p0, p1, p2) <= 0)) {
                    // Remove the offending point from the convex hull
                    hull_indices.pop_back();
                    p0 = coords.row(*std::prev(hull_indices.end(), 2));
                    p1 = coords.row(*std::prev(hull_indices.end(), 1));
                }

                // Add the new point to the convex hull
                hull_indices.push_back(*it);
            }

            return hull_indices;
        }

        std::vector<int> upper_convex_hull(const std::vector<int>& indices,
                                           const Eigen::Matrix<float, Eigen::Dynamic, 3>& rays,
                                           const float world_offset,
                                           const bool& cycle = false) {
            // We need a minimum of 3 non-colinear points to calculate the convex hull
            if (rays.size() < 3) {
                return std::vector<int>();
            }

            // The convex hull indices
            std::vector<int> hull_indices;

            std::vector<int> local_indices(indices.begin(), indices.end());

            // Sort by increasing theta
            sort_by_theta(local_indices.begin(), local_indices.end(), rays, world_offset);

            // Remove all colinear points
            for (auto it = std::next(local_indices.begin(), 2); it != local_indices.end();) {
                const Eigen::Vector2f p0 = project_vector(rays.row(*std::prev(local_indices.end(), 2)));
                const Eigen::Vector2f p1 = project_vector(rays.row(*std::prev(local_indices.end(), 1)));
                Eigen::Vector2f p2       = project_vector(rays.row(*it));

                while ((it != local_indices.end()) && turn_direction(p0, p1, p2) == 0) {
                    it = local_indices.erase(it);
                    p2 = project_vector(rays.row(*it));
                }
                if (it != local_indices.end()) {
                    it = std::next(it);
                }
            }

            // We need a minimum of 3 non-colinear points to calculate the convex hull
            if (local_indices.size() < 3) {
                return std::vector<int>();
            }

            // Add the initial points to the convex hull
            hull_indices.push_back(local_indices[0]);
            hull_indices.push_back(local_indices[1]);

            // Add first point on to the end of the list to make a complete cycle
            if (cycle) {
                local_indices.push_back(local_indices[0]);
            }

            // Now go through the rest of the points and add them to the convex hull if each triple makes an
            // clockwise turn
            for (auto it = std::next(local_indices.begin(), 2); it != local_indices.end(); it = std::next(it)) {
                Eigen::Vector2f p0       = project_vector(rays.row(*std::prev(hull_indices.end(), 2)));
                Eigen::Vector2f p1       = project_vector(rays.row(*std::prev(hull_indices.end(), 1)));
                const Eigen::Vector2f p2 = project_vector(rays.row(*it));
                // Triple does not make an clockwise turn, replace the last element in the list
                while ((hull_indices.size() > 1) && (turn_direction(p0, p1, p2) <= 0)) {
                    // Remove the offending point from the convex hull
                    hull_indices.pop_back();
                    p0 = project_vector(rays.row(*std::prev(hull_indices.end(), 2)));
                    p1 = project_vector(rays.row(*std::prev(hull_indices.end(), 1)));
                }

                // Add the new point to the convex hull
                hull_indices.push_back(*it);
            }

            return hull_indices;
        }

        // Finds the convex hull of a set of points using the Graham Scan algorithm
        // https://en.wikipedia.org/wiki/Graham_scan
        std::vector<int> graham_scan(const std::vector<int>& indices,
                                     const Eigen::Matrix<float, Eigen::Dynamic, 2>& coords,
                                     const bool& cycle = false) {

            // We need a minimum of 3 non-colinear points to calculate the convex hull
            if (indices.size() < 3) {
                return std::vector<int>();
            }

            // The convex hull indices
            std::vector<int> hull_indices;

            // Make a local copy of indices so we can mutate it
            std::vector<int> local_indices(indices.begin(), indices.end());

            // Find the bottom left point
            size_t bottom_left = 0;
            for (size_t idx = 1; idx < indices.size(); ++idx) {
                const Eigen::Vector2f min_p(coords.row(local_indices[bottom_left]));
                const Eigen::Vector2f p(coords.row(local_indices[idx]));

                if ((p.y() > min_p.y()) || ((p.y() == min_p.y()) && (p.x() < min_p.x()))) {
                    bottom_left = idx;
                }
            }

            // Move bottom left to front of list
            if (bottom_left != 0) {
                std::swap(local_indices[0], local_indices[bottom_left]);
                bottom_left = 0;
            }

            // Sort points by increasing angle with respect to the bottom left point (don't include the bottom left
            // point in the sort)
            std::sort(std::next(local_indices.begin()),
                      local_indices.end(),
                      [&bottom_left, &coords](const int& a, const int& b) {
                          const Eigen::Vector2f p0(coords.row(bottom_left));
                          const Eigen::Vector2f p1(coords.row(a));
                          const Eigen::Vector2f p2(coords.row(b));

                          int direction = turn_direction(p0, p1, p2);

                          // If there is no turn then closest point should be sorted before the furthest point
                          if (direction == 0) {
                              return ((p1 - p0).squaredNorm() < (p2 - p0).squaredNorm());
                          }
                          // Otherwise, sort anti-clockwise turns before clockwise turns
                          else {
                              return (direction < 0);
                          }
                      });

            // Remove all colinear points
            for (auto it = std::next(local_indices.begin(), 2); it != local_indices.end();) {
                const Eigen::Vector2f p0(coords.row(*std::prev(it, 2)));
                const Eigen::Vector2f p1(coords.row(*std::prev(it, 1)));
                Eigen::Vector2f p2(coords.row(*it));

                while (turn_direction(p0, p1, p2) == 0) {
                    it = local_indices.erase(it);
                    Eigen::Vector2f p2(coords.row(*it));
                }
                it = std::next(it);
            }

            // We need a minimum of 3 non-colinear points to calculate the convex hull
            if (local_indices.size() < 3) {
                return hull_indices;
            }

            // Add the initial points to the convex hull
            hull_indices.push_back(local_indices[0]);
            hull_indices.push_back(local_indices[1]);
            hull_indices.push_back(local_indices[2]);

            // Add first point on to the end of the list to make a complete cycle
            if (cycle) {
                local_indices.push_back(local_indices[0]);
            }

            // Now go through the rest of the points and add them to the convex hull if each triple makes an
            // anti-clockwise turn
            for (auto it = std::next(local_indices.begin(), 3); it != local_indices.end(); it = std::next(it)) {
                // Triple does not make an anti-clockwise turn, replace the last element in the list
                Eigen::Vector2f p0(coords.row(*std::prev(hull_indices.end(), 2)));
                Eigen::Vector2f p1(coords.row(*std::prev(hull_indices.end(), 1)));
                const Eigen::Vector2f p2(coords.row(*it));
                while (turn_direction(p0, p1, p2) >= 0) {
                    // Remove the offending point from the convex hull
                    hull_indices.pop_back();
                    p0 = coords.row(*std::prev(hull_indices.end(), 2));
                    p1 = coords.row(*std::prev(hull_indices.end(), 1));
                }

                // Add the new point to the convex hull
                hull_indices.push_back(*it);
            }

            return hull_indices;
        }
    }  // namespace geometry
}  // namespace math
}  // namespace utility

#endif

#include "FootStep.h"

#include <Eigen/Geometry>
#include <vector>

#include "extension/Configuration.h"

#include "message/behaviour/ServoCommand.h"
#include "message/input/Sensors.h"
#include "message/motion/FootTarget.h"
#include "message/motion/KinematicsModel.h"

#include "utility/behaviour/Action.h"
#include "utility/input/LimbID.h"
#include "utility/input/ServoID.h"
#include "utility/math/matrix/Transform3D.h"
#include "utility/motion/InverseKinematics.h"
#include "utility/nusight/NUhelpers.h"

namespace module {
namespace motion {
    namespace walk {

        using extension::Configuration;

        using message::behaviour::ServoCommand;
        using message::input::Sensors;
        using message::motion::FootTarget;
        using message::motion::KinematicsModel;
        using utility::behaviour::RegisterAction;
        using utility::input::LimbID;
        using utility::input::ServoID;
        using utility::math::matrix::Transform3D;
        using utility::motion::kinematics::calculateLegJoints;
        using utility::nusight::graph;

        double FootStep::f_x(const Eigen::Vector3d& pos) {
            return std::exp(-std::abs(std::pow(c * pos.x(), -step_steep)));
        }

        double FootStep::f_y(const Eigen::Vector3d& pos) {
            return std::exp(-std::abs(std::pow(c * pos.x(), -step_steep))) - pos.y() / step_height;
        }

        FootStep::FootStep(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

            on<Configuration>("FootStep.yaml").then([this](const Configuration& config) {
                // Use configuration here from file FootStep.yaml
                step_height = config["step_height"];
                well_width  = config["well_width"];
                step_steep  = config["step_steep"];

                // Constant for f_x and f_y
                c = (std::pow(step_steep, 2 / step_steep) * std::pow(step_height, 1 / step_steep)
                     * std::pow(step_steep * step_height + (step_steep * step_steep * step_height), -1 / step_steep))
                    / well_width;
            });

            update_handle = on<Trigger<Sensors>, With<KinematicsModel>, With<FootTarget>>().then(
                [this](const Sensors& sensors, const KinematicsModel& model, const FootTarget& target) {
                    // Get support foot and swing foot coordinate systems
                    Eigen::Affine3d Htf_s;  // support foot
                    Eigen::Affine3d Htf_w;  // swing foot

                    // Right foot is the swing foot
                    if (target.isRightFootSwing) {
                        // Transform of left foot to torso
                        Htf_s = Eigen::Affine3d(sensors.forwardKinematics[ServoID::L_ANKLE_ROLL]);
                        // Transform of right foot to torso
                        Htf_w = Eigen::Affine3d(sensors.forwardKinematics[ServoID::R_ANKLE_ROLL]);
                    }
                    // Left foot is the swing foot
                    else {
                        // Transform of right foot to torso
                        Htf_s = Eigen::Affine3d(sensors.forwardKinematics[ServoID::R_ANKLE_ROLL]);
                        // Transform of left foot to torso
                        Htf_w = Eigen::Affine3d(sensors.forwardKinematics[ServoID::L_ANKLE_ROLL]);
                    }
                    Eigen::Affine3d Haf_s;
                    Haf_s = target.Haf_s;
                    // Get orientation for world (Rotation of world->torso)
                    Eigen::Affine3d Rtw;
                    Rtw.linear() = Eigen::Affine3d(sensors.world).rotation();

                    // Get the yawless world rotation
                    Eigen::Affine3d Rtg =
                        Rtw * Eigen::AngleAxisd(-Rtw.rotation().eulerAngles(0, 2, 1).y(), Eigen::Vector3d::UnitZ());

                    // Apply the foot yaw to the yawless world rotation
                    Rtg = Rtg * Eigen::AngleAxisd(Htf_s.rotation().eulerAngles(0, 2, 1).y(), Eigen::Vector3d::UnitZ());

                    // Construct a torso to foot ground space (support foot centric world oriented space)
                    Eigen::Affine3d Htg;
                    Htg.linear()      = Rtg.linear();         // Rotation from Rtg
                    Htg.translation() = Htf_s.translation();  // Translation is the same as to the support foot

                    // Vector to the swing foot in ground space
                    Eigen::Vector3d rF_wGg = Htg.inverse() * Htf_w.translation();

                    // Vector from ground to target
                    Eigen::Vector3d rAGg = (Htg.inverse() * Htf_s) * -Haf_s.translation();

                    // Direction of the target from the swing foot
                    Eigen::Vector3d rAF_wg = rAGg - rF_wGg;
                    // Create a rotation to the plane that cuts through the two positions
                    Eigen::Matrix3d Rgp;
                    // X axis is the direction towards the target
                    Rgp.leftCols<1>() = rAF_wg.normalized();
                    // Y axis is straight up
                    Rgp.middleCols<1>(1) = Eigen::Vector3d::UnitZ();
                    // Z axis is the cross product of X and Y
                    Rgp.rightCols<1>() = Rgp.leftCols<1>().cross(Rgp.middleCols<1>(1)).normalized();
                    // Fix X axis by finding the cross product Y and Z
                    Rgp.leftCols<1>() = Rgp.middleCols<1>(1).cross(Rgp.rightCols<1>());

                    // Create transform based on above rotation
                    Eigen::Affine3d Hgp;       // plane to ground transform
                    Hgp.linear()      = Rgp;   // Rotation from above
                    Hgp.translation() = rAGg;  // Translation to target

                    // Make a transformation matrix that goes the whole way
                    Eigen::Affine3d Htp = Htg * Hgp;

                    // Swing foot's position on plane
                    Eigen::Vector3d rF_wPp = Hgp.inverse() * rF_wGg;

                    // Find scale to reach target at specified time
                    std::chrono::duration<double> time_left = target.timestamp - NUClear::clock::now();
                    double distance                         = rF_wPp.norm();
                    double scale                            = time_left > std::chrono::duration<double>::zero()
                                       ? (distance / time_left.count()) * time_horizon
                                       : 1;

                    log(time_left.count(), distance, rF_wPp.transpose(), rF_wGg.transpose(), scale, rAGg.transpose());


                    if (distance < 0.001) {
                        update_handle.disable();
                        return;
                    }

                    // Swing foot's new target position on the plane, scaled for time
                    Eigen::Vector3d rF_tPp = rF_wPp + Eigen::Vector3d(f_x(rF_wPp), f_y(rF_wPp), 0).normalized() * scale;
                    // if start y is > 0 and end y is < 0 then make y = 0
                    // this creates a boundary stopping the robot from moving its foot through the floor/pushing on the
                    // floor
                    if (!target.lift) {
                        rF_tPp.y() = 0;
                    }

                    if (scale > distance) {
                        rF_tPp = rF_wPp;
                    }


                    // Foot target's position relative to torso
                    Eigen::Vector3d rF_tTt = Htp * rF_tPp;

                    // Torso to target transform
                    Eigen::Affine3d Hat;
                    Hat = Haf_s * Htf_s.inverse();
                    // Get torso to swing foot rotation as quaternion
                    Eigen::Quaterniond Rf_wt;
                    Rf_wt = Htf_w.inverse().linear();
                    // Create rotation of torso to target as a quaternion
                    Eigen::Quaterniond Rat;
                    Rat = Hat.linear();
                    // Create rotation matrix for foot target
                    Eigen::Matrix3d Rf_tt;
                    // Slerp the above two Quaternions and switch to rotation matrix to get the rotation
                    // TODO: determine t
                    Rf_tt = Rf_wt.slerp(1, Rat).toRotationMatrix();

                    Eigen::Affine3d Htf_t;
                    // Htf_t.linear() = Eigen::Matrix3d::Identity();  // No rotation
                    Htf_t.linear()      = Rf_tt;   // Rotation from above
                    Htf_t.translation() = rF_tTt;  // Translation to foot target
                    Transform3D t       = convert<double, 4, 4>(Htf_t.matrix());
                    auto joints =
                        calculateLegJoints(model, t, target.isRightFootSwing ? LimbID::RIGHT_LEG : LimbID::LEFT_LEG);

                    auto waypoints = std::make_unique<std::vector<ServoCommand>>();

                    for (const auto& joint : joints) {
                        waypoints->push_back(
                            {target.subsumption_id,
                             NUClear::clock::now()
                                 + NUClear::clock::duration(int(time_horizon * NUClear::clock::period::den)),
                             joint.first,
                             joint.second,
                             20,
                             100});  // TODO: support separate gains for each leg
                    }

                    emit(waypoints);
                });

            on<Trigger<FootTarget>>().then([this] { update_handle.enable(); });
        }
    }  // namespace walk
}  // namespace motion
}  // namespace module

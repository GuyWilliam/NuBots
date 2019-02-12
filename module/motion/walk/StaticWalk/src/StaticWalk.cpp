#include "StaticWalk.h"

#include "extension/Configuration.h"
#include "message/input/Sensors.h"
#include "message/motion/FootTarget.h"
#include "message/motion/TorsoTarget.h"
#include "message/motion/WalkCommand.h"
#include "utility/behaviour/Action.h"
#include "utility/input/LimbID.h"
#include "utility/input/ServoID.h"

namespace module {
namespace motion {
    namespace walk {

        using extension::Configuration;
        using message::input::Sensors;
        using message::motion::FootTarget;
        using message::motion::TorsoTarget;
        using message::motion::WalkCommand;
        using utility::behaviour::RegisterAction;
        using utility::input::LimbID;
        using utility::input::ServoID;

        StaticWalk::StaticWalk(std::unique_ptr<NUClear::Environment> environment)
            : Reactor(std::move(environment)), subsumptionId(size_t(this) * size_t(this) - size_t(this)) {

            on<Configuration>("StaticWalk.yaml").then([this](const Configuration& config) {
                start_phase = NUClear::clock::now();
                state       = INITIAL;

                // Use configuration here from file StaticWalk.yaml
                torso_height   = config["torso_height"].as<double>();
                feet_distance  = config["feet_distance"].as<double>();
                stance_width   = config["stance_width"].as<double>();
                phase_time     = std::chrono::milliseconds(config["phase_time"].as<int>());
                double x_speed = config["x_speed"].as<double>();
                double y_speed = config["y_speed"].as<double>();
                double angle   = config["angle"].as<double>();

                emit(std::make_unique<WalkCommand>(subsumptionId, Eigen::Vector3d(x_speed, y_speed, angle)));
            });

            // on<ResetWalk>().then([this] {
            //     state       = INITIAL;
            //     start_phase = NUClear::clock::now();
            // });

            on<Trigger<Sensors>, With<WalkCommand>>().then([this](const Sensors& sensors,
                                                                  const WalkCommand& walkcommand) {
                // INITIAL state occurs only as the first state in the walk to set the matrix Hff_s
                if (state == INITIAL) {
                    torso_height = -Eigen::Affine3d(sensors.forwardKinematics[ServoID::R_ANKLE_ROLL]).translation().z();

                    Hff_s = (sensors.forwardKinematics[ServoID::L_ANKLE_ROLL]).inverse()
                            * (sensors.forwardKinematics[ServoID::R_ANKLE_ROLL]);
                    Hff_s.translation().y() = -stance_width;
                    // if (Hff_s.translation().norm() < stance_width) {
                    //     double z_height = Hff_s.translation().z();
                    //     Hff_s.translation() *= Hff_s.translation().norm() / stance_width;
                    //     Hff_s.translation().z() = z_height;
                    // }

                    state = RIGHT_LEAN;
                }

                // When the time is over for this phase, begin the next phase
                if (NUClear::clock::now() > start_phase + phase_time) {
                    // Reset the start phase time for the new phase
                    start_phase = NUClear::clock::now();
                    // Change the state of the walk based on what the previous state was
                    switch (state) {
                        case LEFT_LEAN: state = RIGHT_STEP; break;
                        case RIGHT_STEP: {
                            // Store where support is relative to swing, ignoring height and setting the y to the
                            // stance width
                            Hff_s = (sensors.forwardKinematics[ServoID::L_ANKLE_ROLL]).inverse()
                                    * (sensors.forwardKinematics[ServoID::R_ANKLE_ROLL]);
                            Hff_s.translation().y() = -stance_width;

                            // if (Hff_s.translation().norm() < stance_width) {
                            //     double z_height = Hff_s.translation().z();
                            //     Hff_s.translation() *= Hff_s.translation().norm() / stance_width;
                            //     Hff_s.translation().z() = z_height;
                            // }

                            state = RIGHT_LEAN;
                        } break;
                        case RIGHT_LEAN: state = LEFT_STEP; break;
                        case LEFT_STEP: {
                            // Store where support is relative to swing, ignoring height and setting the y to the
                            // stance width
                            Hff_s = (sensors.forwardKinematics[ServoID::R_ANKLE_ROLL]).inverse()
                                    * (sensors.forwardKinematics[ServoID::L_ANKLE_ROLL]);
                            Hff_s.translation().y() = stance_width;

                            // if (Hff_s.translation().norm() < stance_width) {
                            //     double z_height = Hff_s.translation().z();
                            //     Hff_s.translation() *= Hff_s.translation().norm() / stance_width;
                            //     Hff_s.translation().z() = z_height;
                            // }

                            state = LEFT_LEAN;
                        } break;
                        default: break;
                    }
                }

                // Put our COM over the correct foot or move foot to target
                switch (state) {
                    case LEFT_LEAN: {
                        // Support foot to torso transform
                        Eigen::Affine3d Htf(sensors.forwardKinematics[ServoID::L_ANKLE_ROLL]);

                        // Create matrix for TorsoTarget
                        Eigen::Affine3d Haf;
                        Haf.linear() = Eigen::Matrix3d::Identity();
                        // Haf.linear()      = Htf.linear();
                        Haf.translation() = -Eigen::Vector3d(0, 0, torso_height);

                        // Move the COM over the left foot
                        emit(std::make_unique<TorsoTarget>(
                            start_phase + phase_time, false, Haf.matrix(), subsumptionId));

                        // Maintain right foot position while the torso moves over the left foot
                        emit(std::make_unique<FootTarget>(
                            start_phase + phase_time, true, Hff_s.matrix(), false, subsumptionId));
                    } break;
                    case RIGHT_LEAN: {
                        // Support foot to torso transform
                        Eigen::Affine3d Htf(sensors.forwardKinematics[ServoID::R_ANKLE_ROLL]);

                        // Create matrix for TorsoTarget
                        Eigen::Affine3d Haf;
                        Haf.linear() = Eigen::Matrix3d::Identity();
                        // Haf.linear()      = Htf.linear();
                        Haf.translation() = -Eigen::Vector3d(0, 0, torso_height);

                        // Move the COM over the right foot
                        emit(
                            std::make_unique<TorsoTarget>(start_phase + phase_time, true, Haf.matrix(), subsumptionId));

                        // Maintain left foot position while the torso moves over the right foot
                        emit(std::make_unique<FootTarget>(
                            start_phase + phase_time, false, Hff_s.matrix(), false, subsumptionId));

                    } break;
                    case RIGHT_STEP: {
                        // walkcommand is (x,y,theta) where x,y is velocity in m/s and theta is angle in
                        // radians/seconds
                        Eigen::Affine3d Haf;

                        if (walkcommand.command.z() == 0) {
                            Haf.translation() = -Eigen::Vector3d(
                                (walkcommand.command.x() * 2)
                                    / std::chrono::duration_cast<std::chrono::duration<double>>(phase_time).count(),
                                (walkcommand.command.y() * 2)
                                        / std::chrono::duration_cast<std::chrono::duration<double>>(phase_time).count()
                                    - stance_width,
                                0);
                            Haf.linear() = Eigen::Matrix3d::Identity();
                        }

                        else {

                            Haf.translation() = -Eigen::Vector3d(
                                (walkcommand.command.x())
                                    / std::chrono::duration_cast<std::chrono::duration<double>>(phase_time).count(),
                                (walkcommand.command.y())
                                    / std::chrono::duration_cast<std::chrono::duration<double>>(phase_time).count(),
                                0);

                            double rotation = walkcommand.command.z();
                            Eigen::Vector3d origin(Haf.translation().y(), Haf.translation().x(), 0);
                            origin /= rotation;
                            double arcLength =
                                rotation
                                * std::chrono::duration_cast<std::chrono::duration<double>>(phase_time).count();
                            double arcAngle = (M_PI - arcLength) / 2;

                            Eigen::Vector3d rAFf;
                            rAFf.x() = origin.x() * cos(arcAngle) - origin.y() * sin(arcAngle);
                            rAFf.y() = origin.x() * sin(arcAngle) + origin.y() * cos(arcAngle);
                            rAFf.z() = 0;

                            rAFf = rAFf.normalized() * ((origin.norm() / sin(arcAngle)) * sin(arcLength));
                            rAFf.y() -= stance_width;

                            log(rAFf.transpose());

                            Haf.linear()      = Eigen::Matrix3d::Identity();
                            Haf.translation() = -rAFf;
                        }
                        // Move the left foot to the location specified by the walkcommand
                        emit(std::make_unique<FootTarget>(
                            start_phase + phase_time, true, Haf.matrix(), true, subsumptionId));
                    } break;
                    case LEFT_STEP: {
                        // walkcommand is (x,y,theta) where x,y is velocity in m/s and theta is angle in
                        // radians/seconds
                        Eigen::Affine3d Haf;

                        if (walkcommand.command.z() == 0) {
                            Haf.translation() = -Eigen::Vector3d(
                                (walkcommand.command.x() * 2)
                                    / std::chrono::duration_cast<std::chrono::duration<double>>(phase_time).count(),
                                (walkcommand.command.y() * 2)
                                        / std::chrono::duration_cast<std::chrono::duration<double>>(phase_time).count()
                                    + stance_width,
                                0);
                            Haf.linear() = Eigen::Matrix3d::Identity();
                        }

                        else {
                            Haf.translation() = -Eigen::Vector3d(
                                (walkcommand.command.x() * 2)
                                    / std::chrono::duration_cast<std::chrono::duration<double>>(phase_time).count(),
                                (walkcommand.command.y() * 2)
                                    / std::chrono::duration_cast<std::chrono::duration<double>>(phase_time).count(),
                                0);

                            double rotation = walkcommand.command.z() * 2;
                            Eigen::Vector3d origin(Haf.translation().y(), Haf.translation().x(), 0);
                            origin /= rotation;
                            double arcLength =
                                rotation
                                * std::chrono::duration_cast<std::chrono::duration<double>>(phase_time).count();
                            double arcAngle = (M_PI - arcLength) / 2;

                            Eigen::Vector3d rAFf;
                            rAFf.x() = origin.x() * cos(arcAngle) - origin.y() * sin(arcAngle);
                            rAFf.y() = origin.x() * sin(arcAngle) + origin.y() * cos(arcAngle);
                            rAFf.z() = 0;

                            rAFf = rAFf.normalized() * ((origin.norm() / sin(arcAngle)) * sin(arcLength));
                            rAFf.y() += stance_width;

                            Haf.linear()      = Eigen::Matrix3d::Identity();
                            Haf.translation() = -rAFf;
                        }
                        // Move the left foot to the location specified by the walkcommand
                        emit(std::make_unique<FootTarget>(
                            start_phase + phase_time, false, Haf.matrix(), true, subsumptionId));
                    } break;
                    default: break;
                }
            });

            emit<Scope::INITIALIZE>(std::make_unique<RegisterAction>(
                RegisterAction{subsumptionId,
                               "StaticWalk",
                               {std::pair<float, std::set<LimbID>>(10, {LimbID::LEFT_LEG, LimbID::RIGHT_LEG})},
                               [this](const std::set<LimbID>&) {},
                               [this](const std::set<LimbID>&) {},
                               [this](const std::set<ServoID>& servoSet) {}}));
        }
    }  // namespace walk
}  // namespace motion
}  // namespace module

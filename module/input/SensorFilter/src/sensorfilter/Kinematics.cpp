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
 * Copyright 2023 NUbots <nubots@nubots.net>
 */

#include "SensorFilter.hpp"

#include "message/actuation/BodySide.hpp"

#include "utility/actuation/ForwardKinematics.hpp"
#include "utility/actuation/tinyrobotics.hpp"
#include "utility/input/LimbID.hpp"
#include "utility/input/ServoID.hpp"

namespace module::input {

    using message::actuation::BodySide;

    using utility::actuation::kinematics::calculateAllPositions;
    using utility::actuation::kinematics::calculateCentreOfMass;
    using utility::actuation::kinematics::calculateInertialTensor;
    using utility::actuation::tinyrobotics::forward_kinematics_to_servo_map;
    using utility::actuation::tinyrobotics::sensors_to_configuration;
    using utility::input::ServoID;

    void SensorFilter::update_kinematics(std::unique_ptr<Sensors>& sensors,
                                         const KinematicsModel& kinematics_model,
                                         const RawSensors& raw_sensors) {

        // Convert the sensor joint angles to a configuration vector
        Eigen::Matrix<double, 20, 1> q = sensors_to_configuration(sensors);

        // **************** Kinematics ****************
        // Htx is a map from ServoID to homogeneous transforms from each ServoID to the torso
        std::vector<Eigen::Isometry3d> fk = forward_kinematics(nugus_model, q);
        auto Htx                          = forward_kinematics_to_servo_map(fk);
        for (const auto& entry : Htx) {
            sensors->Htx[entry.first] = entry.second.matrix();
        }
        auto Htx_old = calculateAllPositions(kinematics_model, *sensors);
        // TODO: Fix Hpc in cameras
        sensors->Htx[ServoID::HEAD_PITCH].block(0, 0, 3, 3) = Htx_old[ServoID::HEAD_PITCH].rotation();

        // **************** Centre of Mass and Inertia Tensor ****************
        sensors->rMTt = centre_of_mass(nugus_model, q);

        // **************** Foot Down Information ****************
        sensors->feet[BodySide::RIGHT].down = true;
        sensors->feet[BodySide::LEFT].down  = true;
        const Eigen::Isometry3d Htr(sensors->Htx[ServoID::R_ANKLE_ROLL]);
        const Eigen::Isometry3d Htl(sensors->Htx[ServoID::L_ANKLE_ROLL]);
        const Eigen::Isometry3d Hlr = Htl.inverse() * Htr;
        const Eigen::Vector3d rRLl  = Hlr.translation();
        switch (cfg.footDown.method()) {
            case FootDownMethod::Z_HEIGHT:
                if (rRLl.z() < -cfg.footDown.threshold()) {
                    sensors->feet[BodySide::LEFT].down = false;
                }
                else if (rRLl.z() > cfg.footDown.threshold()) {
                    sensors->feet[BodySide::RIGHT].down = false;
                }
                break;
            case FootDownMethod::FSR:
                // Determine if any two diagonally opposite FSRs are in contact with the ground, which is either fsr1
                // and fsr3, or fsr2 and fsr4. A FSR is in contact with the ground if its value is greater than the
                // certainty threshold
                sensors->feet[BodySide::LEFT].down = (raw_sensors.fsr.left.fsr1 > cfg.footDown.threshold()
                                                      && raw_sensors.fsr.left.fsr3 > cfg.footDown.threshold())
                                                     || (raw_sensors.fsr.left.fsr2 > cfg.footDown.threshold()
                                                         && raw_sensors.fsr.left.fsr4 > cfg.footDown.threshold());
                sensors->feet[BodySide::RIGHT].down = (raw_sensors.fsr.right.fsr1 > cfg.footDown.threshold()
                                                       && raw_sensors.fsr.right.fsr3 > cfg.footDown.threshold())
                                                      || (raw_sensors.fsr.right.fsr2 > cfg.footDown.threshold()
                                                          && raw_sensors.fsr.right.fsr4 > cfg.footDown.threshold());
                break;
            default: log<NUClear::WARN>("Unknown foot down method"); break;
        }
    }
}  // namespace module::input

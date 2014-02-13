
/*
 * This file is part of NUbugger.
 *
 * NUbugger is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NUbugger is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NUbugger.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2013 NUBots <nubots@nubots.net>
 */

#include "KinematicsDebug.h"

#include "messages/support/Configuration.h"
#include "messages/input/ServoID.h"
#include "messages/input/Sensors.h"
#include "messages/motion/ServoWaypoint.h"
#include "utility/motion/InverseKinematics.h"
#include "utility/motion/ForwardKinematics.h"
#include "utility/math/matrix.h"
#include "utility/motion/RobotModels.h"
#include <cstdlib>


namespace modules {
    namespace debug {
            using messages::support::Configuration;
            using messages::motion::ServoWaypoint;
            using messages::input::ServoID;
            using messages::input::Sensors;
            using utility::motion::kinematics::calculateLegJoints;
            using utility::motion::kinematics::calculatePosition;
            using utility::motion::kinematics::Side;
            using utility::math::matrix::xRotationMatrix;
            using utility::math::matrix::yRotationMatrix;
            using utility::math::matrix::zRotationMatrix;
            using utility::motion::kinematics::DarwinModel;

            KinematicsDebug::KinematicsDebug(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {
        			on< Trigger<Configuration<InverseKinematicsRequest>> >([this](const Configuration<InverseKinematicsRequest>& request) {
        		        return;
        				arma::mat44 target = yRotationMatrix(request.config["yAngle"], 4);
        				target *= xRotationMatrix(request.config["xAngle"], 4);
        				target *= zRotationMatrix(request.config["zAngle"], 4);
        				
        				// translation
        				target(0,3) = request.config["x"]; // down/up
        				target(1,3) = request.config["y"]; // left/right
        				target(2,3) = request.config["z"]; // front/back

                        bool left = request.config["left"];
                        bool right = request.config["right"];

                        auto waypoints = std::make_unique<std::vector<ServoWaypoint> >();

                        if (left) {
                            std::vector<std::pair<ServoID, float> > legJoints = calculateLegJoints<DarwinModel>(target, Side::LEFT);
                            for (auto& legJoint : legJoints) {
                                ServoWaypoint waypoint;

                                ServoID servoID;
                                float position;

                                std::tie(servoID, position) = legJoint;

                                waypoint.time = NUClear::clock::now() + std::chrono::seconds(2);
                                waypoint.id = servoID;
                                waypoint.position = position;
                                waypoint.gain = 20;

                                waypoints->push_back(waypoint);
                            }
                        }

                        if (right) {
                            std::vector<std::pair<ServoID, float> > legJoints = calculateLegJoints<DarwinModel>(target, Side::RIGHT);
                            for (auto& legJoint : legJoints) {
                                ServoWaypoint waypoint;

                                ServoID servoID;
                                float position;

                                std::tie(servoID, position) = legJoint;

                                waypoint.time = NUClear::clock::now() + std::chrono::seconds(2);
                                waypoint.id = servoID;
                                waypoint.position = position;
                                waypoint.gain = 20;

                                waypoints->push_back(waypoint);
                            }
                        }

                        emit(std::move(waypoints));
        			});

                    on< Trigger<Configuration<ForwardKinematicsRequest>> >([this](const Configuration<ForwardKinematicsRequest>& request) {

                    });

                    on< Trigger<Configuration<KinematicsNULLTest>> >([this](const Configuration<KinematicsNULLTest>& request) {
                        
                        arma::mat44 ikRequest = yRotationMatrix(request.config["yAngle"], 4);
                        ikRequest *= xRotationMatrix(request.config["xAngle"], 4);
                        ikRequest *= zRotationMatrix(request.config["zAngle"], 4);
                        
                        // translation
                        ikRequest(0,3) = request.config["x"];
                        ikRequest(1,3) = request.config["y"]; 
                        ikRequest(2,3) = request.config["z"]; 

                        if(request.config["RANDOMIZE"]){
                            ikRequest = yRotationMatrix(2*M_PI*rand()/static_cast<double>(RAND_MAX), 4);
                            ikRequest *= xRotationMatrix(2*M_PI*rand()/static_cast<double>(RAND_MAX), 4);
                            ikRequest *= zRotationMatrix(2*M_PI*rand()/static_cast<double>(RAND_MAX), 4);
                            ikRequest(0,3) = 0.1 * rand()/static_cast<double>(RAND_MAX);
                            ikRequest(1,3) = 0.1 * rand()/static_cast<double>(RAND_MAX);
                            ikRequest(2,3) = 0.1 * rand()/static_cast<double>(RAND_MAX);
                        }

                        bool left = request.config["left"];
                        bool right = request.config["right"];
                        
                        std::unique_ptr<Sensors> sensors = std::make_unique<Sensors>();
                        sensors->servos = std::vector<Sensors::Servo>(20);

                        if (left) {
                            std::vector<std::pair<ServoID, float> > legJoints = calculateLegJoints<DarwinModel>(ikRequest, Side::LEFT);
                            for (auto& legJoint : legJoints) {
                                ServoID servoID;
                                float position;

                                std::tie(servoID, position) = legJoint;
                                
                                sensors->servos[static_cast<int>(servoID)].presentPosition = position;
                            }
                        }

                        if (right) {
                            std::vector<std::pair<ServoID, float> > legJoints = calculateLegJoints<DarwinModel>(ikRequest, Side::RIGHT);
                            for (auto& legJoint : legJoints) {
                                ServoID servoID;
                                float position;

                                std::tie(servoID, position) = legJoint;
                                
                                sensors->servos[static_cast<int>(servoID)].presentPosition = position;
                            }
                        }
                        std::cout<< "KinematicsNULLTest -calculating forward kinematics." <<std::endl;                
                        arma::mat44 footPosition = calculatePosition<DarwinModel>(*sensors, ServoID::L_ANKLE_ROLL) * utility::math::matrix::yRotationMatrix(-M_PI_2, 4).t();
                        NUClear::log<NUClear::DEBUG>("Forward Kinematics predicts: \n",footPosition);
                        std::cout << "Compared to request: \n" << ikRequest << std::endl;

                        float max_error = 0;
                        for(int i = 0; i <16 ; i++){
                            float error = std::abs(footPosition(i%4, i/4) - ikRequest(i%4, i/4));
                            if (error>max_error) {
                                max_error = error;
                            }
                        }
                        std::cout<< "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<std::endl;                        
                        std::cout<< (max_error < 1e-6 ? "IK TEST PASSED" : "\n\n\n!!!!!!!!!! IK TEST FAILED !!!!!!!!!!\n\n\n" ) << "     (max_error = " << max_error << ")"<<std::endl; 
                        std::cout<< "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<std::endl;       

                        sensors->orientation = arma::eye(3,3);                 
                        emit(std::move(sensors));
                    });

            }
    } // debug
} // modules

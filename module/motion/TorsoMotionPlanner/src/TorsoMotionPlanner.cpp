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
 * Copyright 2016 NUbots <nubots@nubots.net>
 */
/*===========================================================================================================*/
/*----------------------------------------CONSTANTS AND DEFINITIONS------------------------------------------*/
/*===========================================================================================================*/
//      INCLUDE(S)
/*===========================================================================================================*/
#include "TorsoMotionPlanner.h"
/*===========================================================================================================*/
//      NAMESPACE(S)
/*===========================================================================================================*/
namespace module 
{
namespace motion 
{
/*=======================================================================================================*/
//      UTILIZATION REFERENCE(S)
/*=======================================================================================================*/

    using message::input::PushDetection;
    using message::input::ServoID;
    using message::input::Sensors;
    using message::input::LimbID;
    using message::behaviour::ServoCommand;
    using message::behaviour::WalkOptimiserCommand;
    using message::behaviour::WalkConfigSaved;
    // using message::behaviour::RegisterAction;
    // using message::behaviour::ActionPriorites;
    using message::motion::WalkCommand;
    using message::motion::NewStepTargetInfo;
    using message::motion::FootMotionUpdate;
    using message::motion::TorsoMotionUpdate;
    using message::motion::TorsoDestinationUpdate;
    using message::motion::EnableTorsoMotion;
    using message::motion::DisableTorsoMotion;
    using message::motion::ServoTarget;
    using message::motion::Script;
    using message::motion::kinematics::KinematicsModel;
    using message::support::SaveConfiguration;
    using message::support::Configuration;

    using utility::motion::kinematics::calculateLegJoints;
    using utility::math::matrix::Transform2D;
    using utility::math::matrix::Transform3D;
    using utility::math::matrix::Rotation3D;
    using utility::math::angle::normalizeAngle;
    using utility::nubugger::graph;
    using utility::support::Expression;  
/*=======================================================================================================*/
//      NUCLEAR METHOD: TorsoMotionPlanner
/*=======================================================================================================*/
    TorsoMotionPlanner::TorsoMotionPlanner(std::unique_ptr<NUClear::Environment> environment)
    : Reactor(std::move(environment)) 
        , DEBUG(false), DEBUG_ITER(0), initialStep(0)
        , balanceEnabled(0.0), emitLocalisation(false), emitFootPosition(false)
        , updateHandle(), generateStandScriptReaction()
        , updateStepInstruction(false)
        , torsoPositionsTransform(), torsoPositionSource(), torsoPositionDestination()
        , leftFootPositionTransform(), leftFootSource(), rightFootPositionTransform()
        , rightFootSource(), leftFootDestination(), rightFootDestination(), uSupportMass()
        , activeForwardLimb(), activeLimbInitial(LimbID::LEFT_LEG)
        , bodyTilt(0.0), bodyHeight(0.0), stanceLimitY2(0.0), stepTime(0.0), stepHeight(0.0)
        , step_height_slow_fraction(0.0f), step_height_fast_fraction(0.0f)
        , stepLimits(arma::fill::zeros), footOffsetCoefficient(arma::fill::zeros), uLRFootOffset()
        , armLPostureTransform(), armLPostureSource(), armLPostureDestination()
        , armRPostureTransform(), armRPostureSource(), armRPostureDestination()
        , beginStepTime(0.0), destinationTime(), footMotionPhase()
        , STAND_SCRIPT_DURATION(0.0), pushTime(), lastVeloctiyUpdateTime()
        , velocityHigh(0.0), accelerationTurningFactor(0.0), velocityLimits(arma::fill::zeros)
        , accelerationLimits(arma::fill::zeros), accelerationLimitsHigh(arma::fill::zeros)
        , velocityCurrent(), velocityCommand()
        , zmpCoefficients(arma::fill::zeros), zmpParameters(arma::fill::zeros)
        , zmpTime(0.0), phase1Single(0.0), phase2Single(0.0)
        , kinematicsModel()
        , lastFootGoalRotation(), footGoalErrorSum()       
    {
        //Configure foot motion planner...
        on<Configuration>("TorsoMotionPlanner.yaml").then("Torso Motion Planner - Configure", [this] (const Configuration& config) 
        {
            configure(config.config);
        });

        //Define kinematics model for physical calculations...
        on<Trigger<KinematicsModel>>().then("WalkEngine - Update Kinematics Model", [this](const KinematicsModel& model)
        {
            kinematicsModel = model;
        });

        //Transform analytical torso positions in accordance with the stipulated targets...
        updateHandle = on<Every<UPDATE_FREQUENCY, Per<std::chrono::seconds>>, /*With<Sensors>,*/ Single, Priority::HIGH>()
        .then("Torso Motion Planner - Update Torso Position", [this] /*(const Sensors& sensors)*/
        {
            if(DEBUG) { NUClear::log("Messaging: Torso Motion Planner - Update Torso Position(0)"); }
            if((DEBUG_ITER++)%1 == 0)
                {
            if (activeForwardLimb == LimbID::RIGHT_LEG) //TODO: delete
            {
std::cout << "\n\n\n\n\rTMP: RIGHT\n\r";                
            }
            else 
            {
std::cout << "\n\n\n\n\rTMP:  LEFT\n\r";  
            }
std::cout << "ZMPTC: Torso Destination\t[X= " << getTorsoDestination().x() << "]\t[Y= " << getTorsoDestination().y() << "]\n\r";         
std::cout << "ZMPTC: Torso Source\t[X= " << getTorsoSource().x() << "]\t[Y= " << getTorsoSource().y() << "]\n\r"; 
std::cout << "ZMPTC: Support Mass\t[X= " << getSupportMass().x() << "]\t[Y= " << getSupportMass().y() << "]\n\r"; 
std::cout << "ZMPTC: Step Time\t[" << stepTime << "]\n\r"; 
std::cout << "ZMPTC: ZMP Time\t[" << zmpTime << "]\n\r"; 
                }
                    updateTorsoPosition();    
            if(DEBUG) { NUClear::log("Messaging: Torso Motion Planner - Update Torso Position(1)"); }
        }).disable();

        //In the event of a new foot step position info specified by the foot placement planning module...
        on<Trigger<NewStepTargetInfo>>().then("Torso Motion Planner - Received Footstep Info", [this] (const NewStepTargetInfo& info) 
        {            
            if(DEBUG) { NUClear::log("Messaging: Torso Motion Planner - Received Footstep Info(0)"); }
            setDestinationTime(info.targetTime);
            setLeftFootSource(info.leftFootSource);
            setRightFootSource(info.rightFootSource);
            setLeftFootDestination(info.leftFootDestination);
            setRightFootDestination(info.rightFootDestination);
            setSupportMass(info.supportMass); 
activeForwardLimb = info.activeForwardLimb; //TODO: delete
            setTorsoSource(getTorsoDestination());
            zmpTorsoCoefficients(); // Determine Torso Destination and Update FPP for next Foot Step...
            emit(std::make_unique<TorsoDestinationUpdate>(getTorsoDestination()));
           if(DEBUG) { NUClear::log("Messaging: Torso Motion Planner - Received Footstep Info(1)"); }
        });

        //In the process of actuating a foot step and emitting updated positional data...
        on<Trigger<FootMotionUpdate>>().then("Torso Motion Planner - Received Foot Motion Update", [this] (const FootMotionUpdate& info) 
        {            
            if(DEBUG) { NUClear::log("Messaging: Torso Motion Planner - Received Foot Motion Update(0)"); }
            setMotionPhase(info.phase);
            if(DEBUG) { NUClear::log("Messaging: Torso Motion Planner - Received Foot Motion Update(1)"); }
        });

        on<Trigger<EnableTorsoMotion>>().then([this]
        {       
            updateHandle.enable();
        });

        //If torso motion no longer requested, cease updating...
        on<Trigger<DisableTorsoMotion>>().then([this] 
        {
            updateHandle.disable(); 
        });
    }
/*=======================================================================================================*/
//      METHOD: updateTorsoPosition
/*=======================================================================================================*/
    void TorsoMotionPlanner::updateTorsoPosition()
    {
        setTorsoPositionArms(zmpTorsoCompensation(getMotionPhase(), zmpTorsoCoefficients(), getZmpParams(), stepTime, zmpTime, phase1Single, phase2Single, getLeftFootSource(), getRightFootSource()));
//std::cout << "\n\rTorso Position (Arms)\t[X= " << getTorsoPositionArms().x() << "]\t[Y= " << getTorsoPositionArms().y() << "]\n\r";         
        setTorsoPositionLegs(zmpTorsoCompensation(getMotionPhase(), zmpTorsoCoefficients(), getZmpParams(), stepTime, zmpTime, phase1Single, phase2Single, getLeftFootSource(), getRightFootSource()));
//std::cout << "Torso Position (Legs)\t[X= " << getTorsoPositionLegs().x() << "]\t[Y= " << getTorsoPositionLegs().y() << "]\n\r";                 
        Transform2D uTorsoWorld = getTorsoPositionArms().localToWorld({-kinematicsModel.Leg.HIP_OFFSET_X, 0, 0});
        setTorsoPosition3D(arma::vec6({uTorsoWorld.x(), uTorsoWorld.y(), bodyHeight, 0, bodyTilt, uTorsoWorld.angle()}));
        emit(std::make_unique<TorsoMotionUpdate>(getTorsoPositionArms(), getTorsoPositionLegs(), getTorsoPosition3D()));
    }
/*=======================================================================================================*/
//      METHOD: stepTorso
/*=======================================================================================================*/
    Transform2D TorsoMotionPlanner::stepTorso(Transform2D uLeftFoot, Transform2D uRightFoot, double shiftFactor) 
    {
        Transform2D uLeftFootSupport  = uLeftFoot.localToWorld({-getFootOffsetCoefficient(0), -getFootOffsetCoefficient(1), 0});
        Transform2D uRightFootSupport = uRightFoot.localToWorld({-getFootOffsetCoefficient(0), getFootOffsetCoefficient(1), 0});
        return uLeftFootSupport.interpolate(shiftFactor, uRightFootSupport);
    }
/*=======================================================================================================*/
//      METHOD: zmpTorsoCoefficients
/*=======================================================================================================*/
    arma::vec4 TorsoMotionPlanner::zmpTorsoCoefficients()
    {
        arma::vec4 zmpCoefficients;
        setTorsoDestination(stepTorso(getLeftFootDestination(), getRightFootDestination(), 0.5));
        // Compute ZMP coefficients...
        zmpCoefficients.rows(0,1) = zmpSolve(getSupportMass().x(), getTorsoSource().x(), getTorsoDestination().x(), getTorsoSource().x(), getTorsoDestination().x(), phase1Single, phase2Single, stepTime, zmpTime);
        zmpCoefficients.rows(2,3) = zmpSolve(getSupportMass().y(), getTorsoSource().y(), getTorsoDestination().y(), getTorsoSource().y(), getTorsoDestination().y(), phase1Single, phase2Single, stepTime, zmpTime);        
        
        return (zmpCoefficients);
    }
/*=======================================================================================================*/
//      METHOD: zmpSolve
/*=======================================================================================================*/
    arma::vec2 TorsoMotionPlanner::zmpSolve(double zs, double z1, double z2, double x1, double x2, double phase1Single, double phase2Single, double stepTime, double zmpTime) 
    {
        /*
        Solves ZMP equations.
        The resulting form of x is
        x(t) = z(t) + aP*exp(t/zmpTime) + aN*exp(-t/zmpTime) - zmpTime*mi*sinh((t-Ti)/zmpTime)
        where the ZMP point is piecewise linear:
        z(0) = z1, z(T1 < t < T2) = zs, z(stepTime) = z2
        */
        double T1 = stepTime * phase1Single;
        double T2 = stepTime * phase2Single;
        double m1 = (zs - z1) / T1;
        double m2 = -(zs - z2) / (stepTime - T2);

        double c1 = x1 - z1 + zmpTime * m1 * std::sinh(-T1 / zmpTime);
        double c2 = x2 - z2 + zmpTime * m2 * std::sinh((stepTime - T2) / zmpTime);
        double expTStep = std::exp(stepTime / zmpTime);
        double aP = (c2 - c1 / expTStep) / (expTStep - 1 / expTStep);
        double aN = (c1 * expTStep - c2) / (expTStep - 1 / expTStep);
        return {aP, aN};
    }
/*=======================================================================================================*/
//      METHOD: zmpTorsoCompensation
/*=======================================================================================================*/
    Transform2D TorsoMotionPlanner::zmpTorsoCompensation(double phase, arma::vec4 zmpTorsoCoefficients, arma::vec4 zmpParams, double stepTime, double zmpTime, double phase1Single, double phase2Single, Transform2D uLeftFootSource, Transform2D uRightFootSource) 
    {
//std::cout << "Phase: " << phase << "\tstepTime: " << stepTime << "\n\rphase1Single: " << phase1Single << "\n\rphase2Single: " << phase2Single << "\n\r";
std::cout << "\n\rPhase: " << phase << "\n\rzmpcoefficents" << zmpTorsoCoefficients << "\n\rzmpParameters: " << zmpParams << "\n\r";
        //Note that phase is the only variable updated during a step
        Transform2D com = {0, 0, 0};
        double expT = std::exp(stepTime * phase / zmpTime);
        com.x() = getSupportMass().x() + zmpTorsoCoefficients[0] * expT + zmpTorsoCoefficients[1] / expT;
        com.y() = getSupportMass().y() + zmpTorsoCoefficients[2] * expT + zmpTorsoCoefficients[3] / expT;
        if (phase < phase1Single) 
        {
            com.x() += zmpParams[0] * stepTime * (phase - phase1Single) -zmpTime * zmpParams[0] * std::sinh(stepTime * (phase - phase1Single) / zmpTime);
            com.y() += zmpParams[1] * stepTime * (phase - phase1Single) -zmpTime * zmpParams[1] * std::sinh(stepTime * (phase - phase1Single) / zmpTime);
        } 
        else if (phase > phase2Single) 
        {
            com.x() += zmpParams[2] * stepTime * (phase - phase2Single) -zmpTime * zmpParams[2] * std::sinh(stepTime * (phase - phase2Single) / zmpTime);
            com.y() += zmpParams[3] * stepTime * (phase - phase2Single) -zmpTime * zmpParams[3] * std::sinh(stepTime * (phase - phase2Single) / zmpTime);
        }
        // com[2] = .5 * (uLeftFoot[2] + uRightFoot[2]);
        // Linear speed turning
        com.angle() = phase * (getLeftFootDestination().angle() + getRightFootDestination().angle()) / 2 + (1 - phase) * (uLeftFootSource.angle() + uRightFootSource.angle()) / 2;
        return com;
    }
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Time
/*=======================================================================================================*/
    double TorsoMotionPlanner::getTime() 
    {
        if(DEBUG) { NUClear::log("System Time:%f\n\r", double(NUClear::clock::now().time_since_epoch().count()) * (1.0 / double(NUClear::clock::period::den))); }
        return (double(NUClear::clock::now().time_since_epoch().count()) * (1.0 / double(NUClear::clock::period::den)));
    }
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Destination Time
/*=======================================================================================================*/
    double TorsoMotionPlanner::getDestinationTime()
    {
        return (destinationTime);
    }
    void TorsoMotionPlanner::setDestinationTime(double inDestinationTime)
    {
        destinationTime = inDestinationTime;
    }
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Motion Phase
/*=======================================================================================================*/    
    double TorsoMotionPlanner::getMotionPhase()
    {
        return (footMotionPhase);
    }
    void TorsoMotionPlanner::setMotionPhase(double inMotionPhase)  
    {
        footMotionPhase = inMotionPhase;
    } 
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: ZMP Parameters
/*=======================================================================================================*/    
    arma::vec4 TorsoMotionPlanner::getZmpParams()
    {
        setZmpParams
        ({
            (getSupportMass().x() - getTorsoPositionArms().x()) / (stepTime * phase1Single),
            (getTorsoDestination().x() - getSupportMass().x()) / (stepTime * (1 - phase2Single)),
            (getSupportMass().y() - getTorsoPositionArms().y()) / (stepTime * phase1Single),
            (getTorsoDestination().y() - getSupportMass().y()) / (stepTime * (1 - phase2Single)),
        });
        return (zmpParameters);
    }
    void TorsoMotionPlanner::setZmpParams(arma::vec4 inZmpParams)
    {
        zmpParameters = inZmpParams;
    }            
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Torso Position
/*=======================================================================================================*/
    Transform2D TorsoMotionPlanner::getTorsoPositionArms()
    {
        return (torsoPositionsTransform.FrameArms);
    }
    void TorsoMotionPlanner::setTorsoPositionArms(const Transform2D& inTorsoPosition)
    {
        torsoPositionsTransform.FrameArms = inTorsoPosition;
    }    
    Transform2D TorsoMotionPlanner::getTorsoPositionLegs()
    {
        return (torsoPositionsTransform.FrameLegs);
    }        
    void TorsoMotionPlanner::setTorsoPositionLegs(const Transform2D& inTorsoPosition)
    {
        torsoPositionsTransform.FrameLegs = inTorsoPosition;
    }
    Transform3D TorsoMotionPlanner::getTorsoPosition3D()
    {
        return (torsoPositionsTransform.Frame3D);
    }            
    void TorsoMotionPlanner::setTorsoPosition3D(const Transform3D& inTorsoPosition)
    {
        torsoPositionsTransform.Frame3D = inTorsoPosition;
    }    
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Torso Source
/*=======================================================================================================*/
    Transform2D TorsoMotionPlanner::getTorsoSource()
    {
        return (torsoPositionSource);
    }
    void TorsoMotionPlanner::setTorsoSource(const Transform2D& inTorsoSource)
    {
        torsoPositionSource = inTorsoSource;
    }
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Torso Destination
/*=======================================================================================================*/
    Transform2D TorsoMotionPlanner::getTorsoDestination()
    {
        return (torsoPositionDestination);
    }
    void TorsoMotionPlanner::setTorsoDestination(const Transform2D& inTorsoDestination)
    {
        torsoPositionDestination = inTorsoDestination;
    }
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Support Mass
/*=======================================================================================================*/
    Transform2D TorsoMotionPlanner::getSupportMass()
    {
        return (uSupportMass);
    }
    void TorsoMotionPlanner::setSupportMass(const Transform2D& inSupportMass)
    {
        uSupportMass = inSupportMass;
    }    
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Foot Offset Coefficient
/*=======================================================================================================*/
    double TorsoMotionPlanner::getFootOffsetCoefficient(int index)
    {
        return (footOffsetCoefficient[index]);
    }
    void TorsoMotionPlanner::setFootOffsetCoefficient(const arma::vec2& inFootOffsetCoefficient)
    {
        footOffsetCoefficient = inFootOffsetCoefficient;
    }
    void TorsoMotionPlanner::setFootOffsetCoefficient(int index, double inValue)
    {
        footOffsetCoefficient[index] = inValue;
    }    
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Left Foot Position
/*=======================================================================================================*/
    Transform2D TorsoMotionPlanner::getLeftFootPosition()
    {
        return (leftFootPositionTransform);
    }
    void TorsoMotionPlanner::setLeftFootPosition(const Transform2D& inLeftFootPosition)
    {
        leftFootPositionTransform = inLeftFootPosition;
    }
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Right Foot Position
/*=======================================================================================================*/
    Transform2D TorsoMotionPlanner::getRightFootPosition()
    {
        return (rightFootPositionTransform);
    }
    void TorsoMotionPlanner::setRightFootPosition(const Transform2D& inRightFootPosition)
    {
        rightFootPositionTransform = inRightFootPosition;
    }
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Left Foot Source
/*=======================================================================================================*/
    Transform2D TorsoMotionPlanner::getLeftFootSource()
    {
        return (leftFootSource);
    }
    void TorsoMotionPlanner::setLeftFootSource(const Transform2D& inLeftFootSource)
    {
        leftFootSource = inLeftFootSource;
    }
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Right Foot Source
/*=======================================================================================================*/
    Transform2D TorsoMotionPlanner::getRightFootSource()
    {
        return (rightFootSource);
    }
    void TorsoMotionPlanner::setRightFootSource(const Transform2D& inRightFootSource)
    {
        rightFootSource = inRightFootSource;
    }        
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Left Foot Destination
/*=======================================================================================================*/
    Transform2D TorsoMotionPlanner::getLeftFootDestination()
    {
        return (leftFootDestination);
    }
    void TorsoMotionPlanner::setLeftFootDestination(const Transform2D& inLeftFootDestination)
    {
        leftFootDestination = inLeftFootDestination;
    }
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Right Foot Destination
/*=======================================================================================================*/
    Transform2D TorsoMotionPlanner::getRightFootDestination()
    {
        return (rightFootDestination);
    }
    void TorsoMotionPlanner::setRightFootDestination(const Transform2D& inRightFootDestination)
    {
        rightFootDestination = inRightFootDestination;
    }   
/*=======================================================================================================*/
//      METHOD: configure
/*=======================================================================================================*/
    void TorsoMotionPlanner::configure(const YAML::Node& config)
    {
        emitLocalisation = config["emit_localisation"].as<bool>();

        auto& stance = config["stance"];
        bodyHeight = stance["body_height"].as<Expression>();
        bodyTilt = stance["body_tilt"].as<Expression>();
        setFootOffsetCoefficient(stance["foot_offset"].as<arma::vec>());
        // gToe/heel overlap checking values
        stanceLimitY2 = kinematicsModel.Leg.LENGTH_BETWEEN_LEGS() - stance["limit_margin_y"].as<Expression>();

        auto& walkCycle = config["walk_cycle"];
        stepTime = walkCycle["step_time"].as<Expression>();
        zmpTime = walkCycle["zmp_time"].as<Expression>();
        stepHeight = walkCycle["step"]["height"].as<Expression>();
        stepLimits = walkCycle["step"]["limits"].as<arma::mat::fixed<3,2>>();

        step_height_slow_fraction = walkCycle["step"]["height_slow_fraction"].as<float>();
        step_height_fast_fraction = walkCycle["step"]["height_fast_fraction"].as<float>();

        auto& velocity = walkCycle["velocity"];
        velocityLimits = velocity["limits"].as<arma::mat::fixed<3,2>>();
        velocityHigh = velocity["high_speed"].as<Expression>();

        auto& acceleration = walkCycle["acceleration"];
        accelerationLimits = acceleration["limits"].as<arma::vec>();
        accelerationLimitsHigh = acceleration["limits_high"].as<arma::vec>();
        accelerationTurningFactor = acceleration["turning_factor"].as<Expression>();

        phase1Single = walkCycle["single_support_phase"]["start"].as<Expression>();
        phase2Single = walkCycle["single_support_phase"]["end"].as<Expression>();

        auto& balance = walkCycle["balance"];
        balanceEnabled = balance["enabled"].as<bool>();

        STAND_SCRIPT_DURATION = config["STAND_SCRIPT_DURATION"].as<Expression>();
    }    
}  // motion
}  // modules


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
#include "FootMotionPlanner.h"
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
    using message::input::LimbID;
    using message::motion::NewStepTargetInfo;
    using message::motion::FootMotionUpdate;
    using message::motion::EnableFootMotion;
    using message::motion::DisableFootMotion;
    using message::motion::FootStepCompleted;
    using message::support::Configuration;

    using utility::support::Expression;
    using utility::math::matrix::Transform2D;
    using utility::math::matrix::Transform3D;
    using utility::nubugger::graph;
/*=======================================================================================================*/
//      NUCLEAR METHOD: FootMotionPlanner
/*=======================================================================================================*/
    FootMotionPlanner::FootMotionPlanner(std::unique_ptr<NUClear::Environment> environment)
    : Reactor(std::move(environment))
        , DEBUG(false), DEBUG_ITER(0), initialStep(0)
        , balanceEnabled(0.0), emitLocalisation(false), emitFootPosition(false)
        , updateHandle()
        , leftFootPositionTransform(), leftFootSource(), rightFootPositionTransform()
        , rightFootSource(), leftFootDestination(), rightFootDestination()
        , activeForwardLimb(), activeLimbInitial(LimbID::LEFT_LEG)
        , bodyTilt(0.0), bodyHeight(0.0), stepTime(0.0), stepHeight(0.0)
        , step_height_slow_fraction(0.0f), step_height_fast_fraction(0.0f)
        , stepLimits(arma::fill::zeros), footOffset(arma::fill::zeros), uLRFootOffset()
        , beginStepTime(0.0), destinationTime(), lastVeloctiyUpdateTime()
        , velocityHigh(0.0), accelerationTurningFactor(0.0), velocityLimits(arma::fill::zeros)
        , accelerationLimits(arma::fill::zeros), accelerationLimitsHigh(arma::fill::zeros)
        , velocityCurrent()
        , zmpCoefficients(arma::fill::zeros), zmpParameters(arma::fill::zeros)
        , zmpTime(0.0), phase1Single(0.0), phase2Single(0.0)
        , lastFootGoalRotation(), footGoalErrorSum() 
        , startFromStep(false)
    {    	
        //Configure foot motion planner...
        on<Configuration>("FootMotionPlanner.yaml").then("Foot Motion Planner - Configure", [this] (const Configuration& config) 
        {             
            configure(config.config);          
        });

        //Transform analytical foot positions in accordance with the stipulated targets...
        updateHandle = on<Every<UPDATE_FREQUENCY, Per<std::chrono::seconds>>, /*With<Sensors>,*/ Single, Priority::HIGH>()
        .then("Foot Motion Planner - Update Foot Position", [this] /*(const Sensors& sensors)*/
        {
            if(DEBUG) { NUClear::log("Messaging: Foot Motion Planner - Update Foot Position(0)"); }       
            if(isNewStepReceived())
            {
                if((DEBUG_ITER++)%5 == 0)
                {
                    if(DEBUG) { NUClear::log("\rUpdate Foot Position : FMP (%d, %d)\n", leftFootDestination.size(), rightFootDestination.size()); }                  
                }
                updateFootPosition(getMotionPhase(), getLeftFootDestination(), getRightFootDestination());
            }
            if(DEBUG) { NUClear::log("Messaging: Foot Motion Planner - Update Foot Position(1)"); }
        }).disable();

        //In the event of a new foot step target specified by the foot placement planning module...
        on<Trigger<NewStepTargetInfo>>().then("Foot Motion Planner - Received Target Foot Position", [this] (const NewStepTargetInfo& target) 
        {
            if(DEBUG) { NUClear::log("Messaging: Foot Motion Planner - Received Target Foot Position(0)"); }
            setDestinationTime(target.targetTime);
            setActiveForwardLimb(target.activeForwardLimb);
            setVelocityCurrent(target.velocityCurrent);
            if(getActiveForwardLimb() == LimbID::LEFT_LEG)
            {           
                setLeftFootDestination(target.leftFootDestination);
                setLeftFootSource(target.leftFootSource);
            }
            else
            {               
                setRightFootDestination(target.rightFootDestination);
                setRightFootSource(target.rightFootSource);
            }
            if(DEBUG) { NUClear::log("Messaging: Foot Motion Planner - Received Target Foot Position(1)"); }
        });

        //If foot motion is requested, enable updating...
        on<Trigger<EnableFootMotion>>().then([this]
        {            
            updateHandle.enable();
        });

        //If foot motion no longer requested, cease updating...
        on<Trigger<DisableFootMotion>>().then([this] 
        {
            updateHandle.disable(); 
        });
    }
/*=======================================================================================================*/
//      METHOD: updateFootPosition
/*=======================================================================================================*/
    void FootMotionPlanner::updateFootPosition(double phase, const Transform2D& leftFootDestination, const Transform2D& rightFootDestination) 
    {       
        //Instantiate unitless phases for x(=0), y(=1) and z(=2) foot motion...
        arma::vec3 getFootPhases = getFootPhase(phase, phase1Single, phase2Single);

        //Lift foot by amount depending on walk speed
        if(DEBUG) { NUClear::log("Messaging: Foot Motion Planner - getFootPhase limits and calculations"); }
//std::cout << "\n\rVelocity\t[X= " << getVelocityCurrent().x() << "]\t[Y= " << getVelocityCurrent().y() << "]\n\r";        
        auto& limit = (getVelocityCurrent().x() > velocityHigh ? accelerationLimitsHigh : accelerationLimits); // TODO: use a function instead
        float speed = std::min(1.0, std::max(std::abs(getVelocityCurrent().x() / limit[0]), std::abs(getVelocityCurrent().y() / limit[1])));
        float scale = (step_height_fast_fraction - step_height_slow_fraction) * speed + step_height_slow_fraction;
        getFootPhases[2] *= scale;
//std::cout << "\n\rFootPhases\t[0= " << getFootPhases[0] << "]\tFootPhases\t[2= " << getFootPhases[2] << "]\n\r"; 
        if(DEBUG) { NUClear::log("Messaging: Foot Motion Planner - Interpolate Transform2D"); }
        //Interpolate Transform2D from start to destination - deals with flat resolved movement in (x,y) coordinates
        if (getActiveForwardLimb() == LimbID::RIGHT_LEG) 
        {
            //TODO: Vector field function??
            setRightFootPosition(getRightFootSource().interpolate(getFootPhases[0], rightFootDestination));
//std::cout << "\n\n\rRight    Interpolate\t[X= " << (rightFootDestination.x() - getRightFootSource().x()) << "]\t[Y= " << (rightFootDestination.y() - getRightFootSource().y()) << "]\n\r";
//std::cout << "\n\n\rRight      Source\t[X= " << getRightFootSource().x() << "]\t[Y= " << getRightFootSource().y() << "]\n\r";      
//std::cout << "Right Destination\t[X= " << rightFootDestination.x() << "]\t[Y= " << rightFootDestination.y() << "]\n\r"; 
//std::cout << "\n\rRight    Position\t[X= " << getRightFootPosition().x() << "]\t[Y= " << getRightFootPosition().y() << "]\t[A= " << getRightFootPosition().angle() << "]\n\r";              
//std::cout << "Foot       Phases\t[0= " << getFootPhases[0] << "]\t[1= " << getFootPhases[1] << "]\t[2= " << getFootPhases[2] << "]\n\r";  
//std::cout << "Right FMP\t[X= " << getRightFootPosition().x() << "]\t[Y= " << getRightFootPosition().y() << "]\n\r";          
        }
        else
        {
            //TODO: Vector field function??
            setLeftFootPosition(getLeftFootSource().interpolate(getFootPhases[0],   leftFootDestination));
//std::cout << "\n\n\rLeft     Interpolate\t[X= " << (leftFootDestination.x() - getLeftFootSource().x()) << "]\t[Y= " << (leftFootDestination.y() - getLeftFootSource().y()) << "]\n\r";
//std::cout << "\n\n\rLeft       Source\t[X= " << getLeftFootSource().x() << "]\t[Y= " << getLeftFootSource().y() << "]\n\r";      
//std::cout << "Left  Destination\t[X= " << leftFootDestination.x() << "]\t[Y= " << leftFootDestination.y() << "]\n\r"; 
//std::cout << "\n\rLeft     Position\t[X= " << getLeftFootPosition().x() << "]\t[Y= " << getLeftFootPosition().y() << "]\t[A= " << getLeftFootPosition().angle() << "]\n\r";                
//std::cout << "Foot       Phases\t[0= " << getFootPhases[0] << "]\t[1= " << getFootPhases[1] << "]\t[2= " << getFootPhases[2] << "]\n\r"; 
//std::cout << "Left  FMP\t[X= " << getLeftFootPosition().x() << "]\t[Y= " << getLeftFootPosition().y() << "]\n\r";  

        }
        
        if(DEBUG) { NUClear::log("Messaging: Foot Motion Planner - Instantiate FootLocal Variables"); }
        //Translates foot motion into z dimension for stepping in three-dimensional space...
        Transform3D leftFootLocal  = Transform2D({ 5, -1,  0.5}); //getLeftFootPosition();
        Transform3D rightFootLocal = Transform2D({-5,  1, -0.5}); //getRightFootPosition();
std::cout << "\n\rMWE: Left     Foot\n\r\t[\n\r\t" << leftFootLocal  << "\t]";  
std::cout << "\n\rMWE: Right    Foot\n\r\t[\n\r\t" << rightFootLocal << "\t]";          

        if(DEBUG) { NUClear::log("Messaging: Foot Motion Planner - Translate Z for support foot"); }
        //Lift swing leg - manipulate(update) z component of foot position to action movement with a varying altitude locus...
        if (getActiveForwardLimb() == LimbID::RIGHT_LEG) 
        {
            //TODO: Vector field function??
            rightFootLocal = rightFootLocal.translateZ(stepHeight * getFootPhases[2]);
        }
        else
        {
            //TODO: Vector field function??
            leftFootLocal  = leftFootLocal.translateZ(stepHeight  * getFootPhases[2]);
        }     

        //DEBUGGING: Emit relative feet position phase with respect to robot state... 
        if (emitFootPosition)
        {
            emit(graph("Foot Phase Motion", phase));
        }

        if(DEBUG) { NUClear::log("Messaging: Foot Motion Planner - Emit FootMotionUpdate"); }
        //Broadcast struct of updated foot motion data at corresponding phase identity...
        emit(std::make_unique<FootMotionUpdate>(phase, leftFootLocal, rightFootLocal));              
    }
/*=======================================================================================================*/
//      METHOD: getFootPhase
/*=======================================================================================================*/
    arma::vec3 FootMotionPlanner::getFootPhase(double phase, double phase1Single, double phase2Single) 
    {
        // Computes relative x,z motion of foot during single support phase
        // phSingle = 0: x=0, z=0, phSingle = 1: x=1,z=0
        double phaseSingle = std::min(std::max(phase - phase1Single, 0.0) / (phase2Single - phase1Single), 1.0);
        double phaseSingleSkew = std::pow(phaseSingle, 0.8) - 0.17 * phaseSingle * (1 - phaseSingle);
        double xf = 0.5 * (1 - std::cos(M_PI * phaseSingleSkew));
        double zf = 0.5 * (1 - std::cos(2 * M_PI * phaseSingleSkew));
        return {xf, phaseSingle, zf};
    }
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Time
/*=======================================================================================================*/
    double FootMotionPlanner::getTime() 
    {
        if(DEBUG) { NUClear::log("System Time:%f\n\r", double(NUClear::clock::now().time_since_epoch().count()) * (1.0 / double(NUClear::clock::period::den))); }
        return (double(NUClear::clock::now().time_since_epoch().count()) * (1.0 / double(NUClear::clock::period::den)));
    }
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Destination Time
/*=======================================================================================================*/
    double FootMotionPlanner::getDestinationTime()
    {
        if(destinationTime.size() > 0)
        {
            return (destinationTime.front());
        }
        else
        {
            return (0); //DEBUGGING: blank value
        }
    }
    void FootMotionPlanner::setDestinationTime(double inDestinationTime)
    {
        destinationTime.push(inDestinationTime);
    }
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Velocity
/*=======================================================================================================*/
    Transform2D FootMotionPlanner::getVelocityCurrent() 
    {
        return (velocityCurrent);
    }
    void FootMotionPlanner::setVelocityCurrent(Transform2D inVelocityCurrent) 
    {
        velocityCurrent = inVelocityCurrent;
    }       
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Left Foot Position
/*=======================================================================================================*/
    Transform2D FootMotionPlanner::getLeftFootPosition()
    {
        return (leftFootPositionTransform);
    }
    void FootMotionPlanner::setLeftFootPosition(const Transform2D& inLeftFootPosition)
    {
        leftFootPositionTransform = inLeftFootPosition;
    }
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Right Foot Position
/*=======================================================================================================*/
    Transform2D FootMotionPlanner::getRightFootPosition()
    {
        return (rightFootPositionTransform);
    }
    void FootMotionPlanner::setRightFootPosition(const Transform2D& inRightFootPosition)
    {
        rightFootPositionTransform = inRightFootPosition;
    }    
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Left Foot Source
/*=======================================================================================================*/
    Transform2D FootMotionPlanner::getLeftFootSource()
    {
        return (leftFootSource);
    }
    void FootMotionPlanner::setLeftFootSource(const Transform2D& inLeftFootSource)
    {
        leftFootSource = inLeftFootSource;
    }
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Right Foot Source
/*=======================================================================================================*/
    Transform2D FootMotionPlanner::getRightFootSource()
    {
        return (rightFootSource);
    }
    void FootMotionPlanner::setRightFootSource(const Transform2D& inRightFootSource)
    {
        rightFootSource = inRightFootSource;
    }
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Left Foot Destination
/*=======================================================================================================*/
    Transform2D FootMotionPlanner::getLeftFootDestination()
    {
        if(leftFootDestination.size() > 0)
        {
            return (leftFootDestination.front());
        }
        else
        {
            return (Transform2D({0, 0, 0})); //DEBUGGING: blank value
        }
    }
    void FootMotionPlanner::setLeftFootDestination(const Transform2D& inLeftFootDestination)
    {
        leftFootDestination.push(inLeftFootDestination);
    }
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Right Foot Destination
/*=======================================================================================================*/
    Transform2D FootMotionPlanner::getRightFootDestination()
    {
        if(rightFootDestination.size() > 0)
        {
            return (rightFootDestination.front());
        }
        else
        {
            return (Transform2D({0, 0, 0})); //DEBUGGING: blank value
        }
    }
    void FootMotionPlanner::setRightFootDestination(const Transform2D& inRightFootDestination)
    {
        rightFootDestination.push(inRightFootDestination);
    }
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Active Forward Limb
/*=======================================================================================================*/
    LimbID FootMotionPlanner::getActiveForwardLimb()
    {
        if(activeForwardLimb.size() > 0)
        {
            return (activeForwardLimb.front());
        }
        else
        {
            return (activeLimbInitial); //DEBUGGING: blank value
        }
    }
    void FootMotionPlanner::setActiveForwardLimb(LimbID inActiveForwardLimb)
    {
        activeForwardLimb.push(inActiveForwardLimb);
    }    
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: New Step Received
/*=======================================================================================================*/
    bool FootMotionPlanner::isNewStepReceived()
    {
        return ((leftFootDestination.size() > 0) | (rightFootDestination.size() > 0));
    }
/*=======================================================================================================*/
//      ENCAPSULATION METHOD: Motion Phase
/*=======================================================================================================*/
    double FootMotionPlanner::getMotionPhase()
    {
        // Obtain current system time...
        double currentTime = getTime();
        // The percentage completed of the current step, range: [0,1]...
        double motionPhase = 1 - ((getDestinationTime() - currentTime) / stepTime);        
        // Bind phase value to range [0,1], emit status if step completed...
        if (motionPhase > 1)
        {
            motionPhase = std::fmod(motionPhase, 1);
            // Consume completed step instruction...
            if (getActiveForwardLimb() == LimbID::LEFT_LEG) 
            {
                if(leftFootDestination.size() > 0)
                {                
                    leftFootDestination.pop();
                }
            }
            else
            {
                if(rightFootDestination.size() > 0)
                {                  
                    rightFootDestination.pop();
                }
            }      
            activeForwardLimb.pop();
            destinationTime.pop();
            // If there has already been an updated instruction, then process before requesting new data...
            if(!isNewStepReceived())
            {
                // Notify helper modules of completed footstep (trigger request for new step instruction)...
                emit(std::make_unique<FootStepCompleted>(true)); 
            }
        }
        return (motionPhase);
    }
/*=======================================================================================================*/
//      METHOD: Configuration
/*=======================================================================================================*/
    void FootMotionPlanner::configure(const YAML::Node& config)
    {
        emitLocalisation = config["emit_localisation"].as<bool>();
        emitFootPosition = config["emit_foot_position"].as<bool>();

        auto& stance = config["stance"];
        bodyHeight = stance["body_height"].as<Expression>();
        bodyTilt = stance["body_tilt"].as<Expression>();
        footOffset = stance["foot_offset"].as<arma::vec>();

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
    }
}  // motion
}  // modules

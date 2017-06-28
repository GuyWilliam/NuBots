#include "BallLocalisation.h"
#include <chrono>
#include "extension/Configuration.h"
#include "utility/time/time.h"

#include "message/localisation/FieldObject.h"
#include "message/vision/VisionObjects.h"
#include "message/support/FieldDescription.h"
#include "message/input/Sensors.h"


#include "utility/support/eigen_armadillo.h"

namespace module {
namespace localisation {

    using extension::Configuration;
    using utility::time::TimeDifferenceSeconds;
    using message::localisation::Ball;
    using message::support::FieldDescription;
    using message::input::Sensors;

    BallLocalisation::BallLocalisation(std::unique_ptr<NUClear::Environment> environment)
    : Reactor(std::move(environment))
    , filter() {

        on<Configuration>("BallLocalisation.yaml").then([this] (const Configuration&) {
            auto message = std::make_unique<std::vector<Ball>>();
        	emit(message);
            emit(std::make_unique<Ball>());

            // Use configuration here from file BallLocalisation.yaml
        });

        /* To run at something like 100Hz that will call Time Update */
        on<Every<100, Per<std::chrono::seconds>>, Sync<BallLocalisation>
         , With<FieldDescription>
         , With<Sensors>>().then("BallLocalisation Time", [this](
            const FieldDescription& field
            , const Sensors& sensors){
            /* Perform time update */
            auto curr_time = NUClear::clock::now();
            double seconds = TimeDifferenceSeconds(curr_time,last_time_update_time);
            last_time_update_time = curr_time;
            filter.timeUpdate(seconds);

            /* Creating ball state vector and covariance matrix for emission */
            //std::unique_ptr ball;
            auto ball = std::make_unique<Ball>();
            ball->locObject.position = convert<double,2>(filter.get());
            ball->locObject.position_cov = convert<double,2,2>(filter.getCovariance());
            ball->locObject.last_measurement_time = last_measurement_update_time;

            /* For graphing purposes */
            arma::vec3 rBWw = { ball->locObject.position[0], ball->locObject.position[1], field.ball_radius };
            // Get our transform to world coordinates
            const Transform3D& Htw = convert<double, 4, 4>(sensors.world);
            const Transform3D& Htc = convert<double, 4, 4>(sensors.forwardKinematics.at(ServoID::HEAD_PITCH)); 
            Transform3D Hcw = Htc.i() * Htw;
            arma::vec3 rBCc_cart = Hcw.transformPoint(rBWw);
            arma::vec3 rBCc_sph1 = cartesianToSpherical(rBCc_cart); // in r,theta,phi
            arma::vec3 rBCc_sph2 = { 1/rBCc_sph1[0], rBCc_sph1[1], rBCc_sph1[2] };  // in roe, theta, phi, where roe is 1/r
            emit(graph("state_roe", rBCc_sph2[0]));
            emit(graph("state_theta", rBCc_sph2[1]));
            emit(graph("state_phi", rBCc_sph2[2]));
            emit(ball);
        });

        /* To run whenever a ball has been detected */
        on<Trigger<std::vector<message::vision::Ball>>
         , With<FieldDescription>
         , With<Sensors>>().then([this](
            const std::vector<message::vision::Ball>& balls
            , const FieldDescription& field
            , const Sensors& sensors){

                double quality = 1.0;   // I don't know what quality should be used for
                if(balls.size() > 0){
                    /* Call Time Update first */
                    auto curr_time = NUClear::clock::now();
                    double seconds = TimeDifferenceSeconds(curr_time,last_time_update_time);
                    last_time_update_time = curr_time;
                    filter.timeUpdate(seconds);

                    /* Now call Measurement Update. Supports multiple measurement methods and will treat them as separate measurements */
                    for (auto& measurement : balls[0].measurements) {
                        /* For graphing purposes */
                        arma::vec3 rBCc = convert<double, 3, 1>(measurement.rBCc);
                        emit(graph("measured_roe", rBCc[0]));
                        emit(graph("measured_theta", rBCc[1]));
                        emit(graph("measured_phi", rBCc[2]));

                        quality *= filter.measurementUpdate(convert<double, 3, 1>(measurement.rBCc),convert<double, 3, 3>(measurement.covariance), field, sensors);
                    }
                    
                    /* Creating ball state vector and covariance matrix for emission */
                    //std::unique_ptr ball;
                    auto ball = std::make_unique<Ball>();
                    ball->locObject.position = convert<double,2>(filter.get());
                    ball->locObject.position_cov = convert<double,2,2>(filter.getCovariance());
                    ball->locObject.last_measurement_time = curr_time;
                    last_measurement_update_time = curr_time;

                    /* For graphing purposes */
                    arma::vec3 rBWw = { ball->locObject.position[0], ball->locObject.position[1], field.ball_radius };
                    // Get our transform to world coordinates
                    const Transform3D& Htw = convert<double, 4, 4>(sensors.world);
                    const Transform3D& Htc = convert<double, 4, 4>(sensors.forwardKinematics.at(ServoID::HEAD_PITCH)); 
                    Transform3D Hcw = Htc.i() * Htw;
                    arma::vec3 rBCc_cart = Hcw.transformPoint(rBWw);
                    arma::vec3 rBCc_sph1 = cartesianToSpherical(rBCc_cart); // in r,theta,phi
                    arma::vec3 rBCc_sph2 = { 1/rBCc_sph1[0], rBCc_sph1[1], rBCc_sph1[2] };  // in roe, theta, phi, where roe is 1/r
                    emit(graph("state_roe", rBCc_sph2[0]));
                    emit(graph("state_theta", rBCc_sph2[1]));
                    emit(graph("state_phi", rBCc_sph2[2]));
                    emit(ball);
                }
        });
    }
}
}

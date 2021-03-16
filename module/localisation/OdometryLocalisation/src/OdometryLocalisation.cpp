#include "OdometryLocalisation.hpp"

#include "extension/Configuration.hpp"

#include "message/behaviour/Nod.hpp"
#include "message/input/Sensors.hpp"
#include "message/localisation/Field.hpp"
#include "message/platform/darwin/DarwinSensors.hpp"

#include "utility/localisation/transform.hpp"
#include "utility/nusight/NUhelpers.hpp"
#include "utility/support/yaml_expression.hpp"

namespace module {
namespace localisation {

    using extension::Configuration;
    using message::behaviour::Nod;
    using message::input::Sensors;
    using message::localisation::Field;
    using message::platform::darwin::ButtonLeftDown;
    using utility::localisation::projectTo2D;
    using utility::nusight::graph;
    using utility::support::Expression;

    OdometryLocalisation::OdometryLocalisation(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment)), localisationOffset(Eigen::Affine2d::Identity()) {

        on<Configuration>("OdometryLocalisation.yaml").then([this](const Configuration& config) {
            // Use configuration here from file OdometryLocalisation.yaml
            localisationOffset.matrix() = config["localisationOffset"].as<Expression>();
        });

        on<Trigger<ButtonLeftDown>, Single, With<Sensors>, Sync<OdometryLocalisation>>().then(
            [this](const Sensors& sensors) {
                NUClear::log("Localisation Orientation reset. This direction is now forward.");
                emit(std::make_unique<Nod>(true));
                // Set localisationOffset = Hrw
                localisationOffset =
                    projectTo2D(Eigen::Affine3d(sensors.Htw), Eigen::Vector3d::UnitZ(), Eigen::Vector3d::UnitX());
            });


        on<Trigger<Sensors>, Sync<OdometryLocalisation>, Single>().then("Odometry Loc", [this](const Sensors& sensors) {
            Eigen::Affine2d Hrw =
                projectTo2D(Eigen::Affine3d(sensors.Htw), Eigen::Vector3d::UnitZ(), Eigen::Vector3d::UnitX());
            Eigen::Affine2d Hwr = Hrw.inverse();

            // Assign the local to world transform to `state`, which becomes the field's position transform
            Eigen::Affine2d state;
            state.translation() =
                localisationOffset.translation() + (localisationOffset.rotation() * Hwr.translation());
            state.linear() = localisationOffset.rotation() * Hwr.rotation();

            auto field        = std::make_unique<Field>();
            field->position   = state.matrix();
            field->covariance = Eigen::Matrix3d::Identity();

            emit(std::make_unique<std::vector<Field>>(1, *field));
            emit(field);
        });
    }
}  // namespace localisation
}  // namespace module

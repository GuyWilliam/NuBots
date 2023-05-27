#include "KickToGoal.hpp"

#include "extension/Behaviour.hpp"
#include "extension/Configuration.hpp"

#include "message/input/Sensors.hpp"
#include "message/localisation/Field.hpp"
#include "message/planning/KickTo.hpp"
#include "message/strategy/KickToGoal.hpp"
#include "message/support/FieldDescription.hpp"

namespace module::strategy {

    using extension::Configuration;
    using KickToGoalTask = message::strategy::KickToGoal;
    using message::input::Sensors;
    using message::localisation::Field;
    using message::planning::KickTo;
    using message::support::FieldDescription;

    KickToGoal::KickToGoal(std::unique_ptr<NUClear::Environment> environment)
        : BehaviourReactor(std::move(environment)) {

        on<Configuration>("KickToGoal.yaml").then([this](const Configuration& config) {
            // Use configuration here from file KickToGoal.yaml
            this->log_level = config["log_level"].as<NUClear::LogLevel>();
        });

        on<Provide<KickToGoalTask>,
           With<Field>,
           With<Sensors>,
           With<FieldDescription>,
           Every<30, Per<std::chrono::seconds>>>()
            .then([this](const Field& field, const Sensors& sensors, const FieldDescription& field_description) {
                // Get the robot's position (pose) on the field
                Eigen::Isometry3d Hrf = Eigen::Isometry3d(sensors.Hrw) * Eigen::Isometry3d(field.Hfw.inverse());

                // Get the goal position relative to the torso to kick to
                Eigen::Vector3d rGFf = Eigen::Vector3d(-field_description.dimensions.field_length / 2.0,
                                                       0.0,
                                                       0.0);  // convert to torso space
                Eigen::Vector3d rGRr = Hrf * rGFf;

                emit<Task>(std::make_unique<KickTo>(rGRr));  // kick the ball if possible
            });
    }

}  // namespace module::strategy

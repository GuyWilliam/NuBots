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
 * Copyright 2013 NUBots <nubots@nubots.net>
 */

#include "NUbugger.h"

#include <yaml-cpp/yaml.h>

#include "messages/support/nubugger/proto/Message.pb.h"
#include "utility/time/time.h"
#include "utility/file/fileutil.h"

/**
* @author Monica Olejniczak
*/
namespace modules {
namespace support {
    using utility::file::listFiles;
    using messages::support::nubugger::proto::Message;
    using utility::time::getUtcTimestamp;
    using messages::support::nubugger::proto::ConfigurationState;

    void processNode(ConfigurationState::Node& proto, YAML::Node& yaml);

    /**
     * Processes a null YAML node by setting its respective type on the protocol node.
     *
     * @param proto The protocol node.
     */
    void processNullNode(ConfigurationState::Node& proto) {
        // set the type of the protocol node to the NULL_VALUE Node Type
        proto.set_type(ConfigurationState::Node::NULL_VALUE);
    }

    /**
     * Processes a scalar YAML node by setting its respective type on the protocol node. A scalar node may comprise a
     * boolean, number or string.
     *
     * @param proto The protocol node.
     * @param yaml A scalar YAML node.
     */
    void processScalarNode(ConfigurationState::Node& proto, YAML::Node& yaml) {
        bool bValue;
        // check if the yaml node is a boolean
        if (YAML::convert<bool>::decode(yaml, bValue)) {
            // set the type of the protocol node to the BOOLEAN Node Type and escape the case
            proto.set_type(ConfigurationState::Node::BOOLEAN);
            // TODO set value!!!!!!
            return;
        }
        long lValue;
        // check if the yaml node is a long
        if (YAML::convert<long>::decode(yaml, lValue)) {
            // set the type of the protocol node to the LONG Node Type and escape the case
            proto.set_type(ConfigurationState::Node::LONG);
            return;
        }
        double dValue;
        // check if the yaml node is a double
        if (YAML::convert<double>::decode(yaml, dValue)) {
            // set the type of the protocol node to the DOUBLE Node Type and escape the case
            proto.set_type(ConfigurationState::Node::DOUBLE);
            return;
        }
        // set the type of the protocol node to the STRING Node Type and escape the case
        proto.set_type(ConfigurationState::Node::STRING);
    }

    /**
     * Processes a sequence YAML node by settings its respective type on the protocol node. It then iterates through
     * all the nodes in this sequence and processes each node.
     *
     * @param proto The protocol node.
     * @param yaml A sequence YAML node.
     */
    void processSequenceNode(ConfigurationState::Node& proto, YAML::Node& yaml) {
        // set the type of the protocol node to the SEQUENCE Node Type
        proto.set_type(ConfigurationState::Node::SEQUENCE);
        // iterate through every yaml node in the sequence
        for (auto&& yamlNode : yaml) {
            // recursively call this function where the protocol node is a new sequence value and the yaml
            // node is the current iteration within the list
            processNode(*proto.add_sequence_value(), yamlNode);
        }
    }

    /**
     * Processes a map YAML node by setting its respective type on the protocol node. It then iterates through all the
     * nodes in this map and processes each node.
     *
     * @param proto The protocol node.
     * @param yaml A map YAML node.
     */
    void processMapNode(ConfigurationState::Node& proto, YAML::Node& yaml) {
        // set the type of the protocol node to the MAP Node Type
        proto.set_type(ConfigurationState::Node::MAP);
        // iterate through every yaml node in the map
        for (auto&& yamlNode : yaml) {
            // create a new map value from the protocol node
            auto* map = proto.add_map_value();
            // set the name of this new node to the key of the yaml node and convert it to a string
            map->set_name(yamlNode.first.as<std::string>());
            // recursively call this function with a pointer to the object that is a part of the protocol
            // buffer as its first parameter and use the value of the yaml node for the second parameter
            processNode(*map->mutable_value(), yamlNode.second);
        }
    }

    /**
     * Processes a particular YAML node into its equivalent protocol node. The tag is initially set and the type of the
     * YAML node is then evaluated.
     *
     * @param proto The protocol node.
     * @param yaml A YAML node.
     */
    void processNode(ConfigurationState::Node& proto, YAML::Node& yaml) {
        // set the tag of the protocol buffer
        proto.set_tag(yaml.Tag());
        switch (yaml.Type()) {
            case YAML::NodeType::Undefined:
                // TODO Handle it somehow?
                break;
            case YAML::NodeType::Null:
                processNullNode(proto);
                break;
            // strings and numbers
            case YAML::NodeType::Scalar:
                processScalarNode(proto, yaml);
                break;
            // arrays and lists
            case YAML::NodeType::Sequence:
                processSequenceNode(proto, yaml);
                break;
            // hashes and dictionaries
            case YAML::NodeType::Map:
                processMapNode(proto, yaml);
                break;
        }
    }

    void NUbugger::sendConfigurationState() {
        // get the list of file paths in the shared config directory and ensure it is recursive
        std::vector<std::string> paths = listFiles("config", true);
        // create a new message
        Message message;
        message.set_type(Message::CONFIGURATION_STATE);         // set the message type to the configuration state
        message.set_filter_id(0);                               // ensure the message is not filtered
        message.set_utc_timestamp(getUtcTimestamp());           // set the timestamp of the message

        auto* state = message.mutable_configuration_state();    // create the configuration state from the message
        auto* root = state->mutable_root();                     // retrieve the root node from the state
        root->set_type(ConfigurationState::Node::DIRECTORY);    // set the Node Type of the root to a DIRECTORY

        // TODO: handle directories w/o root
        for (auto&& path : paths) {                             // iterate through every file path in the config directory
            YAML::Node yaml = YAML::LoadFile(path);             // load the YAML file using the current iteration
            auto* keymap = root->add_map_value();               // add a new map from the root node
            keymap->set_name(path);                             // set the name of the root to the path name
            processNode(*keymap->mutable_value(), yaml);        // processes the yaml node into a protocol node
        }

        send(message);

        // TICK: search through config directory -> config/*.yaml accounting for sub-directories
        // parse yaml (load using yaml.cpp)
        // make protocol buffer tree and send over network to nubugger
    }
}
}
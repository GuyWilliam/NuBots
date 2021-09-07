/*
 * Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef EXTENSION_CONFIGURATION_HPP
#define EXTENSION_CONFIGURATION_HPP

#include <cstdlib>
#include <filesystem>
#include <nuclear>
#include <yaml-cpp/yaml.h>

#include "FileWatch.hpp"

#include "utility/file/fileutil.hpp"
#include "utility/strutil/strutil.hpp"
#include "utility/support/hostname.hpp"
#include "utility/support/yaml_log_level.hpp"

namespace fs = ::std::filesystem;

namespace extension {

    /**
     * @author Trent Houliston
     */
    struct [[nodiscard]] Configuration {
        // Rules:
        // 1) Default config file should define a value for every node.
        // 2) Per-robot config overrides default config values. This file need only override the values that nee to be
        // overriden.
        // 3) Per-binary config overrides per-robot and default config values. This file need only override the values
        // that need to be overriden.
        //
        // Per-robot and per-binary files need not exist.
        // Per-robot and per-binary files can add new nodes to the file, but this is probably unwise.
        //
        // We have to merge the YAML trees to account for situations where a sub-node is not defined in a higher
        // priority tree.

        fs::path fileName{};
        std::string hostname{};
        std::string binary{};
        YAML::Node config{};

        Configuration() = default;
        Configuration(const std::string& fileName,
                      const std::string& hostname,
                      const std::string& binary,
                      const YAML::Node& config)
            : fileName(fileName), hostname(hostname), binary(binary), config(config) {}

        /// @brief Constructor without config node given. The correct config file has to be deduced from the params
        Configuration(const std::string& fileName, const std::string& hostname, const std::string& binary)
            : fileName(fileName), hostname(hostname), binary(binary) {
            bool loaded = false;

            // Load the default config file.
            if (fs::exists(fs::path("config") / fileName)) {
                config = YAML::LoadFile(fs::path("config") / fileName);
                loaded = true;
            }

            // If the same file exists in this robots per-robot config directory then load and merge.
            if (fs::exists(fs::path("config") / hostname / fileName)) {
                if (loaded) {
                    config = mergeYAML(config, YAML::LoadFile(fs::path("config") / hostname / fileName));
                }

                else {
                    config = YAML::LoadFile(fs::path("config") / hostname / fileName);
                    loaded = true;
                }
            }

            // If the same file exists in this binary's per-binary config directory then load and merge.
            if (fs::exists(fs::path("config") / binary / fileName)) {
                if (loaded) {
                    config = mergeYAML(config, YAML::LoadFile(fs::path("config") / binary / fileName));
                }

                else {
                    config = YAML::LoadFile(fs::path("config") / binary / fileName);
                }
            }
        }

        [[nodiscard]] static YAML::Node mergeYAML(const YAML::Node& base, const YAML::Node& override) {
            YAML::Node ret(base);

            for (const auto& item : override) {
                const std::string& key = item.first.as<std::string>();

                // If the key doesn't exist in the base set then add it and move on.
                if (!base[key]) {
                    ret[key] = item.second;
                }

                // If the key does exist then we must go deeper.
                else {
                    // The type dictates whether we need a recursive call or not.
                    switch (item.second.Type()) {
                        // Just a raw value (int, double, etc)
                        case YAML::NodeType::Scalar: {
                            ret[key] = item.second;
                            break;
                        }

                        // Essentially a vector.
                        case YAML::NodeType::Sequence: {
                            ret[key] = item.second;
                            break;
                        }

                        // Recurse.
                        case YAML::NodeType::Map: {
                            ret[key] = mergeYAML(base[key], override[key]);
                            break;
                        }

                        // Its nothing, so overwrite (unset) the value.
                        // Is this really the intended behaviour?
                        case YAML::NodeType::Null: [[fallthrough]];
                        case YAML::NodeType::Undefined: [[fallthrough]];
                        default: {
                            NUClear::log<NUClear::WARN>("Unsetting key",
                                                        "'" + key + "'",
                                                        "in YAML file. Is this what you intended?");
                            ret[key] = item.second;
                            break;
                        }
                    }
                }
            }

            return ret;
        }

        [[nodiscard]] Configuration operator[](const std::string& key) {
            return Configuration(fileName, hostname, binary, config[key]);
        }

        [[nodiscard]] const Configuration operator[](const std::string& key) const {
            return Configuration(fileName, hostname, binary, config[key]);
        }

        [[nodiscard]] Configuration operator[](const char* key) {
            return Configuration(fileName, hostname, binary, config[key]);
        }

        [[nodiscard]] const Configuration operator[](const char* key) const {
            return Configuration(fileName, hostname, binary, config[key]);
        }

        [[nodiscard]] Configuration operator[](size_t index) {
            return Configuration(fileName, hostname, binary, config[index]);
        }

        [[nodiscard]] const Configuration operator[](size_t index) const {
            return Configuration(fileName, hostname, binary, config[index]);
        }

        [[nodiscard]] Configuration operator[](int index) {
            return Configuration(fileName, hostname, binary, config[index]);
        }

        [[nodiscard]] const Configuration operator[](int index) const {
            return Configuration(fileName, hostname, binary, config[index]);
        }

        template <typename T, typename... Args>
        [[nodiscard]] T as(Args&&... args) const {
            return config.as<T>(std::forward<Args>(args)...);
        }

        // Allow iterating through configuration
        [[nodiscard]] YAML::iterator begin() {
            return config.begin();
        }
        [[nodiscard]] YAML::const_iterator begin() const {
            return config.begin();
        }
        [[nodiscard]] YAML::iterator end() {
            return config.end();
        }
        [[nodiscard]] YAML::const_iterator end() const {
            return config.end();
        }

        // All of these disables for this template are because the std::string constructor is magic and screwy
        template <typename T,
                  typename Decayed = typename std::decay<T>::type,
                  typename         = typename std::enable_if<!std::is_same<const char*, Decayed>::value
                                                     && !std::is_same<std::allocator<char>, Decayed>::value
                                                     && !std::is_same<std::initializer_list<char>, Decayed>::value
                                                     && !std::is_same<char, Decayed>::value>::type>
        [[nodiscard]] operator T() const {
            return config.as<T>();
        }

        // The conversion for string is fully specialised because strings get screwy
        // because of their auto conversion to const char* etc
        [[nodiscard]] operator std::string() const {
            return config.as<std::string>();
        }
    };

}  // namespace extension

// NUClear configuration extension
namespace NUClear::dsl {
    namespace operation {

        [[nodiscard]] inline std::string getFirstCommandLineArg() {
            std::shared_ptr<const message::CommandLineArguments> args =
                store::DataStore<message::CommandLineArguments>::get();
            if (!args->empty()) {
                std::vector<char> data(args->at(0).cbegin(), args->at(0).cend());
                data.push_back('\0');
                return std::string(data.data());
            }
            return std::string{};  // Empty string
        }

        template <>
        struct DSLProxy<::extension::Configuration> {
            template <typename DSL>
            static inline void bind(const std::shared_ptr<threading::Reaction>& reaction, const fs::path& path) {
                auto flags = ::extension::FileWatch::RENAMED | ::extension::FileWatch::CHANGED;

                // Get hostname so we can find the correct per-robot config directory.
                const std::string hostname = utility::support::getHostname();

                // Check if there is a default config. If there isn't, try to make one
                const fs::path defaultConfig = fs::path("config") / path;
                if (!fs::exists(defaultConfig)) {
                    NUClear::log<NUClear::WARN>("Configuration file '" + defaultConfig.string()
                                                + "' does not exist. Creating it.");

                    // Check for a directory.
                    if (fs::is_directory(defaultConfig)) {
                        fs::create_directory(defaultConfig);
                    }

                    else {
                        std::ofstream ofs(defaultConfig.string());
                        if (!ofs.is_open()) {
                            throw std::runtime_error("Failed creating file '" + path.string() + "'.");
                        }
                        ofs.close();
                    }
                }

                // Bind our default path
                DSLProxy<::extension::FileWatch>::bind<DSL>(reaction, defaultConfig, flags);

                // Bind our robot specific path if it exists
                const fs::path robotConfig = fs::path("config") / hostname / path;
                if (fs::exists(robotConfig)) {
                    DSLProxy<::extension::FileWatch>::bind<DSL>(reaction, robotConfig, flags);
                }

                // If there were command line arguments, we can get the binary name, and check for a binary config
                // If not, we don't bother checking for a binary config to bind
                const auto binaryName = getFirstCommandLineArg();
                if (binaryName != "") {
                    fs::path binaryConfig = fs::path("config") / binaryName / path;
                    // Bind our binary specific path if it exists
                    if (fs::exists(binaryConfig)) {
                        DSLProxy<::extension::FileWatch>::bind<DSL>(reaction, binaryConfig, flags);
                    }
                }
            }

            template <typename DSL>
            [[nodiscard]] static inline std::shared_ptr<::extension::Configuration> get(threading::Reaction& t) {

                // Get the file watch event
                ::extension::FileWatch watch = DSLProxy<::extension::FileWatch>::get<DSL>(t);

                // Check if the watch is valid
                if (watch && fs::path(watch.path).extension() == ".yaml") {
                    // Return our yaml file
                    try {
                        // Get hostname so we can find the correct per-robot config directory.
                        const std::string hostname = utility::support::getHostname();

                        const auto binaryName = getFirstCommandLineArg();

                        // Get relative path to config file.
                        const auto components = utility::strutil::split(watch.path, '/');
                        fs::path relativePath{};
                        bool flag = false;
                        for (const auto& component : components) {
                            // Ignore the hostname/binary name if they are present.
                            if (flag && (component.compare(hostname) != 0) && (component.compare(binaryName) != 0)) {
                                relativePath = relativePath / component;
                            }

                            // We want out paths relative to the config folder.
                            if (component.compare("config") == 0) {
                                flag = true;
                            }
                        }

                        return std::make_shared<::extension::Configuration>(relativePath, hostname, binaryName);
                    }
                    catch (const YAML::ParserException& e) {
                        throw std::runtime_error(watch.path + " " + std::string(e.what()));
                    }
                }
                else {
                    // Return an empty configuration (which will show up invalid)
                    return std::shared_ptr<::extension::Configuration>(nullptr);
                }
            }
        };
    }  // namespace operation

    // Configuration is transient
    namespace trait {
        template <>
        struct is_transient<std::shared_ptr<::extension::Configuration>> : public std::true_type {};
    }  // namespace trait
}  // namespace NUClear::dsl

#endif  // EXTENSION_CONFIGURATION_HPP

#ifndef MODULE_TOOLS_FIRMWAREINSTALLER_HPP
#define MODULE_TOOLS_FIRMWAREINSTALLER_HPP

#include <nuclear>
#include <string>

namespace module::tools {

    class FirmwareInstaller : public NUClear::Reactor {
    public:
        /// @brief Called by the powerplant to build and setup the FirmwareInstaller
        /// reactor.
        explicit FirmwareInstaller(std::unique_ptr<NUClear::Environment> environment);

    private:
        std::string device;
        struct Firmware {
            std::vector<uint8_t> firmware;
            uint8_t checksum;
        };

        std::map<std::pair<std::string, std::string>, Firmware> firmwares;

        enum MenuState { NO_MENU, DEVICE_MENU, BATTERY_MENU };
        enum Device { NO_DEVICE, CM730, CM740, DYNAMIXEL };
        enum Battery { NO_BATTERY, BATTERY3, BATTERY4 };
        MenuState menu_state;
        Device selected_device;
        Battery selected_battery;

        void showDeviceMenu() const;
        void showBatteryMenu() const;
    };

}  // namespace module::tools

#endif  // MODULE_TOOLS_FIRMWAREINSTALLER_HPP

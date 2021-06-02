#ifndef MODULE_INPUT_FAKECAMERA_HPP
#define MODULE_INPUT_FAKECAMERA_HPP

#include <memory>
#include <mutex>
#include <nuclear>
#include <string>
#include <turbojpeg.h>
#include <vector>

namespace module::input {

    class FakeCamera : public NUClear::Reactor {
    private:
        /// The configuration variables for this reactor
        struct {
            std::string image_folder;
            std::string image_prefix;
            std::string lens_prefix;
        } config;

        std::vector<std::pair<std::string, std::string>> images;
        size_t image_counter = 0;
        std::mutex images_mutex;

        /// @brief JPEG decompressor. Constructed as a shared_ptr so that it will be automatically deleted on class
        /// destruction
        std::shared_ptr<void> decompressor = std::shared_ptr<void>(tjInitDecompress(), [](auto handle) {
            if (handle != nullptr) {
                tjDestroy(handle);
            }
        });

    public:
        /// @brief Called by the powerplant to build and setup the FakeCamera reactor.
        explicit FakeCamera(std::unique_ptr<NUClear::Environment> environment);
    };

}  // namespace module::input

#endif  // MODULE_INPUT_FAKECAMERA_HPP

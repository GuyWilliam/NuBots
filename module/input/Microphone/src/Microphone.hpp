#ifndef MODULE_INPUT_MICROPHONE_HPP
#define MODULE_INPUT_MICROPHONE_HPP

#include <nuclear>

namespace module::input {

    class Microphone : public NUClear::Reactor {
    private:
        /// The configuration variables for this reactor
        struct {
            int sample_rate = 0;
        } config;

        PaStream* stream = nullptr;
        PaError error    = 0;
        paTestData data;


    public:
        /// @brief Called by the powerplant to build and setup the Microphone reactor.
        explicit Microphone(std::unique_ptr<NUClear::Environment> environment);
    };

}  // namespace module::input

#endif  // MODULE_INPUT_MICROPHONE_HPP

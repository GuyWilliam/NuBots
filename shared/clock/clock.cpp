
#include "clock.hpp"

#include <chrono>
#include <mutex>

namespace utility::clock {

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    NUClear::base_clock::time_point last_update = NUClear::base_clock::now();
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    NUClear::base_clock::time_point epoch = last_update;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    double rate = 1.0;  // real time factor

    void update_rtf(const double& rft) {
        auto now = NUClear::base_clock::now();

        // Since we are updating our rtf we need to advance our state so the new deltas will be calculated properly
        // We set the variables in this specific order to mimise the error that will occur if the threading reads as we
        // are writing By reducing the delta between state and now any changes in rtf will have a minimal change on
        // delta
        epoch += delta(now - last_update) * rate;  // set before we update the variables
        last_update = now;
        rate        = rtf;
    }

}  // namespace utility::clock

namespace NUClear {
    clock::time_point clock::now() {
        // Move along our time
        return epoch + delta(NUClear::base_clock::now() - utility::clock::last_update) * utility::clock::custom_rtf;
    }

}  // namespace NUClear

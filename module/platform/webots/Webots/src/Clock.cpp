#include "Clock.hpp"

namespace module::platform::webots {
    Clock::time_point Clock::now() {
        return time_point(current_tick);
    }

    void Clock::tick() {
        current_tick++;
    }
}  // namespace module::platform::webots

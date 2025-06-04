#pragma once

#include "ramen.hpp"
#include <Controllino.h>
#include <cstdint>

struct TimerActor {
    // Configuration
    std::uint32_t interval_ms = 1000U; // 1 second default
    std::uint32_t last_tick = 0U;
    
    // Output event that fires when timer expires
    ramen::Pusher<> tick_event{};
    
    // Constructor to initialize behaviors
    TimerActor() : set_interval([this](const std::uint32_t& new_interval) {
        interval_ms = new_interval;
    }) {}
    
    // Update method to be called in main loop
    void update() {
        std::uint32_t now = millis();
        if (now - last_tick >= interval_ms) {
            last_tick = now;
            if (tick_event) {
                tick_event();  // Fire the tick event
            }
        }
    }
    
    // Behavior to change the interval
    ramen::Pushable<std::uint32_t> set_interval;
};

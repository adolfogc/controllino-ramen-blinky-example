#pragma once

#include "ramen.hpp"
#include <Controllino.h>
#include <cstdint>

struct TimerActor {
    std::uint32_t current_timeout_interval_ms = 0U;
    std::uint32_t timeout_armed_at_ms = 0U;
    bool is_armed = false;

    ramen::Pusher<> actual_timeout_event{};

    ramen::Pushable<std::uint32_t> arm_timeout = [this](const std::uint32_t& requested_interval_ms) {
        current_timeout_interval_ms = requested_interval_ms;
        timeout_armed_at_ms = millis();
        is_armed = true;
    };

    void update() {
        if (is_armed) {
            std::uint32_t now = millis();
            if ((now >= timeout_armed_at_ms && (now - timeout_armed_at_ms >= current_timeout_interval_ms)) ||
                (now < timeout_armed_at_ms && ( (0xFFFFFFFF - timeout_armed_at_ms) + now >= current_timeout_interval_ms) ) ) { // Handle millis() wrap-around
                is_armed = false;
                if (actual_timeout_event) {
                    actual_timeout_event();
                }
            }
        }
    }
};

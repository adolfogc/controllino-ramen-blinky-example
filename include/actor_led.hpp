#pragma once

#include "ramen.hpp"
#include <Controllino.h>
#include <cstdint>

struct LedActor {
    std::uint8_t pin;
    bool state = false;
    
    // Output event when LED state changes
    ramen::Pusher<bool> state_changed{};
    
    explicit LedActor(std::uint8_t led_pin) : pin(led_pin),
        toggle([this]() {
            state = !state;
            digitalWrite(pin, state ? HIGH : LOW);
            if (state_changed) {
                state_changed(state);
            }
        }),
        set_state([this](const bool& new_state) {
            state = new_state;
            digitalWrite(pin, state ? HIGH : LOW);
            if (state_changed) {
                state_changed(state);
            }
        })
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }
    
    // Behavior to toggle the LED
    ramen::Pushable<> toggle;
    
    // Behavior to set LED state directly
    ramen::Pushable<bool> set_state;
};

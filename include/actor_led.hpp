#pragma once

#include "ramen.hpp"
#include <Controllino.h>
#include <cstdint>

struct BlinkyLedActor {
    enum class State {
        LED_OFF,
        LED_ON
    };

    std::uint8_t pin;
    uint32_t blink_interval_ms;
    State current_state;

    // --- RAMEN Ports ---
    ramen::Pusher<> timeout_signal_from_timer{};
    ramen::Pushable<> process_internal_timeout = [this]() {
        handle_timeout_transition();
    };
    ramen::Pusher<std::uint32_t, ramen::Pusher<>*> request_timeout_evt{};

    explicit BlinkyLedActor(std::uint8_t led_pin, std::uint32_t interval_ms) :
        pin(led_pin),
        blink_interval_ms(interval_ms),
        current_state(State::LED_OFF)
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);

        // Internally connect our timeout signal to our processing behavior
        timeout_signal_from_timer >> process_internal_timeout;
    }

    void handle_timeout_transition() {
        switch (current_state) {
            case State::LED_OFF:
                turn_led_on_and_request_next_timeout();
                break;
            case State::LED_ON:
                turn_led_off_and_request_next_timeout();
                break;
        }
    }

    void turn_led_on_and_request_next_timeout() {
        digitalWrite(pin, HIGH);
        current_state = State::LED_ON;
        if (request_timeout_evt) {
            request_timeout_evt(blink_interval_ms, &timeout_signal_from_timer);
        }
    }

    void turn_led_off_and_request_next_timeout() {
        digitalWrite(pin, LOW);
        current_state = State::LED_OFF;
        if (request_timeout_evt) {
            request_timeout_evt(blink_interval_ms, &timeout_signal_from_timer);
        }
    }

    void start() {
        if (request_timeout_evt) {
            request_timeout_evt(blink_interval_ms, &timeout_signal_from_timer);
        }
    }
};

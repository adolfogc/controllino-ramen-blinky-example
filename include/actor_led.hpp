#pragma once
#include "ramen.hpp"
#include "events.hpp"
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
    ramen::Pushable<> process_timeout_event = [this]() {
        handle_timeout();
    };

    ramen::Pusher<std::uint32_t> request_timeout_port{};

    explicit BlinkyLedActor(std::uint8_t led_pin, std::uint32_t interval_ms) :
        pin(led_pin),
        blink_interval_ms(interval_ms),
        current_state(State::LED_OFF)
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    void handle_timeout() {
        switch (current_state) {
            case State::LED_OFF:
                turn_led_on();
                break;
            case State::LED_ON:
                turn_led_off();
                break;
        }
    }

    void turn_led_on() {
        digitalWrite(pin, HIGH);
        current_state = State::LED_ON;
        if (request_timeout_port) {
            request_timeout_port(blink_interval_ms); // arm timer
        }
    }

    void turn_led_off() {
        digitalWrite(pin, LOW);
        current_state = State::LED_OFF;
        if (request_timeout_port) {
            request_timeout_port(blink_interval_ms); // arm timer
        }
    }

    void start() {
        if (request_timeout_port) {
            request_timeout_port(blink_interval_ms); // arm timer
        }
    }
};

#pragma once
#include "event.hpp"
#include "ramen.hpp"
#include <Controllino.h>
#include <cstdint>
#include <array>

// Defines concurrency of timeout events
constexpr size_t MAX_CONCURRENT_TIMEOUTS = 3;

// Error codes for TimerActor operations
enum class TimerError : uint8_t {
    NONE = 0,
    NULL_TARGET_PUSHER,
    NO_FREE_SLOTS,
    TARGET_PUSHER_NOT_FOUND,
    INVALID_INTERVAL
};

struct ActiveTimeout {
    uint32_t target_millis = 0;
    ramen::Pusher<const BaseEvent&>* on_expired_pusher = nullptr; // Client's pusher to invoke
    bool is_active = false;
    bool is_periodic = false;
    uint32_t period_ms = 0;
    void* user_data_for_tick = nullptr; // To pass back in TickEvent if needed

    void clear() {
        target_millis = 0;
        on_expired_pusher = nullptr;
        is_active = false;
        is_periodic = false;
        period_ms = 0;
        user_data_for_tick = nullptr;
    }

    void setup_periodic(uint32_t interval_ms, ramen::Pusher<const BaseEvent&>* pusher, void* udata) {
        target_millis = millis() + interval_ms;
        on_expired_pusher = pusher;
        is_active = true;
        is_periodic = true;
        period_ms = interval_ms;
        // user_data_for_tick = udata;
    }

    void setup_oneshot(uint32_t interval_ms, ramen::Pusher<const BaseEvent&>* pusher, void* udata) {
        target_millis = millis() + interval_ms;
        on_expired_pusher = pusher;
        is_active = true;
        is_periodic = false;
        period_ms = 0;
        user_data_for_tick = udata;
    }

    void restart_periodic() {
        if (is_periodic && period_ms > 0) {
            target_millis = millis() + period_ms;
        }
    }
};

struct TimerActor {
    std::array<ActiveTimeout, MAX_CONCURRENT_TIMEOUTS> active_timeouts;
    TimerError last_error = TimerError::NONE;

    void set_error(TimerError error) { last_error = error; }
    bool has_error() const { return last_error != TimerError::NONE; }
    void clear_error() { last_error = TimerError::NONE; }

    ramen::Pushable<const ArmTimerEvt&> arm_timer_request_in =
        [this](const ArmTimerEvt& evt) {
            clear_error();

            if (!evt.target_pusher) {
                set_error(TimerError::NULL_TARGET_PUSHER);
                return;
            }

            if (evt.interval_ms == 0) {
                set_error(TimerError::INVALID_INTERVAL);
                return;
            }

            // Find a free slot
            for (size_t i = 0; i < MAX_CONCURRENT_TIMEOUTS; ++i) {
                if (!active_timeouts[i].is_active) {
                    if (evt.is_periodic) {
                        active_timeouts[i].setup_periodic(evt.interval_ms, evt.target_pusher, evt.user_data);
                    } else {
                        active_timeouts[i].setup_oneshot(evt.interval_ms, evt.target_pusher, evt.user_data);
                    }
                    return; // Successfully armed
                }
            }
            set_error(TimerError::NO_FREE_SLOTS); // No free slots
        };

    // NEW Pushable to handle DisarmTimerEvt
    ramen::Pushable<const DisarmTimerEvt&> disarm_timer_request_in =
        [this](const DisarmTimerEvt& evt) {
            clear_error();

            if (!evt.target_pusher) {
                set_error(TimerError::NULL_TARGET_PUSHER);
                return;
            }

            bool found = false;
            for (size_t i = 0; i < MAX_CONCURRENT_TIMEOUTS; ++i) {
                if (active_timeouts[i].is_active &&
                    active_timeouts[i].on_expired_pusher == evt.target_pusher) {
                    active_timeouts[i].clear();
                    found = true;
                    // If a client can have multiple timers and this disarms all, continue.
                    // If only one timer per client or disarming specific one, logic might change.
                }
            }

            if (!found) {
                set_error(TimerError::TARGET_PUSHER_NOT_FOUND);
            }
        };

    void update() {
        std::uint32_t now = millis();
        for (size_t i = 0; i < MAX_CONCURRENT_TIMEOUTS; ++i) {
            if (active_timeouts[i].is_active) {
                // Handle wraparound using signed arithmetic
                if ((int32_t)(now - active_timeouts[i].target_millis) >= 0) {
                    // Fire the timer by invoking the client's Pusher
                    if (active_timeouts[i].on_expired_pusher &&
                        *(active_timeouts[i].on_expired_pusher)) {

                        TickEvent tick_to_send;
                        tick_to_send.user_data = active_timeouts[i].user_data_for_tick;

                        // Invoke the client's Pusher. This Pusher will then
                        // trigger any Behaviors (Pushables) linked to it.
                        (*(active_timeouts[i].on_expired_pusher))(tick_to_send);
                    }

                    if (active_timeouts[i].is_periodic) {
                        active_timeouts[i].restart_periodic();
                    } else {
                        active_timeouts[i].clear(); // Clear one-shot timers
                    }
                }
            }
        }
    }

    const char* get_error_string() const {
        switch (last_error) {
            case TimerError::NONE: return "No error";
            case TimerError::NULL_TARGET_PUSHER: return "Null target pusher provided";
            case TimerError::NO_FREE_SLOTS: return "No free timeout slots";
            case TimerError::TARGET_PUSHER_NOT_FOUND: return "Target pusher not found for disarm";
            case TimerError::INVALID_INTERVAL: return "Invalid timeout interval";
            default: return "Unknown error";
        }
    }
};

#pragma once
#include "ramen.hpp"
#include <Controllino.h>
#include <cstdint>
#include <array>

// Defines concurrency of timeouts
// Should allow one slot for each actor that uses timeout events
constexpr size_t MAX_CONCURRENT_TIMEOUTS = 3;

// Error codes for TimerActor operations
enum class TimerError : uint8_t {
    NONE = 0,         // No error
    NULL_PUSHER,      // Attempted to arm timeout with null pusher
    NO_FREE_SLOTS,    // All timeout slots are occupied
    PUSHER_NOT_FOUND, // Pusher not found when trying to disarm
    INVALID_INTERVAL  // Invalid timeout interval (e.g., zero or too large)
};

struct ActiveTimeout {
    uint32_t target_millis = 0;
    ramen::Pusher<>* on_expired_pusher = nullptr;
    bool is_active = false;
    
    // Periodic support
    bool is_periodic = false;
    uint32_t period_ms = 0;
    
    void clear() {
        target_millis = 0;
        on_expired_pusher = nullptr;
        is_active = false;
        is_periodic = false;
        period_ms = 0;
    }
    
    void setup_periodic(uint32_t interval_ms, ramen::Pusher<>* pusher) {
        target_millis = millis() + interval_ms;
        on_expired_pusher = pusher;
        is_active = true;
        is_periodic = true;
        period_ms = interval_ms;
    }
    
    void setup_oneshot(uint32_t interval_ms, ramen::Pusher<>* pusher) {
        target_millis = millis() + interval_ms;
        on_expired_pusher = pusher;
        is_active = true;
        is_periodic = false;
        period_ms = 0;
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
    
    // One-shot timeout (existing interface)
    ramen::Pushable<std::uint32_t, ramen::Pusher<>*> arm_timeout_evt =
        [this](const std::uint32_t& requested_interval_ms, ramen::Pusher<>* requester_pusher_port) {
            setup_timeout(requested_interval_ms, requester_pusher_port, false);
        };
    
    // NEW: Periodic timer interface
    ramen::Pushable<std::uint32_t, ramen::Pusher<>*> arm_periodic_timer_evt =
        [this](const std::uint32_t& period_ms, ramen::Pusher<>* requester_pusher_port) {
            setup_timeout(period_ms, requester_pusher_port, true);
        };
    
    // Disarm all timeouts for a pusher (works for both one-shot and periodic)
    ramen::Pushable<ramen::Pusher<>*> disarm_timeouts_for_pusher =
        [this](ramen::Pusher<>* requester_pusher_port) {
            clear_error();
            
            if (!requester_pusher_port) {
                set_error(TimerError::NULL_PUSHER);
                return;
            }
            
            bool found = false;
            for (size_t i = 0; i < MAX_CONCURRENT_TIMEOUTS; ++i) {
                if (active_timeouts[i].is_active && 
                    active_timeouts[i].on_expired_pusher == requester_pusher_port) {
                    active_timeouts[i].clear();
                    found = true;
                }
            }
            
            if (!found) {
                set_error(TimerError::PUSHER_NOT_FOUND);
            }
        };
    
    void update() {
        std::uint32_t now = millis();
        for (size_t i = 0; i < MAX_CONCURRENT_TIMEOUTS; ++i) {
            if (active_timeouts[i].is_active) {
                // Handle wraparound using signed arithmetic
                if ((int32_t)(now - active_timeouts[i].target_millis) >= 0) {
                    // Fire the timer
                    if (active_timeouts[i].on_expired_pusher && 
                        *(active_timeouts[i].on_expired_pusher)) {
                        (*(active_timeouts[i].on_expired_pusher))();
                    }
                    
                    // Handle periodic vs one-shot
                    if (active_timeouts[i].is_periodic) {
                        active_timeouts[i].restart_periodic();
                    } else {
                        active_timeouts[i].clear();
                    }
                }
            }
        }
    }
    
    const char* get_error_string() const {
        switch (last_error) {
            case TimerError::NONE: return "No error";
            case TimerError::NULL_PUSHER: return "Null pusher provided";
            case TimerError::NO_FREE_SLOTS: return "No free timeout slots";
            case TimerError::PUSHER_NOT_FOUND: return "Pusher not found";
            case TimerError::INVALID_INTERVAL: return "Invalid timeout interval";
            default: return "Unknown error";
        }
    }
    
private:
    void setup_timeout(const std::uint32_t& interval_ms, ramen::Pusher<>* pusher, bool periodic) {
        clear_error();
        
        if (!pusher) {
            set_error(TimerError::NULL_PUSHER);
            return;
        }
        
        if (interval_ms == 0) {
            set_error(TimerError::INVALID_INTERVAL);
            return;
        }
        
        // Find a free slot
        for (size_t i = 0; i < MAX_CONCURRENT_TIMEOUTS; ++i) {
            if (!active_timeouts[i].is_active) {
                if (periodic) {
                    active_timeouts[i].setup_periodic(interval_ms, pusher);
                } else {
                    active_timeouts[i].setup_oneshot(interval_ms, pusher);
                }
                return;
            }
        }
        
        set_error(TimerError::NO_FREE_SLOTS);
    }
};

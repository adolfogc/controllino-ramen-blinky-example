#pragma once
#include "ramen.hpp"
#include <Controllino.h>
#include <cstdint>
#include <array>

// Error codes for TimerActor operations
enum class TimerError : uint8_t {
    NONE = 0,         // No error
    NULL_PUSHER,      // Attempted to arm timeout with null pusher
    NO_FREE_SLOTS,    // All timeout slots are occupied
    PUSHER_NOT_FOUND, // Pusher not found when trying to disarm
    INVALID_INTERVAL  // Invalid timeout interval (e.g., zero or too large)
};

struct ActiveTimeout {
    uint32_t target_millis = 0; // When it should fire
    ramen::Pusher<>* on_expired_pusher = nullptr; // The pusher to trigger when expired
    bool is_active = false; // Is this slot in use?
    
    void clear() {
        target_millis = 0;
        on_expired_pusher = nullptr;
        is_active = false;
    }
};

// Defines concurrency of timeouts
// Should allow one slot for each actor that uses timeout events
constexpr size_t MAX_CONCURRENT_TIMEOUTS = 4;

struct TimerActor {
    std::array<ActiveTimeout, MAX_CONCURRENT_TIMEOUTS> active_timeouts;
    
    // Public error state - can be checked by other actors
    TimerError last_error = TimerError::NONE;
    
    // Helper method to set error and optionally clear previous errors
    void set_error(TimerError error) {
        last_error = error;
    }
    
    // Helper method to check if there's an error
    bool has_error() const {
        return last_error != TimerError::NONE;
    }
    
    // Helper method to clear the error state
    void clear_error() {
        last_error = TimerError::NONE;
    }
    
    // Behavior to arm a timeout for a specific pusher
    // The 'requester_pusher_port' is the ramen::Pusher<> from the actor
    // that wants to be notified.
    ramen::Pushable<std::uint32_t, ramen::Pusher<>*> arm_timeout_evt =
        [this](const std::uint32_t& requested_interval_ms, ramen::Pusher<>* requester_pusher_port) {
            // Clear previous error
            clear_error();
            
            if (!requester_pusher_port) {
                set_error(TimerError::NULL_PUSHER);
                return;
            }
            
            if (requested_interval_ms == 0) {
                set_error(TimerError::INVALID_INTERVAL);
                return;
            }
            
            // Find a free slot
            for (size_t i = 0; i < MAX_CONCURRENT_TIMEOUTS; ++i) {
                if (!active_timeouts[i].is_active) {
                    active_timeouts[i].target_millis = millis() + requested_interval_ms;
                    // Note: Edge case where millis() wraps during calculation 
                    // is handled by the signed arithmetic in update()
                    active_timeouts[i].on_expired_pusher = requester_pusher_port;
                    active_timeouts[i].is_active = true;
                    return; // slot found and used - no error
                }
            }
            
            // No free timeout slots
            set_error(TimerError::NO_FREE_SLOTS);
        };
    
    // Behavior to cancel/disarm all timeouts associated with a specific pusher
    // (Useful if an actor is being "deactivated" or knows it no longer needs pending timeouts)
    ramen::Pushable<ramen::Pusher<>*> disarm_timeouts_for_pusher =
        [this](ramen::Pusher<>* requester_pusher_port) {
            // Clear previous error
            clear_error();
            
            if (!requester_pusher_port) {
                set_error(TimerError::NULL_PUSHER);
                return;
            }
            
            bool found = false;
            for (size_t i = 0; i < MAX_CONCURRENT_TIMEOUTS; ++i) {
                if (active_timeouts[i].is_active && active_timeouts[i].on_expired_pusher == requester_pusher_port) {
                    active_timeouts[i].clear();
                    found = true;
                    // Continue to disarm all matching timeouts (don't return early)
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
                    if (active_timeouts[i].on_expired_pusher && *(active_timeouts[i].on_expired_pusher)) {
                        (*(active_timeouts[i].on_expired_pusher))();
                    }
                    active_timeouts[i].clear();
                }
            }
        }
    }
    
    // Optional: Get error as string for debugging
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
};

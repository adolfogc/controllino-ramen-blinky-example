#pragma once
#include "sml.hpp"
#include "ramen.hpp"
#include <Controllino.h>
#include <cstdint>

namespace led {
    // Events
    struct timeout_event {};
    struct start_event {};

    // States
    struct led_off {};
    struct led_on {};

    // Dynamic LED component for runtime pin assignment
    struct dynamic_led {
        mutable std::uint8_t pin;  // Make pin mutable to allow modification in const contexts
        
        explicit dynamic_led(std::uint8_t p) : pin(p) {}
        
        void setup() const { pinMode(pin, OUTPUT); }
        void on() const    { digitalWrite(pin, HIGH); }
        void off() const   { digitalWrite(pin, LOW); }
    };

    template<class Led>
    struct blinky_led_fsm {
        Led led_component;
        uint32_t blink_interval_ms;
        ramen::Pusher<std::uint32_t, ramen::Pusher<>*>* request_timeout_port;
        ramen::Pusher<>* timeout_signal_port;
        
        blinky_led_fsm(Led led, uint32_t interval,
                    ramen::Pusher<std::uint32_t, ramen::Pusher<>*>* req_port,
                    ramen::Pusher<>* sig_port) 
            : led_component(led), blink_interval_ms(interval), 
            request_timeout_port(req_port), timeout_signal_port(sig_port) 
        {
            led_component.setup();
            led_component.off();
        }

        auto operator()() const {
            using namespace boost::sml;
            
            // Actions with proper SML action signature - no longer need mutable
            auto turn_on_and_request = [this](auto, auto, auto, auto) { 
                led_component.on(); 
                request_next_timeout();
            };
            
            auto turn_off_and_request = [this](auto, auto, auto, auto) { 
                led_component.off(); 
                request_next_timeout();
            };
            
            auto start_timer = [this](auto, auto, auto, auto) {
                request_next_timeout();
            };
            
            // State machine transition table
            return make_transition_table(
                *state<led_off> + event<start_event>   / start_timer           = state<led_off>,
                state<led_off> + event<timeout_event> / turn_on_and_request   = state<led_on>,
                state<led_on>  + event<timeout_event> / turn_off_and_request  = state<led_off>
            );
        }
        
    private:
        // Helper method to request next timeout
        void request_next_timeout() const {
            if (request_timeout_port && *request_timeout_port && timeout_signal_port) {
                (*request_timeout_port)(blink_interval_ms, timeout_signal_port);
            }
        }
    };

    struct BlinkyLedActor {
        std::uint8_t pin;
        uint32_t blink_interval_ms;
        
        // --- RAMEN Ports ---
        ramen::Pusher<> timeout_signal_from_timer{};
        ramen::Pusher<std::uint32_t, ramen::Pusher<>*> request_timeout_evt{};
        
        // State machine with dynamic LED - declare before process_internal_timeout
        using fsm_type = blinky_led_fsm<dynamic_led>;
        boost::sml::sm<fsm_type> sm;
        
        // Initialize this after sm to avoid reorder warning
        ramen::Pushable<> process_internal_timeout;
        
        explicit BlinkyLedActor(std::uint8_t led_pin, std::uint32_t interval_ms) :
            pin(led_pin),
            blink_interval_ms(interval_ms),
            sm(fsm_type{dynamic_led{led_pin}, interval_ms, &request_timeout_evt, &timeout_signal_from_timer}),
            process_internal_timeout([this]() {
                // Process timeout event
                sm.process_event(timeout_event{});
            })
        {
            // Connect RAMEN ports
            timeout_signal_from_timer >> process_internal_timeout;
        }
        
        void start() {
            sm.process_event(start_event{});
        }
    };
}

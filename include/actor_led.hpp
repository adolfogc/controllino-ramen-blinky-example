#pragma once
#include "sml.hpp"
#include "ramen.hpp"
#include <Controllino.h>
#include <cstdint>

namespace led {
// Events
struct periodic_tick {};
struct start_blinking {};
struct stop_blinking {};

// States  
struct stopped {};
struct led_off {};
struct led_on {};

// Dynamic LED component
struct dynamic_led {
    mutable std::uint8_t pin;
    
    explicit dynamic_led(std::uint8_t p) : pin(p) {}
    
    void setup() const { pinMode(pin, OUTPUT); }
    void on() const    { digitalWrite(pin, HIGH); }
    void off() const   { digitalWrite(pin, LOW); }
};

template<class Led>
struct periodic_blinky_fsm {
    Led led_component;
    uint32_t blink_interval_ms;
    ramen::Pusher<std::uint32_t, ramen::Pusher<>*>* periodic_timer_port;
    ramen::Pusher<>* tick_signal_port;
    ramen::Pusher<ramen::Pusher<>*>* disarm_timer_port;
    
    periodic_blinky_fsm(Led led, uint32_t interval,
                        ramen::Pusher<std::uint32_t, ramen::Pusher<>*>* timer_port,
                        ramen::Pusher<>* tick_port,
                        ramen::Pusher<ramen::Pusher<>*>* disarm_port)
        : led_component(led), blink_interval_ms(interval), 
          periodic_timer_port(timer_port), tick_signal_port(tick_port),
          disarm_timer_port(disarm_port)
    {
        led_component.setup();
        led_component.off();  // Start with LED off
    }

    auto operator()() const {
        using namespace boost::sml;
        
        // Actions
        auto start_periodic_timer = [this](auto, auto, auto, auto) {
            request_periodic_timer();
        };
        
        auto stop_periodic_timer = [this](auto, auto, auto, auto) {
            disarm_periodic_timer();
        };
        
        auto turn_led_on = [this](auto, auto, auto, auto) {
            led_component.on();
        };
        
        auto turn_led_off = [this](auto, auto, auto, auto) {
            led_component.off();
        };
        
        auto turn_off_and_stop = [this](auto, auto, auto, auto) {
            led_component.off();
            disarm_periodic_timer();
        };
        
        // State machine transition table
        return make_transition_table(
            // Start blinking: setup periodic timer and enter blink cycle
            *state<stopped> + event<start_blinking> / start_periodic_timer = state<led_off>,
            
            // Single periodic event drives the entire blink cycle
             state<led_off> + event<periodic_tick>  / turn_led_on         = state<led_on>,
             state<led_on>  + event<periodic_tick>  / turn_led_off        = state<led_off>,
            
            // Stop from any blinking state
             state<led_off> + event<stop_blinking>  / stop_periodic_timer = state<stopped>,
             state<led_on>  + event<stop_blinking>  / turn_off_and_stop   = state<stopped>
        );
    }
    
private:
    void request_periodic_timer() const {
        if (periodic_timer_port && tick_signal_port) {
            (*periodic_timer_port)(blink_interval_ms, tick_signal_port);
        }
    }
    
    void disarm_periodic_timer() const {
        if (disarm_timer_port && tick_signal_port) {
            (*disarm_timer_port)(tick_signal_port);
        }
    }
};

struct BlinkyLedActor {
    std::uint8_t pin;
    uint32_t blink_interval_ms;
    
    // --- RAMEN Ports ---
    ramen::Pusher<> periodic_tick_signal{};
    ramen::Pusher<std::uint32_t, ramen::Pusher<>*> request_periodic_timer{};
    ramen::Pusher<ramen::Pusher<>*> disarm_timer{};
    
    // State machine
    using fsm_type = periodic_blinky_fsm<dynamic_led>;
    boost::sml::sm<fsm_type> sm;
    
    ramen::Pushable<> process_periodic_tick;
    
    explicit BlinkyLedActor(std::uint8_t led_pin, std::uint32_t interval_ms) :
        pin(led_pin),
        blink_interval_ms(interval_ms),
        sm(fsm_type{dynamic_led{led_pin}, interval_ms, &request_periodic_timer, 
                   &periodic_tick_signal, &disarm_timer}),
        process_periodic_tick([this]() {
            sm.process_event(periodic_tick{});
        })
    {
        // Connect periodic timer signal to processing
        periodic_tick_signal >> process_periodic_tick;
    }
    
    void start() {
        sm.process_event(start_blinking{});
    }
    
    void stop() {
        sm.process_event(stop_blinking{});
    }
    
    // Optional: Change blink rate dynamically
    void set_blink_interval(uint32_t new_interval_ms) {
        blink_interval_ms = new_interval_ms;
        // Could add a change_interval event to FSM for cleaner handling
    }
};
} // namespace led

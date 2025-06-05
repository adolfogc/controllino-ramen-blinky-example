#pragma once
#include "event.hpp"
#include "sml.hpp"
#include "ramen.hpp"
#include <Controllino.h>
#include <cstdint>

namespace led {

// FSM Events (internal to BlinkyLedActor's state machine)
struct periodic_timeout {};
struct start_blinking {};
struct stop_blinking {};
struct change_interval_request {
    uint32_t new_interval_ms;
};

// States
struct stopped {};
struct led_off {};
struct led_on {};

// Dynamic LED component
struct dynamic_led {
    const std::uint8_t pin;

    explicit dynamic_led(std::uint8_t p) : pin(p) {}

    void setup() { pinMode(pin, OUTPUT); }
    void on()    { digitalWrite(pin, HIGH); }
    void off()   { digitalWrite(pin, LOW); }
};

template<class Led>
struct periodic_blinky_fsm {
    mutable Led led_component;
    uint32_t& blink_interval_ms_ref;

    ramen::Pusher<const ArmTimerEvt&>* arm_timer_request_out_port;
    ramen::Pusher<const BaseEvent&>*   timeout_event_relay_out_port;
    ramen::Pusher<const DisarmTimerEvt&>* disarm_timer_request_out_port;

    periodic_blinky_fsm(Led led, uint32_t& interval_ref,
                        ramen::Pusher<const ArmTimerEvt&>* arm_port,
                        ramen::Pusher<const BaseEvent&>* tick_relay_port,
                        ramen::Pusher<const DisarmTimerEvt&>* disarm_port)
        : led_component(led), blink_interval_ms_ref(interval_ref),
          arm_timer_request_out_port(arm_port),
          timeout_event_relay_out_port(tick_relay_port),
          disarm_timer_request_out_port(disarm_port)
    {
        led_component.setup();
        led_component.off();
    }

    auto operator()() const {
        using namespace boost::sml;

        auto start_periodic_timer_action = [this] {
            request_periodic_timer();
        };
        auto stop_periodic_timer_action = [this] {
            disarm_periodic_timer();
        };
        auto turn_led_on_action = [this] {
            led_component.on();
        };
        auto turn_led_off_action = [this] {
            led_component.off();
        };
        auto turn_off_and_stop_action = [this] {
            led_component.off();
            disarm_periodic_timer();
        };
        auto update_interval_and_rearm_action = [this](const change_interval_request& evt) {
            blink_interval_ms_ref = evt.new_interval_ms;
            disarm_periodic_timer();
            if (blink_interval_ms_ref > 0) {
                request_periodic_timer();
            }
        };
        auto update_interval_when_stopped_action = [this](const change_interval_request& evt) {
            blink_interval_ms_ref = evt.new_interval_ms;
        };

        return make_transition_table(
            *state<stopped> + event<start_blinking>           / start_periodic_timer_action     = state<led_off>,
             state<stopped> + event<change_interval_request>  / update_interval_when_stopped_action = state<stopped>,

             state<led_off> + event<periodic_timeout>            / turn_led_on_action              = state<led_on>,
             state<led_on>  + event<periodic_timeout>            / turn_led_off_action             = state<led_off>,

             state<led_off> + event<stop_blinking>            / stop_periodic_timer_action      = state<stopped>,
             state<led_on>  + event<stop_blinking>            / turn_off_and_stop_action        = state<stopped>,

             state<led_off> + event<change_interval_request>  / update_interval_and_rearm_action = state<led_off>,
             state<led_on>  + event<change_interval_request>  / update_interval_and_rearm_action = state<led_on>
        );
    }

private:
    void request_periodic_timer() const {
        if (arm_timer_request_out_port && timeout_event_relay_out_port) {
            if (blink_interval_ms_ref > 0) {
                ArmTimerEvt evt(blink_interval_ms_ref, timeout_event_relay_out_port, true);
                (*arm_timer_request_out_port)(evt);
            }
        }
    }

    void disarm_periodic_timer() const {
        if (disarm_timer_request_out_port && timeout_event_relay_out_port) {
            DisarmTimerEvt evt(timeout_event_relay_out_port);
            (*disarm_timer_request_out_port)(evt);
        }
    }
};

struct BlinkyLedActor {
    std::uint8_t pin;
    uint32_t blink_interval_ms;

    ramen::Pusher<const ArmTimerEvt&> arm_timer_request_out;
    ramen::Pusher<const DisarmTimerEvt&> disarm_timer_request_out;
    ramen::Pusher<const BaseEvent&> timeout_event_relay_out;
    ramen::Pushable<const BaseEvent&> event_handler_in;

    using fsm_type = periodic_blinky_fsm<dynamic_led>;
    boost::sml::sm<fsm_type> sm;

    explicit BlinkyLedActor(std::uint8_t led_pin, uint32_t interval_ms_initial) :
        pin(led_pin),
        blink_interval_ms(interval_ms_initial),
        event_handler_in([this](const BaseEvent& event) {
            switch (event.type) {
                case AppEventType::EV_TIMEOUT:
                    // If TickEvent had specific members you needed, you would static_cast here:
                    // const TickEvent& tick_evt = static_cast<const TickEvent&>(event);
                    this->sm.process_event(periodic_timeout{});
                    break;
                default:
                    // Optionally, handle or log unknown/unexpected event types
                    break;
            }
        }),
        sm(fsm_type{dynamic_led{led_pin}, this->blink_interval_ms,
                   &this->arm_timer_request_out,
                   &this->timeout_event_relay_out,
                   &this->disarm_timer_request_out
                   })
    {
        timeout_event_relay_out >> event_handler_in;
    }

    void start() {
        sm.process_event(start_blinking{});
    }

    void stop() {
        sm.process_event(stop_blinking{});
    }

    void set_blink_interval(uint32_t new_interval_ms) {
        sm.process_event(change_interval_request{new_interval_ms});
    }
};
} // namespace led

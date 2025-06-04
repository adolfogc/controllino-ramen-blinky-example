#include "actor_timer.hpp"
#include "actor_led.hpp"
#include "sml.hpp"
#include <Controllino.h>

// Global actor instances
TimerActor timer;
led::BlinkyLedActor led1(CONTROLLINO_D0, 500); // 500ms interval
led::BlinkyLedActor led2(CONTROLLINO_D1, 5000); // 5s interval
led::BlinkyLedActor led3(CONTROLLINO_D2, 500);

void setup() {    
    // Connect periodic timer requests to TimerActor
    led1.request_periodic_timer >> timer.arm_periodic_timer_evt;
    led2.request_periodic_timer >> timer.arm_periodic_timer_evt;
    led3.request_periodic_timer >> timer.arm_periodic_timer_evt;
    
    // Connect disarm requests (for stop functionality)
    led1.disarm_timer >> timer.disarm_timeouts_for_pusher;
    led2.disarm_timer >> timer.disarm_timeouts_for_pusher;
    led3.disarm_timer >> timer.disarm_timeouts_for_pusher;

    led1.start();
    led2.start();
    led3.start();
}

void loop() {
    // Update actors that need periodic processing
    timer.update();
}

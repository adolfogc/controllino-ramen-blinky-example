#include "actor_timer.hpp"
#include "actor_led.hpp"
#include "sml.hpp"
#include <Controllino.h>

// Global actor instances
TimerActor timer;
BlinkyLedActor led1(CONTROLLINO_D0, 500); // 500ms interval
BlinkyLedActor led2(CONTROLLINO_D1, 5000); // 5s interval

void setup() {    
    led1.request_timeout_evt >> timer.arm_timeout_evt;
    led2.request_timeout_evt >> timer.arm_timeout_evt;

    led1.start();
    led2.start();
}

void loop() {
    // Update actors that need periodic processing
    timer.update();
}

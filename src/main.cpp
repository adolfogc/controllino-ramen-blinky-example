#include "actor_timer.hpp"
#include "actor_led.hpp"
#include <Controllino.h>

TimerActor timer;
led::BlinkyLedActor led1(CONTROLLINO_D0, 500);  // 500ms interval
led::BlinkyLedActor led2(CONTROLLINO_D1, 1000); // 1s interval
led::BlinkyLedActor led3(CONTROLLINO_D2, 500);

void setup() {
    // Connect ArmTimerEvt requests:
    led1.arm_timer_request_out >> timer.arm_timer_request_in;
    led2.arm_timer_request_out >> timer.arm_timer_request_in;
    led3.arm_timer_request_out >> timer.arm_timer_request_in;

    // Connect DisarmTimerEvt requests:
    led1.disarm_timer_request_out >> timer.disarm_timer_request_in;
    led2.disarm_timer_request_out >> timer.disarm_timer_request_in;
    led3.disarm_timer_request_out >> timer.disarm_timer_request_in;

    led1.start();
    led2.start();
    led3.start();
}

void loop() {
          
    // Poll actors needing periodic checks (like TimerActor).
    // Event-driven actors (e.g., BlinkyLed) are updated by their event sources.
    timer.update();
}

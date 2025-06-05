#include "actor_timer.hpp"
#include "actor_led.hpp"
#include "actor_serial_commander.hpp"
#include <Controllino.h>

TimerActor timer;
led::BlinkyLedActor led1(CONTROLLINO_D0, 500);  // 500ms interval
led::BlinkyLedActor led2(CONTROLLINO_D1, 1000); // 1s interval
led::BlinkyLedActor led3(CONTROLLINO_D2, 500);

serial_cmd::SerialCommandSystem commander(led1, led2, led3);

void setup() {
    // Initialize serial commander
    commander.init();
    
    // Connect ArmTimerEvt requests:
    led1.arm_timer_request_out >> timer.arm_timer_request_in;
    led2.arm_timer_request_out >> timer.arm_timer_request_in;
    led3.arm_timer_request_out >> timer.arm_timer_request_in;

    // Connect DisarmTimerEvt requests:
    led1.disarm_timer_request_out >> timer.disarm_timer_request_in;
    led2.disarm_timer_request_out >> timer.disarm_timer_request_in;
    led3.disarm_timer_request_out >> timer.disarm_timer_request_in;

    // Start all LEDs initially (optional - can be controlled via serial)
    led1.start();
    led2.start();
    led3.start();
}

void loop() {
    // Process serial commands
    commander.update();
    
    // Poll actors needing periodic checks (like TimerActor).
    // Event-driven actors (e.g., BlinkyLed) are updated by their event sources.
    timer.update();
}

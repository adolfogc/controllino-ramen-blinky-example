#include "actor_timer.hpp"
#include "actor_led.hpp"
#include "sml.hpp"
#include <Controllino.h>

// Global actor instances
TimerActor timer;
BlinkyLedActor led(CONTROLLINO_D0, 500); // Pin D0, 500ms interval

void setup() {    
    led.request_timeout_port >> timer.arm_timeout; // for receiving timeout event
    timer.actual_timeout_event >> led.process_timeout_event; // for arming the timer

    led.start();
}

void loop() {
    // Update actors that need periodic processing
    timer.update();
}

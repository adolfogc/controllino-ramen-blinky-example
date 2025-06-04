#include "actors.hpp"
#include <Controllino.h>

// Global actor instances
TimerActor timer;
LedActor led(CONTROLLINO_D0);

void setup() {
    timer.tick_event >> led.toggle;

    timer.set_interval(500);  // 500ms blink interval
}

void loop() {
        // Update actors that need periodic processing
        timer.update();
}

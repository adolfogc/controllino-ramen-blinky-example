# Controllino Maxi Automation RAMEN Blinky Example

This repository presents an exploratory setup for modern embedded software development on the Controllino Maxi Automation platform (potentially adaptable to other AVR-based boards) using PlatformIO. The PoC focuses on an actor-based programming paradigm, utilizing the ramen library alongside avr-libstdcpp for C++ standard library support and Boost SML for state machine implementation. This approach was pursued as an alternative following the discontinuation of official QP-nano support for AVR-based Arduino platforms. Given that the upstream ramen library requires C++20, a C++17 port was necessitated by AVR toolchain limitations; this initial translation was assisted by AI language models (Claude 4.0 Sonnet and Gemini Pro 2.5). A simple Blinky application was then developed to test the ported library. This preliminary work indicates that these libraries could provide a valuable framework for developing software for Controllino Maxi Automation PLCs using contemporary C++ facilities, although rigorous vetting of the translated ramen codebase remains a critical next step.

The PoC aims for actor-based concurrency with functional reactive programming patterns via RAMEN's port-based communication system.

Initial testing (example below) demonstrates efficient resource utilization (22.4% RAM, 6.4% Flash) while providing type-safe, declarative state machine programming on resource-constrained hardware using modern C++.

Example:

```cpp
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
```

To test:

```bash
brew install platformio

pio init

pio run

pio run -t upload
```

## Links

- https://platformio.org
- https://github.com/modm-io/avr-libstdcpp
- https://github.com/Zubax/ramen
- https://boost-ext.github.io/sml/examples.html#arduino-integration

# Controllino Maxi Automation RAMEN Blinky Example

This repository presents an exploratory setup for modern embedded software development on the Controllino Maxi Automation platform (potentially adaptable to other AVR-based boards) using PlatformIO. The PoC focuses on an actor-based programming paradigm, utilizing the ramen library alongside avr-libstdcpp and Boost SML. This approach was pursued as an alternative following the discontinuation of official QP-nano support for AVR-based Arduino platforms. Given that the upstream ramen library requires C++20, a C++17 port was necessitated by AVR toolchain limitations; this initial translation was assisted by AI language models (Claude 4.0 Sonnet and Gemini Pro 2.5). A simple Blinky application was then developed to test the ported library. This preliminary work indicates that these libraries could provide a valuable framework for developing software for Controllino Maxi Automation PLCs using contemporary C++ facilities, although rigorous vetting of the translated ramen codebase remains a critical next step.

Example:

```cpp
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
    // Connect periodic timer requests to enhanced TimerActor
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

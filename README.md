# Controllino Maxi Automation RAMEN Blinky Example

This repository presents an exploratory setup for modern embedded software development on the Controllino Maxi Automation platform (potentially adaptable to other AVR-based boards) using PlatformIO. The PoC focuses on an actor-based programming paradigm, utilizing the ramen library alongside avr-libstdcpp. This approach was pursued as an alternative following the discontinuation of official QP-nano support for AVR-based Arduino platforms. Given that the upstream ramen library requires C++20, a C++17 port was necessitated by AVR toolchain limitations; this initial translation was assisted by AI language models (Claude 4.0 Sonnet and Gemini Pro 2.5). A simple Blinky application was then developed to test the ported library. This preliminary work indicates that these libraries could provide a valuable framework for developing software for Controllino Maxi Automation PLCs using contemporary C++ facilities, although rigorous vetting of the translated ramen codebase remains a critical next step.

To test:

```bash
brew install platformio

pio init

pio run

pio run -t upload
```

## Links

- https://platformio.org/
- https://github.com/modm-io/avr-libstdcpp
- https://github.com/Zubax/ramen

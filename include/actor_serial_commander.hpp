#pragma once
#include "actor_led.hpp"
#include "ramen.hpp"
#include <Controllino.h>
#include <cstdint>
#include <cstring>
#include <cctype>
#include <cstdio>

namespace serial_cmd {

struct SerialCharEvent {
    std::uint8_t character;
};

struct CommandLineEvent {
    std::uint8_t command_line[64];
    std::size_t length;
};

struct LedCommandEvent {
    std::uint8_t led_id;      // 0=led1, 1=led2, 2=led3
    enum Type { START, STOP, SET_INTERVAL } type;
    std::uint32_t interval_ms = 0;  // Only used for SET_INTERVAL
};

struct StatusRequestEvent {};
struct HelpRequestEvent {};

class SerialCollectorActor {
private:
    static constexpr std::size_t MAX_CMD_LENGTH = 64;
    std::uint8_t buffer[MAX_CMD_LENGTH];
    std::size_t pos = 0;

public:
    // Input: individual characters from serial
    ramen::Pushable<SerialCharEvent> char_in = 
        [this](const SerialCharEvent& evt) {
            std::uint8_t c = evt.character;
            
            if (c == '\n' || c == '\r') {
                if (pos > 0) {
                    buffer[pos] = '\0';
                    CommandLineEvent cmd_evt;
                    // Copy as bytes, but treat as null-terminated string
                    std::memcpy(cmd_evt.command_line, buffer, pos + 1);
                    cmd_evt.length = pos;
                    
                    line_out(cmd_evt);
                    pos = 0;
                }
            } else if (pos < MAX_CMD_LENGTH - 1) {
                buffer[pos++] = c;
            } else {
                // Buffer overflow, reset
                pos = 0;
            }
        };
    
    // Output: complete command lines
    ramen::Pusher<CommandLineEvent> line_out;
};

class CommandParserActor {
private:
    bool parse_led_id(const std::uint8_t* str, std::uint8_t& led_id) {
        while (*str == ' ' || *str == '\t') str++;
        if (*str >= '1' && *str <= '3') {
            led_id = *str - '1';
            return true;
        }
        return false;
    }
    
    bool parse_interval_command(const std::uint8_t* str, std::uint8_t& led_id, std::uint32_t& interval) {
        // Skip whitespace
        while (*str == ' ' || *str == '\t') str++;
        
        // Parse LED ID
        if (*str < '1' || *str > '3') {
            Serial.println("Debug: Invalid LED ID");
            return false;
        }
        led_id = *str - '1';
        str++;
        
        // Skip whitespace
        while (*str == ' ' || *str == '\t') str++;
        
        // Parse interval - convert to char* for strtol
        char* endptr;
        long parsed_interval = std::strtol(reinterpret_cast<const char*>(str), &endptr, 10);
        
        // Check if parsing succeeded and value is in valid range
        if (endptr == reinterpret_cast<const char*>(str)) {
            return false;
        }
        
        if (parsed_interval < 1 || parsed_interval > 60000) {  // 1ms to 60s range
            return false;
        }
        
        interval = static_cast<std::uint32_t>(parsed_interval);
        return true;
    }

public:
    // Input: command lines to parse
    ramen::Pushable<CommandLineEvent> line_in = 
        [this](const CommandLineEvent& evt) {
            std::uint8_t lower_cmd[64];
            std::memcpy(lower_cmd, evt.command_line, sizeof(lower_cmd));
            
            // Convert to lowercase
            for (std::uint8_t* p = lower_cmd; *p; ++p) {
                *p = static_cast<std::uint8_t>(std::tolower(*p));
            }
            
            // Parse and route commands
            if (std::strncmp(reinterpret_cast<const char*>(lower_cmd), "help", 4) == 0) {
                help_request_out(HelpRequestEvent{});
            }
            else if (std::strncmp(reinterpret_cast<const char*>(lower_cmd), "status", 6) == 0) {
                status_request_out(StatusRequestEvent{});
            }
            else if (std::strncmp(reinterpret_cast<const char*>(lower_cmd), "start", 5) == 0) {
                std::uint8_t led_id;
                if (parse_led_id(lower_cmd + 5, led_id)) {
                    LedCommandEvent cmd{led_id, LedCommandEvent::START, 0};
                    led_command_out(cmd);
                } else {
                    error_out("Invalid LED ID for start command");
                }
            }
            else if (std::strncmp(reinterpret_cast<const char*>(lower_cmd), "stop", 4) == 0) {
                std::uint8_t led_id;
                if (parse_led_id(lower_cmd + 4, led_id)) {
                    LedCommandEvent cmd{led_id, LedCommandEvent::STOP, 0};
                    led_command_out(cmd);
                } else {
                    error_out("Invalid LED ID for stop command");
                }
            }
            else if (std::strncmp(reinterpret_cast<const char*>(lower_cmd), "interval", 8) == 0) {
                std::uint8_t led_id;
                std::uint32_t interval;
                
                if (parse_interval_command(lower_cmd + 8, led_id, interval)) {
                    LedCommandEvent cmd{led_id, LedCommandEvent::SET_INTERVAL, interval};
                    led_command_out(cmd);
                } else {
                    error_out("Invalid format for interval command");
                }
            }
            else {
                error_out("Unknown command. Type 'help' for usage.");
            }
        };
    
    // Outputs: parsed commands
    ramen::Pusher<LedCommandEvent> led_command_out;
    ramen::Pusher<HelpRequestEvent> help_request_out;
    ramen::Pusher<StatusRequestEvent> status_request_out;
    ramen::Pusher<const char*> error_out;
};

class LedExecutorActor {
private:
    led::BlinkyLedActor* leds[3];
    const char* led_names[3] = {"LED1", "LED2", "LED3"};

public:
    LedExecutorActor(led::BlinkyLedActor& led1, 
                    led::BlinkyLedActor& led2, 
                    led::BlinkyLedActor& led3) {
        leds[0] = &led1;
        leds[1] = &led2;
        leds[2] = &led3;
    }
    
    // Input: LED commands to execute
    ramen::Pushable<LedCommandEvent> command_in = 
        [this](const LedCommandEvent& evt) {
            if (evt.led_id >= 3) {
                response_out("Invalid LED ID");
                return;
            }
            
            switch (evt.type) {
                case LedCommandEvent::START:
                    leds[evt.led_id]->start();
                    {
                        std::uint8_t msg[32];
                        std::snprintf(reinterpret_cast<char*>(msg), sizeof(msg), 
                                     "%s started", led_names[evt.led_id]);
                        response_out(reinterpret_cast<const char*>(msg));
                    }
                    break;
                    
                case LedCommandEvent::STOP:
                    leds[evt.led_id]->stop();
                    {
                        std::uint8_t msg[32];
                        std::snprintf(reinterpret_cast<char*>(msg), sizeof(msg), 
                                     "%s stopped", led_names[evt.led_id]);
                        response_out(reinterpret_cast<const char*>(msg));
                    }
                    break;
                    
                case LedCommandEvent::SET_INTERVAL:
                    leds[evt.led_id]->set_blink_interval(evt.interval_ms);
                    {
                        std::uint8_t msg[64];
                        std::snprintf(reinterpret_cast<char*>(msg), sizeof(msg), 
                                     "%s interval set to %lums", 
                                     led_names[evt.led_id], 
                                     static_cast<unsigned long>(evt.interval_ms));
                        response_out(reinterpret_cast<const char*>(msg));
                    }
                    break;
            }
        };
    
    // Output: response messages
    ramen::Pusher<const char*> response_out;
};

class StatusReporterActor {
private:
    led::BlinkyLedActor* leds[3];
    const char* led_names[3] = {"LED1", "LED2", "LED3"};

public:
    StatusReporterActor(led::BlinkyLedActor& led1, 
                       led::BlinkyLedActor& led2, 
                       led::BlinkyLedActor& led3) {
        leds[0] = &led1;
        leds[1] = &led2;
        leds[2] = &led3;
    }
    
    ramen::Pushable<StatusRequestEvent> request_in = 
        [this](const StatusRequestEvent&) {
            response_out("LED Status:");
            for (std::size_t i = 0; i < 3; i++) {
                std::uint8_t msg[64];
                std::snprintf(reinterpret_cast<char*>(msg), sizeof(msg), 
                             "  %s: Pin D%d, Interval: %lums", 
                             led_names[i], 
                             static_cast<int>(leds[i]->pin), 
                             static_cast<unsigned long>(leds[i]->blink_interval_ms));
                response_out(reinterpret_cast<const char*>(msg));
            }
        };
    
    ramen::Pusher<const char*> response_out;
};

class HelpProviderActor {
public:
    ramen::Pushable<HelpRequestEvent> request_in = 
        [this](const HelpRequestEvent&) {
            static const char* help_lines[] = {
                "Available commands:",
                "  start <1-3>        - Start LED (1=LED1, 2=LED2, 3=LED3)",
                "  stop <1-3>         - Stop LED",
                "  interval <1-3> <ms> - Set blink interval in milliseconds",
                "  status             - Show current status",
                "  help               - Show this help",
                "",
                "Examples:",
                "  start 1            - Start LED1",
                "  stop 2             - Stop LED2",
                "  interval 1 200     - Set LED1 to 200ms blink interval",
                nullptr
            };
            
            for (const char** line = help_lines; *line != nullptr; ++line) {
                response_out(*line);
            }
        };
    
    ramen::Pusher<const char*> response_out;
};

class SerialOutputActor {
public:
    ramen::Pushable<const char*> message_in = 
        [this](const char* const& msg) {
            Serial.println(msg);
        };
};

// Main serial commander system - composes all actors
class SerialCommandSystem {
private:
    SerialCollectorActor collector;
    CommandParserActor parser;
    LedExecutorActor executor;
    StatusReporterActor status_reporter;
    HelpProviderActor help_provider;
    SerialOutputActor output;

public:
    SerialCommandSystem(led::BlinkyLedActor& led1, 
                       led::BlinkyLedActor& led2, 
                       led::BlinkyLedActor& led3) 
        : executor(led1, led2, led3)
        , status_reporter(led1, led2, led3) {
        
        // Wire up the data flow using push syntax
        collector.line_out >> parser.line_in;
        
        parser.led_command_out >> executor.command_in;
        parser.help_request_out >> help_provider.request_in;
        parser.status_request_out >> status_reporter.request_in;
        
        // All text outputs go to serial
        executor.response_out >> output.message_in;
        status_reporter.response_out >> output.message_in;
        help_provider.response_out >> output.message_in;
        parser.error_out >> output.message_in;
    }
    
    void init() {
        Serial.begin(9600);
        Serial.println(F("LED Controller Ready"));
        HelpRequestEvent help{};
        help_provider.request_in(help);
    }
    
    void update() {
        // Feed characters into the system
        while (Serial.available()) {
            int ch = Serial.read();
            if (ch >= 0) {  // Valid character received
                SerialCharEvent evt{static_cast<std::uint8_t>(ch)};
                collector.char_in(evt);
            }
        }
    }
};

} // namespace serial_cmd

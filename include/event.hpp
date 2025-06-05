#pragma once

#include "ramen.hpp"
#include <cstdint>

// Enum for event types, unique across your application
enum class AppEventType : uint8_t {
    EV_UNKNOWN = 0,
    EV_TIMEOUT,
    EV_ARM_TIMER,
    EV_DISARM_TIMER
};

struct BaseEvent {
    AppEventType type;
    void* user_data;

    explicit BaseEvent(AppEventType t) : type(t) {}
    virtual ~BaseEvent() = default;
};

struct TickEvent : public BaseEvent {
    TickEvent() : BaseEvent(AppEventType::EV_TIMEOUT) {}
};

struct ArmTimerEvt : public BaseEvent {
    std::uint32_t interval_ms;
    ramen::Pusher<const BaseEvent&>* target_pusher;
    bool is_periodic;

    ArmTimerEvt(std::uint32_t interval, ramen::Pusher<const BaseEvent&>* pusher, bool periodic)
        : BaseEvent(AppEventType::EV_ARM_TIMER), interval_ms(interval), target_pusher(pusher), is_periodic(periodic) {}
};

struct DisarmTimerEvt : public BaseEvent {
    ramen::Pusher<const BaseEvent&>* target_pusher;

    explicit DisarmTimerEvt(ramen::Pusher<const BaseEvent&>* pusher)
        : BaseEvent(AppEventType::EV_DISARM_TIMER), target_pusher(pusher) {}
};

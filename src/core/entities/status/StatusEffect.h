#pragma once

enum class StatusType
{
    NONE,

    FREEZE,
    SLOW,
    ATTACK_DOWN
};

struct StatusEffect
{
    StatusType type = StatusType::NONE;

    float durationRemaining = 0.f;

    float value = 0.f;
};
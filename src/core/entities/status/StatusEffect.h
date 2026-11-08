#pragma once

#include <string>

enum class StatusType
{
    NONE,

    PARALYZED,
    SLOW,
    ATTACK_DOWN,

    COUNT // Not a status: keep it last (used to check statuses.json defines every status)
};

// Name used in statuses.json and the debug tools
inline const char* to_string(StatusType type)
{
    switch (type)
    {
    case StatusType::PARALYZED:   return "PARALYZED";
    case StatusType::SLOW:        return "SLOW";
    case StatusType::ATTACK_DOWN: return "ATTACK_DOWN";
    default:                      return "NONE";
    }
}

/*
 * StatusEffect
 * ------------
 * One status currently active on an entity.
 * Only the StatusManager adds, changes or removes them.
 */
struct StatusEffect
{
    StatusType type = StatusType::NONE;

    float durationRemaining = 0.f;

    float value = 0.f;          // Strength (e.g. 0.5 = attack x0.5 for ATTACK_DOWN)
};

/*
 * StatusData
 * ----------
 * Definition of a status, loaded from game_data/statuses.json.
 */
enum class StrongestRule
{
    LOWEST,   // A lower value is stronger (e.g. ATTACK_DOWN 0.3 beats 0.5)
    HIGHEST   // A higher value is stronger
};

struct StatusData
{
    StatusType type = StatusType::NONE;
    std::string name;            // Shown to the player
    std::string description;     // For the future "more information" panel
    StrongestRule strongest = StrongestRule::LOWEST; // Which value wins when the status is applied again
    int iconIndex = -1;          // Index in the status icons sprite sheet
};

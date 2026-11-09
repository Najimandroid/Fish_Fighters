#pragma once

#include <string>

enum class StatusType
{
    NONE,

    PARALYZED,
    SLOW,
    WEAKEN,

    COUNT // Not a status: keep it last (used to check statuses.json defines every status)
};

// Name used in statuses.json and the debug tools
inline const char* to_string(StatusType type)
{
    switch (type)
    {
    case StatusType::PARALYZED:   return "PARALYZED";
    case StatusType::SLOW:        return "SLOW";
    case StatusType::WEAKEN: return "WEAKEN";
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

    float value = 0.f;          // Strength (e.g. 0.5 = attack x0.5 for WEAKEN)
};

/*
 * StatusData
 * ----------
 * Definition of a status, loaded from game_data/statuses.json.
 */
enum class StrongestRule
{
    LOWEST,   // A lower value is stronger (e.g. WEAKEN 0.3 beats 0.5)
    HIGHEST   // A higher value is stronger
};

struct StatusData
{
    StatusType type = StatusType::NONE;
    std::string name;            // Shown to the player
    std::string description;     // For the future "more information" panel
    StrongestRule strongest = StrongestRule::LOWEST; // Which value wins when the status is applied again
    int iconIndex = -1;          // Index in the status icons sprite sheet (STATUS_ICONS_TEXTURE), -1 = no icon
};

/*
 * Status icons sprite sheet
 * -------------------------
 * One square icon per cell, read left to right then top to bottom:
 * with 2 columns, iconIndex 0 is top-left, 1 top-right, 2 the start of the second row.
 * Used by UIStatusIcons in battle and by FishEditor.
 */
inline constexpr const char* STATUS_ICONS_TEXTURE = "assets/images/textures/icons/statuses.png";
inline constexpr int STATUS_ICON_CELL_SIZE = 100;   // Pixels of one icon in the sheet

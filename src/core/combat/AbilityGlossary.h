#pragma once

#include "../../data/AbilityData.h"
#include "DamageCalculator.h"

#include <string>

/*
 * AbilityGlossary
 * ---------------
 * How types and abilities are shown to the player: names, the cell of their icon in the icon sheets,
 * and a sentence explaining what an ability does (with its real numbers).
 * Shared by the game's menus (upgrade, encyclopedias) and FishEditor, so both say the same thing.
 *
 * Icon sheets (100 px cells, read left to right, top to bottom):
 *   types.png:     crustacean, shelled / jelly, blank
 *   abilities.png: strong against, resistant, attack down / paralyze, slow, poison
 */

namespace AbilityGlossary
{
    inline constexpr const char* TYPE_ICONS_TEXTURE = "assets/images/textures/icons/types.png";
    inline constexpr const char* ABILITY_ICONS_TEXTURE = "assets/images/textures/icons/abilities.png";
    inline constexpr int ICON_CELL_SIZE = 100;

    // Player-facing name of a type ("Shelled")
    inline const char* type_name(EntityType type)
    {
        switch (type)
        {
        case EntityType::SHELLED:    return "Shelled";
        case EntityType::CRUSTACEAN: return "Crustacean";
        case EntityType::JELLY:      return "Jelly";
        case EntityType::FISH:       return "Fish";
        default:                     return "None";
        }
    }

    // Cell of the type in types.png (the blank cell for types without an icon)
    inline int type_icon_index(EntityType type)
    {
        switch (type)
        {
        case EntityType::CRUSTACEAN: return 0;
        case EntityType::SHELLED:    return 1;
        case EntityType::JELLY:      return 2;
        default:                     return 3;
        }
    }

    // Cell of the ability in abilities.png, -1 without an icon
    inline int ability_icon_index(AbilityType type)
    {
        switch (type)
        {
        case AbilityType::STRONG_AGAINST:    return 0;
        case AbilityType::RESISTANT_AGAINST: return 1;
        case AbilityType::WEAKEN_ATTACK:     return 2;
        case AbilityType::PARALYZE_ATTACK:   return 3;
        case AbilityType::SLOW_ATTACK:       return 4;
        default:                             return -1;
        }
    }

    // Short label shown before the target types ("Strong against")
    inline const char* ability_label(AbilityType type)
    {
        switch (type)
        {
        case AbilityType::STRONG_AGAINST:    return "Strong against";
        case AbilityType::RESISTANT_AGAINST: return "Resistant to";
        case AbilityType::WEAKEN_ATTACK:     return "Weakens";
        case AbilityType::PARALYZE_ATTACK:   return "Paralyzes";
        case AbilityType::SLOW_ATTACK:       return "Slows";
        default:                             return "";
        }
    }

    namespace detail
    {
        inline std::string number(float value)
        {
            std::string text = std::to_string(value);
            text.erase(text.find_last_not_of('0') + 1);
            if (!text.empty() && text.back() == '.') text.pop_back();
            return text;
        }
    }

    // Who an ability targets: "Crustacean and Shelled enemies", or "fish" for the abilities of enemies
    inline std::string targets_text(const std::vector<EntityType>& targetTypes)
    {
        std::string names;
        bool targetsFish = false;
        for (size_t i = 0; i < targetTypes.size(); ++i)
        {
            if (targetTypes[i] == EntityType::FISH) { targetsFish = true; continue; }
            if (!names.empty()) names += (i + 1 == targetTypes.size()) ? " and " : ", ";
            names += type_name(targetTypes[i]);
        }
        if (names.empty()) return targetsFish ? "fish" : "nothing";
        return names + " enemies";
    }

    // What the ability does, with its numbers ("Deals x2 damage to Jelly enemies.")
    inline std::string ability_explanation(const AbilityData& ability)
    {
        using detail::number;
        const std::string targets = targets_text(ability.targetTypes);
        const std::string chance = number(ability.chance) + " % chance";
        const std::string duration = number(ability.duration) + " s";

        switch (ability.type)
        {
        case AbilityType::STRONG_AGAINST:
            return "Deals x" + number(DamageCalculator::STRONG_AGAINST_MULTIPLIER) + " damage to " + targets + ".";
        case AbilityType::RESISTANT_AGAINST:
            return "Takes x" + number(DamageCalculator::RESISTANT_MULTIPLIER) + " damage from " + targets + ".";
        case AbilityType::WEAKEN_ATTACK:
            return "Each hit has a " + chance + " to lower the attack of " + targets + " to x" + number(ability.value) + " for " + duration + ".";
        case AbilityType::PARALYZE_ATTACK:
            return "Each hit has a " + chance + " to stop " + targets + " (no moving, no attacking) for " + duration + ".";
        case AbilityType::SLOW_ATTACK:
            return "Each hit has a " + chance + " to slow " + targets + " to x" + number(ability.value) + " speed for " + duration + ".";
        default:
            return "";
        }
    }
}

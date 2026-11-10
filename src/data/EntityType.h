#pragma once

/*
 * EntityType
 * ----------
 * Represents an entity type.
 * Types are used as targets for abilities. (for example, a fish could deal more damage to a specific enemy type)
 * In the JSON, types are written as numbers: SHELLED = 0, CRUSTACEAN = 1, FISH = 2, JELLY = 3.
 * New types go at the end (before COUNT) so the numbers already in the JSON keep their meaning.
 *
 * FISH is hidden: every unit gets it automatically (units.json doesn't list types), and it's the
 * default target of enemy abilities, so an enemy ability without "targetTypes" affects every fish.
 */

enum class EntityType
{
	NONE = -1,
	SHELLED,
	CRUSTACEAN,
	FISH,       // Hidden: every unit has it, enemies can't
	JELLY,      // Jellyfish and anemones: status inflicting critters

	COUNT // Not a type: keep it last, the loader uses it to reject unknown type numbers
};

// The types an enemy can have (every type but the hidden FISH)
inline constexpr EntityType ENEMY_TYPES[] = { EntityType::SHELLED, EntityType::CRUSTACEAN, EntityType::JELLY };

// Display name of a type (used by the debug tools)
inline const char* to_string(EntityType type)
{
	switch (type)
	{
	case EntityType::SHELLED:    return "SHELLED";
	case EntityType::CRUSTACEAN: return "CRUSTACEAN";
	case EntityType::FISH:       return "FISH";
	case EntityType::JELLY:      return "JELLY";
	default:                     return "NONE";
	}
}

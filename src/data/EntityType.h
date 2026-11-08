#pragma once

/*
 * EntityType
 * ----------
 * Represents an entity type.
 * Types are used as targets for abilities. (for example, a fish could deal more damage to a specific enemy type)
 */

enum class EntityType
{
	NONE = -1,
	SHELLED,

	COUNT // Not a type: keep it last, the loader uses it to reject unknown type numbers
};
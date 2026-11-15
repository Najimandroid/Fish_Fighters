#pragma once

#include <string>

/*
 * Rarity
 * ------
 * How rare a fish is, like The Battle Cats' Normal / Rare / Super Rare cats. Written in units.json as "rarity":
 * "NORMAL", "RARE" or "SUPER_RARE" (a fish without it is Normal). The stages unlock the Normal fish; the giant clam
 * (Clam Room) gives the rarer ones; the Upgrade screen has a category card for each rarity the player owns a fish of.
 */
enum class Rarity
{
    NORMAL,
    RARE,
    SUPER_RARE,

    COUNT   // Not a rarity: keep it last
};

// The giant clam only gives fish this rare or rarer. While no fish is (no Rare fish exist yet), it gives every fish:
// the Normal ones. The first Rare fish added to units.json makes it give the Rare and Super Rare fish only, by itself
inline constexpr Rarity CLAM_MIN_RARITY = Rarity::RARE;

// units.json's word for it
inline const char* rarity_key(Rarity rarity)
{
    switch (rarity)
    {
    case Rarity::RARE:       return "RARE";
    case Rarity::SUPER_RARE: return "SUPER_RARE";
    default:                 return "NORMAL";
    }
}

// Player-facing name ("Super Rare")
inline const char* rarity_name(Rarity rarity)
{
    switch (rarity)
    {
    case Rarity::RARE:       return "Rare";
    case Rarity::SUPER_RARE: return "Super Rare";
    default:                 return "Normal";
    }
}

// From units.json's word (unknown words: COUNT, the loader rejects them)
inline Rarity rarity_from_key(const std::string& key)
{
    for (int i = 0; i < static_cast<int>(Rarity::COUNT); ++i)
        if (key == rarity_key(static_cast<Rarity>(i))) return static_cast<Rarity>(i);
    return Rarity::COUNT;
}

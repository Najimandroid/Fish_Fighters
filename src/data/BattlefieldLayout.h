#pragma once

/*
 * Battlefield layout
 * ------------------
 * Where things stand in a battle. Shared by the game (Stage) and FishEditor's previews,
 * so both place entities, bases and backgrounds the same way.
 */

// ===== Layers =====
inline constexpr float BATTLE_GROUND_Y = 720.f;    // World y of the feet on the front layer (1080 * 2/3)
inline constexpr int MAX_BATTLE_LAYER = 50;        // Layers 0 (front) to 50 (back): on layer L, an entity stands L px higher
inline constexpr float BACK_LAYER_Y = BATTLE_GROUND_Y - MAX_BATTLE_LAYER; // The bases stand on the last layer

// ===== Backgrounds =====
// Drawn x1.5, copies side by side, the background's ground row on BATTLE_GROUND_Y
inline constexpr float BACKGROUND_SCALE = 1.5f;
inline constexpr float DEFAULT_BACKGROUND_GROUND_RATIO = 613.f / 1120.f; // The 1280 x 1120 template: row 613

// Row of the background placed on the ground: its own (backgrounds.json), or the template's ratio of its height
inline float background_ground_row(float groundRow, float imageHeight)
{
    return groundRow >= 0.f ? groundRow : imageHeight * DEFAULT_BACKGROUND_GROUND_RATIO;
}

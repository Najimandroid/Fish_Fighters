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

// ===== Shadows =====
// Soft round shadows on the ground (like The Battle Cats), drawn before what stands on them
inline constexpr const char* SHADOW_TEXTURE = "assets/images/textures/effects/shadow.png";
inline constexpr float SHADOW_HEIGHT_RATIO = 0.3f;  // Height of a shadow compared to its width
inline constexpr float ENTITY_SHADOW_RISE = 0.25f;  // Center above the feet (part of its height): sprites leave a little space under the drawing
inline constexpr float BASE_SHADOW_WIDTH = 1.1f;    // Compared to the base image width
inline constexpr float BASE_SHADOW_RISE = 0.1f;     // Base images stand on their bottom edge

struct ShadowShape
{
    float centerX = 0.f;
    float centerY = 0.f;
    float width = 0.f;
    float height = 0.f;
};

// The shadow of a unit or an enemy: under its body (the hitbox, the back half of the frame, where the drawing is),
// as wide as the body x shadowScale (EntityData, "shadowScale" in the JSON, 1 by default)
inline ShadowShape entity_shadow(float bodyCenterX, float feetY, float bodyWidth, float shadowScale)
{
    const float width = bodyWidth * shadowScale;
    const float height = width * SHADOW_HEIGHT_RATIO;
    return { bodyCenterX, feetY - height * ENTITY_SHADOW_RISE, width, height };
}

// The shadow of a base: under its whole image, on its bottom edge
inline ShadowShape base_shadow(float imageCenterX, float imageBottom, float imageWidth)
{
    const float width = imageWidth * BASE_SHADOW_WIDTH;
    const float height = width * SHADOW_HEIGHT_RATIO;
    return { imageCenterX, imageBottom - height * BASE_SHADOW_RISE, width, height };
}

#pragma once

#include "../EditorContext.h"

/*
 * EntityFields
 * ------------
 * The fields unit forms and enemies share (same JSON keys, same meaning in the game).
 * Each function draws its rows inside an open property table.
 */
namespace EntityFields
{
    void stats(EditorContext& ctx, DataFile file, ordered_json& entity);     // Health, attack power, knockbacks, speed
    void attack(EditorContext& ctx, DataFile file, ordered_json& entity);    // Type, range, damage zone, timings
    // Sheet, frames, knockback frame; "Import .mdp..." makes the sheet and the icon (iconPath) from a FireAlpaca drawing
    void sprite(EditorContext& ctx, DataFile file, ordered_json& entity, ImageFolder folder, const std::string& iconPath);

    // The icon at iconPath (200 x 150): shown, imported, or made from the first frame of entity's sprite sheet
    void icon(EditorContext& ctx, const std::string& iconPath, const char* label, const char* help, const ordered_json& entity);

    // Damage per second of one target, without abilities ("x" multiplies the attack: level or stage)
    float damage_per_second(const ordered_json& entity, float multiplier = 1.f);
}

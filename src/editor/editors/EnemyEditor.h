#pragma once

#include "../EditorContext.h"

/*
 * EnemyEditor
 * -----------
 * An enemy (enemies.json): name, types, stats, attack, abilities and sprite sheet.
 * Stages scale its health and attack with their own multipliers ("magnification").
 */
namespace EnemyEditor
{
    void draw(EditorContext& ctx);
}

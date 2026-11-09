#pragma once

#include "../EditorContext.h"

/*
 * StageEditor
 * -----------
 * A stage (stages.json): rules, enemy base and background images, enemy waves (with a
 * timeline of when each wave spawns) and rewards.
 */
namespace StageEditor
{
    void draw(EditorContext& ctx);
}

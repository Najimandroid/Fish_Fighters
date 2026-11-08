#pragma once

#include "../EditorContext.h"

/*
 * ChapterEditor
 * -------------
 * A chapter (chapters.json): name, description, map image, and the stages on the map.
 * Stage points are dragged on the map like the game draws them; their order is the path.
 */
namespace ChapterEditor
{
    void draw(EditorContext& ctx);
}

#pragma once

#include "EditorContext.h"

/*
 * Validator
 * ---------
 * Checks the project data the way the game will see it.
 *
 *  1. The game's own DataLoader loads a copy of the edited files (in a temporary folder),
 *     so whatever it refuses is reported with its exact message
 *  2. Checks the loader doesn't do: missing images, UIDs used by stages and chapters that
 *     don't exist, sprite sheets that don't split into the frame count, form levels...
 */
namespace Validator
{
    ValidationReport run(const ProjectData& project, TextureCache& textures);
}

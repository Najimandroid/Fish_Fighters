#pragma once

#include <string>

/*
 * BackgroundData
 * --------------
 * What the game needs to know about a stage background image, loaded from game_data/backgrounds.json
 * (keyed by the image's path). Every stage using the image shares it.
 *
 *   {
 *     "assets/images/textures/backgrounds/stages/CoralSeabed.png": { "groundRow": 791 }
 *   }
 *
 * An image that isn't listed uses the defaults (see BattlefieldLayout.h).
 */
struct BackgroundData
{
    std::string texture;      // "assets/..." path of the image (its key in backgrounds.json)
    float groundRow = -1.f;   // Row of the image the units stand on (front layer), -1 = the template's ratio
};

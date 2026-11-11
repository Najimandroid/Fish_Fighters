#pragma once

#include <SFML/Graphics.hpp>
#include <string>

struct EntityData;

/*
 * EntityPortraits
 * ---------------
 * How units and enemies are pictured in the menus (chapter map, encyclopedia):
 * a unit by its icon, an enemy by its icon too (icons/enemies/icon_<UID>.png). An enemy with no icon yet is shown
 * by the first frame of its sprite sheet, only its body (the back half of the frame, where the drawing is:
 * enemies face right, so the left half). Both can be drawn as a black
 * silhouette (the drawing and its holes filled), like The Battle Cats shows what the player hasn't met yet.
 */

namespace EntityPortraits
{

    // A unit's icon (DataLoader::get_unit_icon_texture_path) cropped to fill area
    void draw_unit(sf::RenderTarget& target, const std::string& iconPath, const sf::FloatRect& area, bool isSilhouette = false);

    // An enemy: its icon (DataLoader::get_enemy_icon_texture_path) cropped to fill area, or without one its body in
    // the first frame of its sheet, fitted inside area
    void draw_enemy(sf::RenderTarget& target, const EntityData& enemy, const std::string& iconPath, const sf::FloatRect& area,
                    bool isSilhouette = false);
}

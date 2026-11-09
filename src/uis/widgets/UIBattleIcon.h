#pragma once

#include "UIButtonElement.h"
#include <string>

/*
 * UIBattleIcon
 * ------------
 * Represents a clickable icon in the deployment UI for spawning units during a stage.
 * - Inherits from UIButtonElement for click handling
 * - Drawn like a Battle Cats deploy slot: light blue slot, the unit icon under a rounded frame,
 *   the cost in yellow outlined digits, a cyan bar while it cools down, darkened when it can't be used
 */

class UIBattleIcon : public UIButtonElement
{
public:

    // Size of a slot on screen
    static constexpr float WIDTH = 166.f;
    static constexpr float HEIGHT = 124.f;

    // Constructor
    // position: position of the icon on the screen
    // iconTexturePath: path to the unit's icon texture
    UIBattleIcon(sf::Vector2f position, const std::string& iconTexturePath);

    // Updates cooldown progress and price text
    void update(float deltaTime) override;

    // Draws the slot, unit icon, shade, cooldown bar, frame and price
    void render(sf::RenderWindow& window) override;

    // Getters
    int get_uid() const;                // returns the unit ID
    bool get_if_on_cooldown() const;    // returns true if icon is cooling down
    int get_cost() const;               // returns the unit cost

    // Starts the cooldown timer and visual overlay
    void start_cooldown();

    // Setters
    void set_uid(int uid);
    void set_max_cooldown(float cooldown);
    void set_cost(int cost);
    void set_texture(const std::string& texturePath);
    void set_darkened(bool isDarkened); // visually darkens the icon when unavailable

private:

    // Unit info
    int m_uid = -1;           // Unique ID of the unit
    int m_cost = 50;          // Cost to deploy the unit
    bool m_isOnCooldown = false;
    float m_currentCooldown = 0.0f;
    float m_maxCooldown = 1.0f;
    bool m_isDarkened = false;

    // Visuals
    sf::Texture m_texture;    // Unit icon texture
    sf::Sprite m_sprite;      // Sprite to display the texture
};

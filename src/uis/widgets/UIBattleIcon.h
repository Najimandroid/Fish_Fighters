#pragma once

#include "UIButtonElement.h"
#include <string>

/*
 * UIBattleIcon
 * ------------
 * Represents a clickable icon in the deployment UI for spawning units during a stage.
 * - Inherits from UIButtonElement for click handling
 * - Displays a unit icon, cost, and cooldown overlay
 */

class UIBattleIcon : public UIButtonElement
{
public:

    // Constructor
    // position: position of the icon on the screen
    // iconTexturePath: path to the unit's icon texture
    UIBattleIcon(sf::Vector2f position, const std::string& iconTexturePath);

    // Updates cooldown progress, price text, and overlay
    void update(float deltaTime) override;

    // Draws the icon, cooldown overlay, price, and unit texture
    void render(sf::RenderWindow& window) override;

    // Getters
    int get_uid() const;                // returns the unit ID
    bool get_if_on_cooldown() const;    // returns true if icon is cooling down
    int get_cost() const;               // returns the unit cost

    // Starts the cooldown timer and visual overlay
    void start_cooldown();

    // Setters
    void set_uid(int uid);
    void set_max_cooldown(int cooldown);
    void set_cost(int cost);
    void set_texture(const std::string& texturePath);
    void set_darkened(bool isDarkened); // visually darkens the icon when unavailable

private:

    // Unit info
    int m_uid = -1;           // Unique ID of the unit
    int m_cost = 50;          // Cost to deploy the unit
    bool m_isOnCooldown = false;
    float m_currentCooldown = 0.0f;
    int m_maxCooldown = 1;

    // Visuals
    sf::Texture m_texture;    // Unit icon texture
    sf::Sprite m_sprite;      // Sprite to display the texture
    sf::RectangleShape m_blackFilter;  // overlay to darken icon
    sf::RectangleShape m_barBackground; // cooldown bar background
    sf::RectangleShape m_barFill;       // cooldown bar fill
};

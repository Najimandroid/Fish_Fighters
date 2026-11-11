#pragma once

#include "UIButtonElement.h"

class Stage;

/*
 * UICashUp
 * ---------
 * A UI button that allows the player to upgrade cash-related attributes during a stage
 * (bottom left of the battle, drawn in a slot frame like the deploy slots):
 * - Maximum cash capacity
 * - Cash production speed
 *
 * Inherits from UIButtonElement to handle click interactions and rendering.
 */

class UICashUp : public UIButtonElement
{
public:

    /*
     * Initializes the button with the given stage reference.
     * Sets up the icon, text, and callback for upgrade logic.
     */
    UICashUp(std::shared_ptr<Stage> stage);

    /*
     * Updates the button each frame.
     * - Changes icon and text color depending on whether the upgrade is affordable.
     * - Updates text to show current level, cost, or maxed status.
     */
    void update(float deltaTime) override;

    // Draws the button, icon, and text to the given window.
    void render(sf::RenderWindow& window) override;

    // The tutorial's questions: its level, and whether the next one can be bought now
    int get_level() const { return m_level; }
    bool can_upgrade() const { return m_level < m_maxLevel && m_isAffordable; }

private:

    int m_level = 1;      // Current upgrade level
    int m_maxLevel = 3;   // Maximum upgrade level allowed
    int m_cost = 250;     // Cost for the next upgrade

    sf::Sprite m_icon;    // Icon representing the upgrade
    sf::Texture m_texture;// Texture used for the icon
    sf::Texture m_disabledTexture; // Icon shown when the upgrade isn't affordable
    sf::Text m_levelText; // "LEVEL 1" at the top (m_text shows the cost at the bottom)
    bool m_isAffordable = false;

    std::weak_ptr<Stage> m_stage; // Reference to the current stage (weak to avoid circular ownership)
};

#pragma once

#include "UIButtonElement.h"

class Stage;

/*
 * UICashUp
 * ---------
 * A UI button that allows the player to upgrade cash-related attributes during a stage:
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

private:

    int m_level = 1;      // Current upgrade level
    int m_maxLevel = 3;   // Maximum upgrade level allowed
    int m_cost = 250;     // Cost for the next upgrade

    sf::Sprite m_icon;    // Icon representing the upgrade
    sf::Texture m_texture;// Texture used for the icon

    std::weak_ptr<Stage> m_stage; // Reference to the current stage (weak to avoid circular ownership)
};

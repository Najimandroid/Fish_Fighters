#pragma once

#include "UIElement.h"

class Stage;

/*
 * UIBaseHealthInfo
 * ----------------
 * Displays the current health of both the player's base and the enemies' base during a stage.
 */

class UIBaseHealthInfo : public UIElement
{
public:

    /*
     * @param stage - shared pointer to the current Stage
     * Initializes fonts and text objects for both bases.
     */
    UIBaseHealthInfo(std::shared_ptr<Stage> stage);

    // Updates the displayed health values and positions above each base.
    void update(float deltaTime) override;

    // Draws the base health texts to the window.
    void render(sf::RenderWindow& window) override;

private:

    std::weak_ptr<Stage> m_stage; // Weak reference to the current stage

    sf::Font m_font;             // Font used for the health text
    sf::Text m_textUnitBase;     // Displays player's base health
    sf::Text m_textEnemyBase;    // Displays enemy's base health
};

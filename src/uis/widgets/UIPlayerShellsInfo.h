#pragma once

#include "UITextElement.h"

class DataLoader;

/*
 * UIPlayerShellsInfo
 * ------------------
 * Displays the current number of player "shells" (currency) on the UI:
 * a dark pill at the right end of the title bar, with a shell icon and yellow digits
 * (like the XP counter of The Battle Cats).
 * Inherits from UITextElement for text rendering.
 * Continuously updates to reflect changes in the player's shells.
 * The same pill shows the Shiny Scales (the Clam Room's currency), further left so both fit.
 */

class UIPlayerShellsInfo : public UITextElement
{
public:

    // Constructor
    // dataLoader: reference to DataLoader to access player data
    enum class Currency { SHELLS, SHINY_SCALES };
    UIPlayerShellsInfo(std::shared_ptr<DataLoader> dataLoader, Currency currency = Currency::SHELLS);

    // Update the displayed shell count
    // deltaTime is not used but required to override base class
    void update(float deltaTime) override;

    // Pill, icon and digits
    void render(sf::RenderWindow& window) override;

private:

    std::shared_ptr<DataLoader> m_dataLoader; // Reference to DataLoader to access player info
    Currency m_currency = Currency::SHELLS;

    const float OFFSET_Y = 18.f;      // Vertical padding from the top
    const float OFFSET_X = 24.f;      // Horizontal padding from the right
    const float PILL_HEIGHT = 74.f;
    const float MIN_PILL_WIDTH = 300.f;
};

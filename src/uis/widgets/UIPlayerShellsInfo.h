#pragma once

#include "UITextElement.h"

class DataLoader;

/*
 * UIPlayerShellsInfo
 * ------------------
 * Displays the current number of player "shells" (currency) on the UI.
 * Inherits from UITextElement for text rendering.
 * Continuously updates to reflect changes in the player's shells.
 */

class UIPlayerShellsInfo : public UITextElement
{
public:

    // Constructor
    // dataLoader: reference to DataLoader to access player data
    UIPlayerShellsInfo(std::shared_ptr<DataLoader> dataLoader);

    // Update the displayed shell count
    // deltaTime is not used but required to override base class
    void update(float deltaTime) override;

private:

    std::shared_ptr<DataLoader> m_dataLoader; // Reference to DataLoader to access player info

    const float OFFSET_Y = 10.f; // Vertical padding from the top
    const float OFFSET_X = 10.f; // Horizontal padding from the right
};

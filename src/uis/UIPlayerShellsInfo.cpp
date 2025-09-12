#include "UIPlayerShellsInfo.h"
#include "../core/DataLoader.h"
#include <iostream>

/*
 * Initializes text properties (size, color, outline) for displaying shell count.
 */
UIPlayerShellsInfo::UIPlayerShellsInfo(std::shared_ptr<DataLoader> dataLoader) :
    UITextElement({ 1.f, 1.f }, { 1.f, 1.f }),
    m_dataLoader(dataLoader)
{
    m_text.setCharacterSize(72);              // Large text for visibility
    m_text.setFillColor(sf::Color::Cyan);     // Text color
    m_text.setOutlineColor(sf::Color::Black); // Outline color for contrast
    m_text.setOutlineThickness(3.f);          // Thickness of outline
}

/*
 * Updates the displayed shell count each frame.
 * Sets text to "[shells]¤" and positions it at the top-right corner.
 */
void UIPlayerShellsInfo::update(float deltaTime)
{
    // Get current shell count from player data and update text
    m_text.setString(std::to_string(m_dataLoader->get_player_data().lock()->shells) + "¤");

    // Set origin to the right of the text for proper alignment
    auto bounds = m_text.getLocalBounds();
    m_text.setOrigin({
        bounds.position.x + bounds.size.x,
        bounds.position.y
        });

    // Position text at the top-right corner with offsets
    m_text.setPosition({ 1920.f - OFFSET_Y, 0.f + OFFSET_Y });
}

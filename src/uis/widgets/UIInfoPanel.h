#pragma once

#include "UIElement.h"
#include "UIPanel.h"
#include "UIRoundButton.h"

#include <string>
#include <vector>

/*
 * UIInfoPanel
 * -----------
 * The help window an "i" opens (UIInfoButton): a title and a short explanation of a screen or a feature, on the dark
 * panel over the dimmed screen. It closes with its X, a tap outside it, Escape or Enter.
 * Add it after the screen's other elements (a window: on top, the only one getting input while open).
 *
 * The text can have paragraphs (an empty line between them).
 *
 * Example usage:
 *   auto info = std::make_shared<UIInfoPanel>("School Rank", "Every fish's level, added up...");
 *   infoButton->set_callback([weak = std::weak_ptr(info)]() { if (auto panel = weak.lock()) panel->open(); });
 */
class UIInfoPanel : public UIElement
{
public:
    UIInfoPanel(const std::string& title, const std::string& text);

    void open();
    void close();

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override;     // The whole screen while open
    bool is_blocking_input() const override;

private:
    sf::FloatRect m_box;
    UIPanel m_panel;
    UIRoundButton m_closeButton;
    sf::Text m_title;
    sf::Text m_text;
    bool m_isPressedWhileOpen = false;   // A tap outside closes it only if it began while it was open
};

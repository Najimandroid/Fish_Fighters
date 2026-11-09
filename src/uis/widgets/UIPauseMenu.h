#pragma once

#include "UIPanel.h"
#include "UIButtonElement.h"

/*
 * UIPauseMenu
 * -----------
 * The window shown over the battle when it's paused: the stage name, "Continue" and "Retreat".
 * Opened by the pause button or Escape, closed by "Continue" or Escape.
 * While open it dims the battle and takes every input event (is_blocking_input).
 *
 * It doesn't change the game speed itself: UIManager gives it callbacks that do.
 */

class UIPauseMenu : public UIElement
{
public:

    UIPauseMenu(const std::string& stageName);

    // Called when the menu asks to open (Escape while hidden): pauses the battle and calls open()
    void set_open_callback(std::function<void()> callback);

    // Called by "Continue" and Escape: resumes the battle and calls close()
    void set_continue_callback(std::function<void()> callback);

    // Called by "Retreat": leaves the battle
    void set_retreat_callback(std::function<void()> callback);

    // The menu closes itself when the battle isn't paused anymore (debug keys, developer tools)
    void set_is_paused_callback(std::function<bool()> callback);

    void open();
    void close();

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

    // The whole screen while open (nothing behind can be clicked), nothing while closed
    sf::FloatRect get_bounds() const override;
    bool is_blocking_input() const override;

private:

    sf::RectangleShape m_dim;          // Darkens the battle
    UIPanel m_panel;
    sf::Text m_title;                  // "PAUSED"
    sf::Text m_stageName;
    UIButtonElement m_continueButton;
    UIButtonElement m_retreatButton;

    std::function<void()> m_openCallback;
    std::function<void()> m_continueCallback;
    std::function<bool()> m_isPausedCallback;
};

#pragma once

#include "UIPanel.h"
#include "UIButtonElement.h"

#include <functional>
#include <string>

/*
 * UIConfirmDialog
 * ---------------
 * "Are you sure?" before an action that can't be undone (quitting the game, retreating from a battle).
 * A dark panel over a dimmed screen with a question, a line of explanation and two buttons:
 * the action on the left (orange), the safe choice on the right (yellow).
 * Escape and Enter pick the safe choice. While open it takes every input event (is_blocking_input),
 * so add it after the other elements of the screen.
 */
class UIConfirmDialog : public UIElement
{
public:
    UIConfirmDialog(const std::string& question, const std::string& explanation,
        const std::string& confirmLabel, const std::string& cancelLabel);

    // Shows the dialog; onConfirm runs if the player confirms (the dialog closes either way)
    void open(std::function<void()> onConfirm);
    void close();

    // Sound of the confirm button ("ui_back" by default)
    void set_confirm_sound(const std::string& soundName);

    // Changes what it asks (a dialog reused for different things: selling 3 Clownfish, crafting...)
    void set_texts(const std::string& question, const std::string& explanation, const std::string& confirmLabel);

    // The confirm button's color: orange by default (danger), white keeps the yellow (a good action)
    void set_confirm_tint(const sf::Color& tint);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

    // The whole screen while open (nothing behind can be clicked), nothing while closed
    sf::FloatRect get_bounds() const override;
    bool is_blocking_input() const override;

private:
    sf::RectangleShape m_dim;          // Darkens the screen behind
    UIPanel m_panel;
    sf::Text m_question;
    sf::Text m_explanation;
    UIButtonElement m_confirmButton;   // Left, orange
    UIButtonElement m_cancelButton;    // Right, yellow: the safe choice

    std::function<void()> m_onConfirm;
};

#include "UIPauseMenu.h"
#include "../UISkin.h"

namespace
{
    const sf::Vector2f SCREEN_SIZE{ 1920.f, 1080.f };
    const sf::Vector2f PANEL_SIZE{ 640.f, 590.f };
    const sf::Vector2f PANEL_POSITION = (SCREEN_SIZE - PANEL_SIZE) / 2.f;
    const sf::Vector2f BUTTON_SIZE{ 400.f, 96.f };
    const float BUTTON_X = (SCREEN_SIZE.x - BUTTON_SIZE.x) / 2.f;
    const sf::Color RETREAT_TINT{ 255, 150, 130 };   // Turns the yellow button orange
}

UIPauseMenu::UIPauseMenu(const std::string& stageName) :
    UIElement(SCREEN_SIZE, { 0.f, 0.f }),
    m_dim(SCREEN_SIZE),
    m_panel(PANEL_SIZE, PANEL_POSITION, UISkin::PANEL_DARK, 1.2f),
    m_title(UISkin::heavy_font(), "PAUSED"),
    m_stageName(UISkin::heavy_font(), stageName),
    m_continueButton(BUTTON_SIZE, { BUTTON_X, PANEL_POSITION.y + 210.f }, "CONTINUE"),
    m_optionsButton(BUTTON_SIZE, { BUTTON_X, PANEL_POSITION.y + 330.f }, "OPTIONS"),
    m_retreatButton(BUTTON_SIZE, { BUTTON_X, PANEL_POSITION.y + 450.f }, "RETREAT")
{
    m_dim.setFillColor(sf::Color(0, 0, 0, 150));

    UISkin::style_text(m_title, 72);
    UISkin::set_text_anchor(m_title, { 0.5f, 0.5f });
    m_title.setPosition({ SCREEN_SIZE.x / 2.f, PANEL_POSITION.y + 75.f });

    UISkin::style_text(m_stageName, 36, UISkin::NUMBER);
    UISkin::set_text_anchor(m_stageName, { 0.5f, 0.5f });
    m_stageName.setPosition({ SCREEN_SIZE.x / 2.f, PANEL_POSITION.y + 150.f });

    m_continueButton.set_text_size(40);
    m_retreatButton.set_text_size(40);
    m_optionsButton.set_text_size(40);
    m_optionsButton.set_style(UIButtonElement::Style::Secondary);
    m_retreatButton.set_fill_color(RETREAT_TINT);
    m_continueButton.set_click_sound("");          // UIManager's continue callback plays "ui_unpause"
    // RETREAT asks first (UIManager's confirm dialog plays "ui_back" if the player confirms)

    m_continueButton.set_callback([this]() { if (m_continueCallback) m_continueCallback(); });

    m_isVisible = false;
}

void UIPauseMenu::set_open_callback(std::function<void()> callback)
{
    m_openCallback = callback;
}

void UIPauseMenu::set_continue_callback(std::function<void()> callback)
{
    m_continueCallback = callback;
}

void UIPauseMenu::set_retreat_callback(std::function<void()> callback)
{
    m_retreatButton.set_callback(callback);
}

void UIPauseMenu::set_options_callback(std::function<void()> callback)
{
    m_optionsButton.set_callback(callback);
}

void UIPauseMenu::set_is_paused_callback(std::function<bool()> callback)
{
    m_isPausedCallback = callback;
}

void UIPauseMenu::open()
{
    m_isVisible = true;
}

void UIPauseMenu::close()
{
    m_isVisible = false;
}

void UIPauseMenu::update(float deltaTime)
{
    if (m_isVisible && m_isPausedCallback && !m_isPausedCallback())
        close();

    m_continueButton.update(deltaTime);
    m_optionsButton.update(deltaTime);
    m_retreatButton.update(deltaTime);
}

void UIPauseMenu::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    window.draw(m_dim);
    m_panel.render(window);
    window.draw(m_title);
    window.draw(m_stageName);
    m_continueButton.render(window);
    m_optionsButton.render(window);
    m_retreatButton.render(window);
}

void UIPauseMenu::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    // Escape, P or Space opens the menu, or works like "Continue" when it's open
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyPressed->code == sf::Keyboard::Key::Escape || keyPressed->code == sf::Keyboard::Key::P
            || keyPressed->code == sf::Keyboard::Key::Space)
        {
            if (m_isVisible && m_continueCallback) m_continueCallback();
            else if (!m_isVisible && m_openCallback) m_openCallback();
            return;
        }
    }

    if (!m_isVisible) return;

    m_continueButton.handle_event(event, window);
    m_optionsButton.handle_event(event, window);
    m_retreatButton.handle_event(event, window);
}

sf::FloatRect UIPauseMenu::get_bounds() const
{
    if (!m_isVisible) return sf::FloatRect{ { 0.f, 0.f }, { 0.f, 0.f } };
    return sf::FloatRect{ m_position, m_size };
}

bool UIPauseMenu::is_blocking_input() const
{
    return m_isVisible;
}

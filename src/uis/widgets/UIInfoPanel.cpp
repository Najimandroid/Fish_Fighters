#include "UIInfoPanel.h"
#include "../UISkin.h"
#include "SFML/Window/Event.hpp"

#include <algorithm>

namespace
{
    const float WIDTH = 1180.f;
    const float PADDING = 64.f;
    const float TITLE_BAND = 96.f;          // The title and the gap under it
    const unsigned int TITLE_SIZE = 52;
    const unsigned int TEXT_SIZE = 32;
    const float LINE_SPACING = 1.15f;
    const float CLOSE_SIZE = 96.f;

    std::string wrapped(const std::string& text)
    {
        return UISkin::wrap_text(text, TEXT_SIZE, WIDTH - 2.f * PADDING, UISkin::regular_font());
    }

    // The panel fits its text, centered on the screen
    sf::FloatRect box_for(const std::string& text)
    {
        sf::Text measure(UISkin::regular_font(), wrapped(text), TEXT_SIZE);
        measure.setLineSpacing(LINE_SPACING);
        const float height = std::min(940.f, PADDING + TITLE_BAND + measure.getLocalBounds().size.y + PADDING);
        return { { (1920.f - WIDTH) / 2.f, (1080.f - height) / 2.f }, { WIDTH, height } };
    }

    // The X on the panel's top right corner, half out of it
    sf::Vector2f close_position(const sf::FloatRect& box)
    {
        return box.position + sf::Vector2f{ box.size.x - CLOSE_SIZE * 0.6f, -CLOSE_SIZE * 0.4f };
    }
}

UIInfoPanel::UIInfoPanel(const std::string& title, const std::string& text) :
    UIElement({ 1920.f, 1080.f }, { 0.f, 0.f }),
    m_box(box_for(text)),
    m_panel(m_box.size, m_box.position, UISkin::PANEL_DARK, 1.2f),
    m_closeButton(CLOSE_SIZE, close_position(m_box), "icon_close"),
    m_title(UISkin::heavy_font(), title),
    m_text(UISkin::regular_font())
{
    UISkin::style_text(m_title, TITLE_SIZE, UISkin::NUMBER);
    UISkin::set_text_anchor(m_title, { 0.f, 0.f });
    m_title.setPosition(m_box.position + sf::Vector2f{ PADDING, PADDING - 12.f });

    UISkin::style_body_text(m_text, TEXT_SIZE);
    m_text.setLineSpacing(LINE_SPACING);
    m_text.setString(wrapped(text));
    UISkin::set_text_anchor(m_text, { 0.f, 0.f });
    m_text.setPosition(m_box.position + sf::Vector2f{ PADDING, PADDING + TITLE_BAND });

    m_closeButton.set_click_sound("ui_back");
    m_closeButton.set_callback([this]() { close(); });
    m_isVisible = false;
}

void UIInfoPanel::open()
{
    m_isVisible = true;
    m_isPressedWhileOpen = false;
}

void UIInfoPanel::close()
{
    m_isVisible = false;
}

void UIInfoPanel::update(float deltaTime)
{
    if (m_isVisible) m_closeButton.update(deltaTime);
}

void UIInfoPanel::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    sf::RectangleShape dim({ 1920.f, 1080.f });
    dim.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(dim);
    m_panel.render(window);
    window.draw(m_title);
    window.draw(m_text);
    m_closeButton.render(window);
}

void UIInfoPanel::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    m_closeButton.handle_event(event, window);
    if (!m_isVisible) return;

    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
    {
        if (key->code == sf::Keyboard::Key::Escape || key->code == sf::Keyboard::Key::Enter)
        {
            play_sound("ui_back");
            close();
        }
    }
    else if (event.is<sf::Event::MouseButtonPressed>())
    {
        m_isPressedWhileOpen = true;
    }
    else if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>())
    {
        // A tap outside the panel closes it too (a phone has no Escape); only a tap begun while it's open (the release
        // of the tap on the "i" that opened it reaches it too)
        const sf::Vector2f point = window.mapPixelToCoords(released->position);
        const bool wasPressed = m_isPressedWhileOpen;
        m_isPressedWhileOpen = false;
        if (wasPressed && released->button == sf::Mouse::Button::Left && !m_box.contains(point) && !m_closeButton.get_bounds().contains(point))
        {
            play_sound("ui_back");
            close();
        }
    }
}

sf::FloatRect UIInfoPanel::get_bounds() const
{
    return m_isVisible ? sf::FloatRect{ { 0.f, 0.f }, { 1920.f, 1080.f } } : sf::FloatRect{};
}

bool UIInfoPanel::is_blocking_input() const
{
    return m_isVisible;
}

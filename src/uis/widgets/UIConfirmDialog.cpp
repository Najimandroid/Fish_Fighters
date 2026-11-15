#include "UIConfirmDialog.h"
#include "../UISkin.h"

namespace
{
    const sf::Vector2f SCREEN_SIZE{ 1920.f, 1080.f };
    // A message box (UISkin::draw_message_box, like The Battle Cats' dialogs), its buttons straddling its bottom edge
    const sf::Vector2f PANEL_SIZE{ 840.f, 300.f };
    const sf::Vector2f PANEL_POSITION{ (SCREEN_SIZE.x - PANEL_SIZE.x) / 2.f, (SCREEN_SIZE.y - PANEL_SIZE.y) / 2.f - 40.f };
    const sf::Vector2f BUTTON_SIZE{ 340.f, 96.f };   // "KEEP FIGHTING" fits
    const float BUTTON_GAP = 40.f;
    const float BUTTON_Y = PANEL_POSITION.y + PANEL_SIZE.y - BUTTON_SIZE.y * 0.4f;
    const float EXPLANATION_WIDTH = PANEL_SIZE.x - 120.f;
    const unsigned int QUESTION_SIZE = 48;
    const unsigned int EXPLANATION_SIZE = 30;
    const sf::Color DANGER_TINT{ 255, 150, 130 };   // Turns the yellow button orange (like QUIT and RETREAT)
}

UIConfirmDialog::UIConfirmDialog(const std::string& question, const std::string& explanation,
    const std::string& confirmLabel, const std::string& cancelLabel) :
    UIElement(SCREEN_SIZE, { 0.f, 0.f }),
    m_dim(SCREEN_SIZE),
    m_question(UISkin::heavy_font(), question),
    m_explanation(UISkin::regular_font()),
    m_confirmButton(BUTTON_SIZE, { SCREEN_SIZE.x / 2.f - BUTTON_GAP / 2.f - BUTTON_SIZE.x, BUTTON_Y }, confirmLabel),
    m_cancelButton(BUTTON_SIZE, { SCREEN_SIZE.x / 2.f + BUTTON_GAP / 2.f, BUTTON_Y }, cancelLabel)
{
    m_dim.setFillColor(sf::Color(0, 0, 0, 150));

    UISkin::style_text(m_question, QUESTION_SIZE);
    UISkin::set_text_anchor(m_question, { 0.5f, 0.5f });
    m_question.setPosition({ SCREEN_SIZE.x / 2.f, PANEL_POSITION.y + 74.f });

    UISkin::style_body_text(m_explanation, EXPLANATION_SIZE, UISkin::TITLE);
    m_explanation.setString(UISkin::wrap_text(explanation, EXPLANATION_SIZE, EXPLANATION_WIDTH, UISkin::regular_font()));
    UISkin::set_text_anchor(m_explanation, { 0.5f, 0.5f });
    m_explanation.setPosition({ SCREEN_SIZE.x / 2.f, PANEL_POSITION.y + 158.f });

    m_confirmButton.set_text_size(38);
    m_confirmButton.set_fill_color(DANGER_TINT);
    m_confirmButton.set_click_sound("ui_back");
    m_confirmButton.set_callback([this]()
        {
            auto onConfirm = m_onConfirm;
            close();
            if (onConfirm) onConfirm();
        });

    m_cancelButton.set_text_size(38);   // The safe choice stays yellow: it's the one to take, framed in pink
    m_cancelButton.set_selected(true);
    m_cancelButton.set_callback([this]() { close(); });

    m_isVisible = false;
}

void UIConfirmDialog::open(std::function<void()> onConfirm)
{
    m_onConfirm = std::move(onConfirm);
    m_isVisible = true;
}

void UIConfirmDialog::close()
{
    m_isVisible = false;
    m_onConfirm = nullptr;
}

void UIConfirmDialog::set_confirm_sound(const std::string& soundName)
{
    m_confirmButton.set_click_sound(soundName);
}

void UIConfirmDialog::set_texts(const std::string& question, const std::string& explanation, const std::string& confirmLabel)
{
    m_question.setString(question);
    UISkin::set_text_anchor(m_question, { 0.5f, 0.5f });
    m_explanation.setString(UISkin::wrap_text(explanation, EXPLANATION_SIZE, EXPLANATION_WIDTH, UISkin::regular_font()));
    UISkin::set_text_anchor(m_explanation, { 0.5f, 0.5f });
    m_confirmButton.set_text(confirmLabel);
}

void UIConfirmDialog::set_confirm_tint(const sf::Color& tint)
{
    m_confirmButton.set_fill_color(tint);
}

void UIConfirmDialog::update(float deltaTime)
{
    m_confirmButton.update(deltaTime);
    m_cancelButton.update(deltaTime);
}

void UIConfirmDialog::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    window.draw(m_dim);
    UISkin::draw_message_box(window, { PANEL_POSITION, PANEL_SIZE });
    window.draw(m_question);
    window.draw(m_explanation);
    m_confirmButton.render(window);
    m_cancelButton.render(window);
}

void UIConfirmDialog::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    // Escape and Enter: the safe choice
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyPressed->code == sf::Keyboard::Key::Escape || keyPressed->code == sf::Keyboard::Key::Enter)
        {
            play_sound("ui_click");
            close();
            return;
        }
    }

    m_confirmButton.handle_event(event, window);
    m_cancelButton.handle_event(event, window);
}

sf::FloatRect UIConfirmDialog::get_bounds() const
{
    if (!m_isVisible) return sf::FloatRect{ { 0.f, 0.f }, { 0.f, 0.f } };
    return sf::FloatRect{ m_position, m_size };
}

bool UIConfirmDialog::is_blocking_input() const
{
    return m_isVisible;
}

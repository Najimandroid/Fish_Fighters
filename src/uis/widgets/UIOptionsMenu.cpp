#include "UIOptionsMenu.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"
#include "../../audio/AudioManager.h"

namespace
{
    const sf::Vector2f SCREEN_SIZE{ 1920.f, 1080.f };
    const sf::Vector2f PANEL_SIZE{ 1000.f, 860.f };
    const sf::Vector2f PANEL_POSITION{ (SCREEN_SIZE.x - PANEL_SIZE.x) / 2.f, 110.f };
    const float LEFT = PANEL_POSITION.x + 70.f;          // Row names and section headers
    const float SLIDER_X = PANEL_POSITION.x + 340.f;
    const float SLIDER_WIDTH = 440.f;
    const float SLIDER_HEIGHT = 22.f;
    const float MASTER_Y = 305.f, MUSIC_Y = 375.f, EFFECTS_Y = 445.f, MUTE_Y = 520.f;   // Row centers
    const unsigned int HEADER_SIZE = 34;
    const unsigned int ROW_SIZE = 34;
}

UIOptionsMenu::UIOptionsMenu(std::shared_ptr<DataLoader> dataLoader, std::shared_ptr<AudioManager> audio,
    std::function<void(bool)> setFullscreen) :
    UIElement(SCREEN_SIZE, { 0.f, 0.f }),
    m_dataLoader(dataLoader), m_audio(audio), m_setFullscreen(std::move(setFullscreen)),
    m_dim(SCREEN_SIZE),
    m_panel(PANEL_SIZE, PANEL_POSITION, UISkin::PANEL_DARK, 1.2f),
    m_masterSlider({ SLIDER_X, MASTER_Y - SLIDER_HEIGHT / 2.f }, SLIDER_WIDTH, 1.f),
    m_musicSlider({ SLIDER_X, MUSIC_Y - SLIDER_HEIGHT / 2.f }, SLIDER_WIDTH, 1.f),
    m_effectsSlider({ SLIDER_X, EFFECTS_Y - SLIDER_HEIGHT / 2.f }, SLIDER_WIDTH, 1.f),
    m_muteButton({ 200.f, 70.f }, { SLIDER_X + SLIDER_WIDTH - 200.f + 130.f, MUTE_Y - 35.f }, "ON"),
    m_fullscreenButton({ 320.f, 80.f }, { LEFT, 635.f }, "FULLSCREEN"),
    m_windowedButton({ 320.f, 80.f }, { LEFT + 350.f, 635.f }, "WINDOWED"),
    m_closeButton({ 280.f, 84.f }, { (SCREEN_SIZE.x - 280.f) / 2.f, 850.f }, "CLOSE")
{
    m_dim.setFillColor(sf::Color(0, 0, 0, 150));

    // Static texts
    auto addText = [this](const std::string& text, unsigned int size, sf::Color color, sf::Vector2f position, sf::Vector2f anchor)
        {
            sf::Text label(UISkin::heavy_font(), text);
            UISkin::style_text(label, size, color);
            UISkin::set_text_anchor(label, anchor);
            label.setPosition(position);
            m_labels.push_back(label);
        };
    addText("OPTIONS", 64, UISkin::TEXT, { SCREEN_SIZE.x / 2.f, 175.f }, { 0.5f, 0.5f });
    addText("AUDIO", HEADER_SIZE, UISkin::TITLE, { LEFT, 245.f }, { 0.f, 0.5f });
    addText("Master", ROW_SIZE, UISkin::TEXT, { LEFT, MASTER_Y }, { 0.f, 0.5f });
    addText("Music", ROW_SIZE, UISkin::TEXT, { LEFT, MUSIC_Y }, { 0.f, 0.5f });
    addText("Effects", ROW_SIZE, UISkin::TEXT, { LEFT, EFFECTS_Y }, { 0.f, 0.5f });
    addText("Mute in background", ROW_SIZE, UISkin::TEXT, { LEFT, MUTE_Y }, { 0.f, 0.5f });
    addText("DISPLAY", HEADER_SIZE, UISkin::TITLE, { LEFT, 600.f }, { 0.f, 0.5f });
    addText("CONTROLS", HEADER_SIZE, UISkin::TITLE, { LEFT, 765.f }, { 0.f, 0.5f });
    addText("Pause: Esc, P or Space    Speed: F    Camera: drag and mouse wheel", 26, UISkin::TEXT,
        { LEFT, 805.f }, { 0.f, 0.5f });

    // Sliders: live, the Effects one plays a click so its level can be heard
    auto onVolume = [this](float Settings::* field)
        {
            return [this, field](float value) { (m_dataLoader->get_settings().*field) = value; apply(); };
        };
    m_masterSlider.set_change_callback(onVolume(&Settings::masterVolume));
    m_musicSlider.set_change_callback(onVolume(&Settings::musicVolume));
    m_effectsSlider.set_change_callback(onVolume(&Settings::effectsVolume));
    m_masterSlider.set_release_callback([this](float) { play_sound("ui_click"); });
    m_effectsSlider.set_release_callback([this](float) { play_sound("ui_click"); });

    m_muteButton.set_text_size(36);
    m_muteButton.set_callback([this]()
        {
            Settings& settings = m_dataLoader->get_settings();
            settings.muteInBackground = !settings.muteInBackground;
            apply();
        });

    m_fullscreenButton.set_text_size(34);
    m_windowedButton.set_text_size(34);
    auto setMode = [this](bool fullscreen)
        {
            Settings& settings = m_dataLoader->get_settings();
            if (settings.fullscreen == fullscreen) return;
            settings.fullscreen = fullscreen;
            refresh_buttons();
            if (m_setFullscreen) m_setFullscreen(fullscreen);
        };
    m_fullscreenButton.set_callback([setMode]() { setMode(true); });
    m_windowedButton.set_callback([setMode]() { setMode(false); });

    m_closeButton.set_text_size(40);
    m_closeButton.set_click_sound("ui_back");
    m_closeButton.set_callback([this]() { close(); });

    m_isVisible = false;
}

void UIOptionsMenu::open()
{
    const Settings& settings = m_dataLoader->get_settings();
    m_masterSlider.set_value(settings.masterVolume);
    m_musicSlider.set_value(settings.musicVolume);
    m_effectsSlider.set_value(settings.effectsVolume);
    refresh_buttons();
    m_isVisible = true;
}

void UIOptionsMenu::close()
{
    m_isVisible = false;
    m_dataLoader->save_settings();
}

void UIOptionsMenu::apply()
{
    if (m_audio) m_audio->apply_settings(m_dataLoader->get_settings());
    refresh_buttons();
}

void UIOptionsMenu::refresh_buttons()
{
    const Settings& settings = m_dataLoader->get_settings();
    m_muteButton.set_text(settings.muteInBackground ? "ON" : "OFF");
    using Style = UIButtonElement::Style;
    m_muteButton.set_style(settings.muteInBackground ? Style::Primary : Style::Secondary);
    m_fullscreenButton.set_style(settings.fullscreen ? Style::Primary : Style::Secondary);
    m_windowedButton.set_style(settings.fullscreen ? Style::Secondary : Style::Primary);
}

void UIOptionsMenu::update(float deltaTime)
{
    m_muteButton.update(deltaTime);
    m_fullscreenButton.update(deltaTime);
    m_windowedButton.update(deltaTime);
    m_closeButton.update(deltaTime);
}

void UIOptionsMenu::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    window.draw(m_dim);
    m_panel.render(window);
    for (const sf::Text& label : m_labels) window.draw(label);
    m_masterSlider.render(window);
    m_musicSlider.render(window);
    m_effectsSlider.render(window);
    m_muteButton.render(window);
    m_fullscreenButton.render(window);
    m_windowedButton.render(window);
    m_closeButton.render(window);
}

void UIOptionsMenu::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyPressed->code == sf::Keyboard::Key::Escape)
        {
            play_sound("ui_back");
            close();
            return;
        }
    }

    m_masterSlider.handle_event(event, window);
    m_musicSlider.handle_event(event, window);
    m_effectsSlider.handle_event(event, window);
    m_muteButton.handle_event(event, window);
    m_fullscreenButton.handle_event(event, window);
    m_windowedButton.handle_event(event, window);
    m_closeButton.handle_event(event, window);
}

sf::FloatRect UIOptionsMenu::get_bounds() const
{
    if (!m_isVisible) return sf::FloatRect{ { 0.f, 0.f }, { 0.f, 0.f } };
    return sf::FloatRect{ m_position, m_size };
}

bool UIOptionsMenu::is_blocking_input() const
{
    return m_isVisible;
}

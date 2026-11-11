#pragma once

#include "UIPanel.h"
#include "UIButtonElement.h"
#include "UISlider.h"

#include <functional>
#include <memory>

class DataLoader;
class AudioManager;

/*
 * UIOptionsMenu
 * -------------
 * The Options window, opened from the title screen, the Fish Tank and the pause menu:
 *   - Audio: Master / Music / Effects sliders (applied live), "Mute in background"
 *   - Display: fullscreen or a resizable window
 *   - Controls: the keys
 * The choices go to DataLoader's Settings and are saved in settings.json when the window closes.
 * While open it takes every input event (is_blocking_input): add it after the other elements.
 */
class UIOptionsMenu : public UIElement
{
public:
    // setFullscreen: switches the window mode (the Game does it after the events are processed)
    UIOptionsMenu(std::shared_ptr<DataLoader> dataLoader, std::shared_ptr<AudioManager> audio,
        std::function<void(bool)> setFullscreen);

    void open();
    void close();   // Saves the settings

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

    // The whole screen while open, nothing while closed
    sf::FloatRect get_bounds() const override;
    bool is_blocking_input() const override;

private:
    // Applies the settings to the audio and shows them on the toggle buttons
    void apply();
    void refresh_buttons();

    std::shared_ptr<DataLoader> m_dataLoader;
    std::shared_ptr<AudioManager> m_audio;
    std::function<void(bool)> m_setFullscreen;

    sf::RectangleShape m_dim;
    UIPanel m_panel;
    std::vector<sf::Text> m_labels;     // Title, section headers, row names, controls

    UISlider m_masterSlider;
    UISlider m_musicSlider;
    UISlider m_effectsSlider;
    UIButtonElement m_muteButton;       // ON / OFF
    UIButtonElement m_fullscreenButton;
    UIButtonElement m_windowedButton;
    UIButtonElement m_closeButton;
};

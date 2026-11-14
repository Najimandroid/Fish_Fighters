#pragma once

#include "UIElement.h"

#include <string>

/*
 * UIBattleIntro
 * -------------
 * The battle start, for as long as the Battle Stations! jingle plays: deep water rises over the screen with a wavy
 * edge (UISkin::draw_water_band, the menus' wave), "GET READY FOR BATTLE!" pops in over the stage's name on the
 * plain water, and the water sweeps up and away when the jingle ends, uncovering the battle; then the widget hides.
 * Nothing to click: the battle is already running under it.
 *
 * Example usage:
 *   add_ui_element(std::make_shared<UIBattleIntro>("Clam Leader", audio->get_music_length(jingle)));   // Over the HUD
 */
class UIBattleIntro : public UIElement
{
public:
    // duration: the jingle's length (seconds)
    UIBattleIntro(const std::string& stageName, float duration);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override { return {}; }

private:
    sf::Text m_title;
    sf::Text m_stageName;
    float m_time = 0.f;
    float m_duration = 2.4f;
};

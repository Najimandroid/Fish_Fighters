#pragma once

#include "UIElement.h"

#include <string>

/*
 * UIBattleIntro
 * -------------
 * The battle start, for as long as the Battle Stations! jingle plays (UIManager::start_battle adds one over the screen
 * the player is on, loads the stage once it's covered, and the battle screen gets one starting at that same moment):
 * deep water rises over the screen with a wavy
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
    // duration: the jingle's length (seconds); startTime: how far into it this starts (the battle's own intro carries
    // on from the moment the jingle is at once the stage has loaded under the water)
    UIBattleIntro(const std::string& stageName, float duration, float startTime = 0.f);

    static constexpr float COVER_SECONDS = 0.35f;   // The water rising over the screen

    // A frame with the screen fully covered was drawn: the stage can load now (the player sees the water and the title)
    bool has_covered_screen() const { return m_hasCovered; }

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override { return {}; }

private:
    sf::Text m_title;
    sf::Text m_stageName;
    float m_time = 0.f;
    float m_duration = 2.4f;
    bool m_hasCovered = false;
};

#pragma once

#include "UIElement.h"

#include <string>
#include <vector>

/*
 * UIBattleIntro
 * -------------
 * The battle start, for as long as the Battle Stations! jingle plays: a wall of bubbles (ui/bubble.png) taller than
 * the screen falls through it (covering it all mid-way, gone at the bottom when the jingle ends) while
 * "GET READY FOR BATTLE!" pops in over the stage's name; then the widget hides.
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
    struct Bubble
    {
        float x = 0.f;
        float y = 0.f;
        float size = 80.f;
        float speed = 1000.f;    // px/s, downward
        float sway = 0.f;        // Phase of its side to side wobble
    };

    std::vector<Bubble> m_bubbles;
    sf::Text m_title;
    sf::Text m_stageName;
    float m_time = 0.f;
    float m_duration = 2.4f;
};

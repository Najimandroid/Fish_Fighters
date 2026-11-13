#pragma once

#include "UIButtonElement.h"
#include "UIRoundButton.h"
#include "../../data/BattleResult.h"

#include <functional>
#include <memory>
#include <string>

class DataLoader;

/*
 * UIResultsPanel
 * --------------
 * The end of a battle, over the frozen battlefield: the screen dims, a dark panel slides up with a
 * big banner ("VICTORY!" / "DEFEAT..."), then
 *   - victory: the shells counting up (a tick per step), "FIRST CLEAR!" and the new fish if any
 *   - defeat: how much of the enemy base was left (a bar) and a tip
 * and one row of buttons centered under it: the main action big and yellow, a second one, and a round
 * button back to the menus. Enter is the main action, R retries, Escape is the round button.
 * It takes every input event (is_blocking_input).
 */
class UIResultsPanel : public UIElement
{
public:
    struct Actions
    {
        std::string primaryLabel;               // "NEXT STAGE" / "RETRY" / "MAP"
        std::function<void()> primary;
        std::string secondaryLabel;             // "RETRY" / "UPGRADE"
        std::function<void()> secondary;
        std::function<void()> back;             // The round button (Fish Tank / map)
        std::function<void()> retry;            // The R key
    };

    UIResultsPanel(const BattleResult& result, const std::string& stageName, std::shared_ptr<DataLoader> dataLoader, Actions actions);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

    sf::FloatRect get_bounds() const override;
    bool is_blocking_input() const override { return true; }

private:
    void render_victory(sf::RenderWindow& window, const sf::FloatRect& panel, std::uint8_t alpha);
    void render_defeat(sf::RenderWindow& window, const sf::FloatRect& panel, std::uint8_t alpha);
    bool are_buttons_ready() const;

    BattleResult m_result;
    sf::FloatRect m_panel;              // Its final place: as tall as its content
    std::string m_stageName;
    std::string m_tip;
    Actions m_actions;
    float m_time = 0.f;

    // Victory: the count-up and the new fish
    int m_shownShells = 0;
    float m_nextTick = 0.f;
    bool m_isCountDone = false;
    bool m_isNewFishShown = false;
    sf::Texture m_newFishTexture;
    bool m_isClamRoomShown = false;   // "CLAM ROOM OPEN!" (the stage unlocked it), after the new fish
    float clam_room_time() const;

    UIButtonElement m_primaryButton;
    UIButtonElement m_secondaryButton;
    UIRoundButton m_backButton;
};

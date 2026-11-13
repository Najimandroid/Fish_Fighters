#pragma once

#include "UIElement.h"
#include "UIButtonElement.h"

#include <memory>
#include <random>
#include <vector>

class DataLoader;
class UIPlayerShellsInfo;
class UIFlyingIcons;
class UIConfirmDialog;

/*
 * UIClamStorage
 * -------------
 * Where the fish out of the clam wait (PlayerData::clamStorage). They swim around a tank on the left, each one a
 * fish of its own; on the right, a panel about the ones picked.
 *   - Click a fish to pick it; click others of the same kind to add (or remove) them; "Select all" picks every
 *     one of that kind. Clicking another kind (or the water) starts again.
 *   - USE: the first fish of a kind not in the tank yet unlocks it; each other one is +1 extra level, up to +10.
 *     Past that they can only be sold (the panel says so, USE is greyed).
 *   - SELL for shells (5 x the fish's base upgrade cost each) or for one Fish Scale each (10 Fish Scales craft a
 *     Shiny Scale: the dialog says so before selling). Both ask first (UIConfirmDialog). The shells / Fish Scales
 *     fly from the fish into their pill (UIFlyingIcons).
 * Using fish raises the Shoal Power: its milestones' Shiny Scales are announced (UIToast).
 */
class UIClamStorage : public UIElement
{
public:
    explicit UIClamStorage(std::shared_ptr<DataLoader> dataLoader);

    // The pills the sales fly into, the flying icons, the "are you sure?" dialog (all owned by the screen)
    void set_counters(std::shared_ptr<UIPlayerShellsInfo> shells, std::shared_ptr<UIPlayerShellsInfo> fishScales);
    void set_flying_icons(std::shared_ptr<UIFlyingIcons> flying);
    void set_confirm_dialog(std::shared_ptr<UIConfirmDialog> dialog);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override { return { { 0.f, 0.f }, { 1920.f, 1080.f } }; }

private:
    struct Floater
    {
        int uid = -1;
        sf::Vector2f position;
        sf::Vector2f velocity;
        float turnIn = 0.f;        // Seconds before it picks a new way
        float phase = 0.f;         // Its bob and its frames, not in step with the others
        bool isSelected = false;
    };
    struct Bubble { sf::Vector2f position; float speed = 40.f; float radius = 4.f; bool isBurst = false; };   // Burst: a fish used (gone at the top)

    void sync_floaters();                      // As many floaters as stored fish
    Floater* floater_at(sf::Vector2f point);
    void click_fish(Floater& floater);
    void set_select_all(bool selectAll);
    void clear_selection();
    int selected_count() const;
    void use_selected();
    void ask_sell(bool forFishScales);
    void sell_selected(bool forFishScales);
    int shells_each(int uid) const;
    float random(float from, float to);

    void render_floater(sf::RenderWindow& window, const Floater& floater, float scale) const;
    void render_panel(sf::RenderWindow& window);

    std::shared_ptr<DataLoader> m_dataLoader;
    std::weak_ptr<UIPlayerShellsInfo> m_shellsPill;
    std::weak_ptr<UIPlayerShellsInfo> m_fishScalesPill;
    std::weak_ptr<UIFlyingIcons> m_flying;
    std::weak_ptr<UIConfirmDialog> m_dialog;

    UIButtonElement m_use;
    UIButtonElement m_sellShells;
    UIButtonElement m_sellScales;
    sf::Text m_text;

    std::vector<Floater> m_floaters;
    std::vector<Bubble> m_bubbles;
    int m_selectedUid = -1;           // The kind picked (-1: none)
    bool m_selectAll = false;
    float m_time = 0.f;
    std::mt19937 m_random{ std::random_device{}() };
};

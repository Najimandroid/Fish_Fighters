#pragma once

#include "UIEquipIcon.h"
#include "UIEquipSlot.h"
#include "UIButtonElement.h"
#include "UIRoundButton.h"
#include "UIPanel.h"
#include "UISwipe.h"

#include "tweeny.h"

#include <memory>
#include <array>
#include <vector>
#include <string>
#include <unordered_map>

class DataLoader;

/*
 * UIEquipSlider
 * ----------------
 * A horizontal slider UI that lets the player browse through their owned units
 * and equip them into a grid of 10 slots (2x5 layout), like The Battle Cats' Equip screen.
 *
 * Features:
 * - Displays available fish units as cards (UIEquipIcon), the selected one big in the middle.
 * - Allows dragging units into the lineup slots (drawn like the deploy slots in battle),
 *   and a unit of the lineup onto another slot to swap them.
 * - Animates slider movement when switching between icons (keys, arrows or a click on a side card).
 * - Works with DataLoader to fetch unit textures and player data.
 */

class UIEquipSlider : public UIElement
{
public:

    // Constructor
    // dataLoader: shared pointer to DataLoader for unit/player data
    UIEquipSlider(std::shared_ptr<DataLoader> dataLoader);

    // Initialize icons for all available equips
    void init_icons();

    // Moves the slider focus to a specific index (animated)
    // force: glide back to it even if it's the current one (after a drag)
    void move_to_index(int newIndex, bool force = false);
    void follow_drag(float dragX);       // The cards follow a finger sliding them
    int snap_index(float dragX) const;   // Where they settle when let go

    // Refresh all equipped slots after an unequip action
    void refresh_slots_after_unequip();

    // Adds the selected card's fish to the first empty slot of the lineup (a click on it, or Enter)
    void add_selected_to_lineup();

    // Standard UI loop methods
    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

private:

    struct EquipSlot
    {
        sf::FloatRect bounds;               // Bounds of the slot
        std::shared_ptr<UIElement> ghost;   // Equipped unit placeholder (if any)
    };

    struct DragInfo
    {
        int uid = -1;                       // Unit ID
        int cost = 0;                       // Deployment cost
        int level = 1;                       // Unit level
        const sf::Texture* texture = nullptr; // Icon texture
        sf::Vector2f iconSize{ 0.f, 0.f };  // Size of icon being dragged
    };

    // The unit following the mouse while it's dragged: a big lineup slot that shrinks onto the slot under it
    struct TempGhost
    {
        int uid = -1;
        int cost = 0;
        int level = 0;

        sf::Texture texture;
        sf::RectangleShape shape;           // Position and size (scaled by currentScale)

        bool active = false;
        float currentScale = 1.0f;
        tweeny::tween<float> scaleTween = tweeny::from(1.f).to(1.f).during(1);

        // Constructor
        TempGhost(const sf::Texture& placeholder);

        // Non-copyable, movable
        TempGhost(const TempGhost&) = delete;
        TempGhost& operator=(const TempGhost&) = delete;
        TempGhost(TempGhost&&) noexcept = default;
        TempGhost& operator=(TempGhost&&) noexcept = default;
    };

    // Drag helpers
    // begin_drag: starts dragging the pressed unit (the selected card, or the lineup slot m_dragFromSlot)
    void begin_drag(const sf::Vector2f& startPos);
    DragInfo make_drag_info(int uid, const sf::Vector2f& iconSize);
    void start_drag(const DragInfo& dragInfo, const sf::Vector2f& startPos);
    void update_drag_position(const sf::Vector2f& pos);
    void end_drag(const sf::Vector2f& pos);

    // Lineup panel, its title and the empty slots
    void render_lineup(sf::RenderWindow& window);

    // The dragged unit
    void render_ghost(sf::RenderWindow& window);

private:

    int m_currentIndex = 0; // Currently selected icon index
    std::vector<int> m_availableEquips; // List of owned unit UIDs
    std::vector<std::shared_ptr<UIEquipIcon>> m_equipIcons; // Icon elements for slider
    std::unordered_map<int, sf::Texture> m_unitTextures;    // Cached textures

    std::array<EquipSlot, 10> m_slots; // 10 equip slots (2x5 grid)

    std::shared_ptr<DataLoader> m_dataLoader; // Data loader reference

    // Shared resources
    sf::Texture m_placeholderTexture;

    std::unique_ptr<TempGhost> m_tempGhost; // Currently dragged ghost (if active)

    // Look
    UIRoundButton m_previousButton;   // Arrows on both sides of the cards
    UIRoundButton m_nextButton;
    UIPanel m_lineupPanel;
    sf::Text m_lineupTitle;           // "LINEUP"
    sf::Text m_hint;                  // How to equip / remove
    sf::Text m_ghostCost;             // Deploy cost on the dragged unit

    // Drag state tracking
    UISwipe m_swipe;                  // Swiping the cards sideways
    bool m_isFollowing = false;       // The cards follow the finger
    bool m_mouseDown = false;
    bool m_dragging = false;
    int m_dragFromSlot = -1;          // Lineup slot being dragged to another slot (-1: a card is dragged)
    sf::Vector2f m_mouseDownPos{ 0.f, 0.f };
    sf::Clock m_holdClock;
    static constexpr int HOLD_THRESHOLD_MS = 50; // Long press threshold

    // Card layout: bottom-aligned on CARDS_BOTTOM, the selected card centered
    const sf::Vector2f baseIconSize = { 250.0f, 250.0f };
    const float CARDS_BOTTOM = 520.f;
    const float ICON_SPACING = 380.0f;
    const float CENTER_SCALE = 1.3f;
    const float SIDE_SCALE = 1.0f;
    const float CENTER_OFFSET_X = (CENTER_SCALE - SIDE_SCALE) * 250.f / 2.f; // Keeps the bigger card centered
    const float TWEEN_DURATION = 0.3f; // Animation duration in seconds
};

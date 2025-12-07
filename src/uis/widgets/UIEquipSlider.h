#pragma once

#include "UIEquipIcon.h"
#include "UIEquipSlot.h"
#include "UIButtonElement.h"

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
 * and equip them into a grid of 10 slots (2x5 layout).
 *
 * Features:
 * - Displays available fish units as selectable icons.
 * - Allows dragging units into equip slots.
 * - Animates slider movement when switching between icons.
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
    void move_to_index(int newIndex);

    // Refresh all equipped slots after an unequip action
    void refresh_slots_after_unequip();

    // Standard UI loop methods
    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

private:

    /*
     * EquipSlot
     * Represents one of the 10 equip slots.
     * Each slot can hold a ghost element of a unit (visual placeholder).
     */
    struct EquipSlot
    {
        sf::FloatRect bounds;               // Bounds of the slot
        sf::RectangleShape background;      // Slot background rectangle
        std::shared_ptr<UIElement> ghost;   // Equipped unit placeholder (if any)
    };

    /*
     * DragInfo
     * Temporary data used while dragging a unit icon.
     */
    struct DragInfo
    {
        int uid = -1;                       // Unit ID
        int cost = 0;                       // Deployment cost
        int level = 1;                       // Unit level
        const sf::Texture* texture = nullptr; // Icon texture
        sf::Vector2f iconSize{ 0.f, 0.f };  // Size of icon being dragged
    };

    /*
     * TempGhost
     * Temporary visual representation of a unit while being dragged.
     * Includes scaling tween animation when hovering slots.
     */
    struct TempGhost
    {
        int uid = -1;
        int cost = 0;
        int level = 0;

        sf::Texture texture;
        sf::Sprite sprite;

        sf::RectangleShape shape;

        sf::Text costText;
        sf::Text levelText;

        bool active = false;

        float currentScale = 1.0f;
        tweeny::tween<float> scaleTween = tweeny::from(1.f).to(1.f).during(1);

        const float INFO_BASE_TEXT_SIZE = 45.f; // Font size of the cost and level texts of the ghost
        const float INFO_BASE_TEXT_OFFSET = 8.f; // Offset (in pixels) applied when positioning both the texts

        // Constructor: sets up cost text styling
        TempGhost(const sf::Font& font, const sf::Texture& placeholder);
        // Non-copyable, movable
        TempGhost(const TempGhost&) = delete;
        TempGhost& operator=(const TempGhost&) = delete;
        TempGhost(TempGhost&&) noexcept = default;
        TempGhost& operator=(TempGhost&&) noexcept = default;
    };

    // Drag helpers
    void start_drag(const DragInfo& dragInfo, const sf::Vector2f& startPos);
    void update_drag_position(const sf::Vector2f& pos);
    void end_drag(const sf::Vector2f& pos);

private:

    int m_currentIndex = 0; // Currently selected icon index

    std::vector<int> m_availableEquips; // List of owned unit UIDs
    std::vector<std::shared_ptr<UIEquipIcon>> m_equipIcons; // Icon elements for slider
    std::unordered_map<int, sf::Texture> m_unitTextures;    // Cached textures

    std::array<EquipSlot, 10> m_slots; // 10 equip slots (2x5 grid)

    std::shared_ptr<DataLoader> m_dataLoader; // Data loader reference

    // Shared resources
    sf::Font m_sharedFont;
    sf::Texture m_placeholderTexture;

    std::unique_ptr<TempGhost> m_tempGhost; // Currently dragged ghost (if active)

    // Drag state tracking
    bool m_mouseDown = false;
    bool m_dragging = false;
    sf::Vector2f m_mouseDownPos{ 0.f, 0.f };
    sf::Clock m_holdClock;
    static constexpr int HOLD_THRESHOLD_MS = 50; // Long press threshold

    // Layout constants
    const sf::Vector2f baseIconSize = { 350.0f, 350.0f };
    const float ICON_SPACING = 475.0f;
    const float CENTER_OFFSET_X = 40.f;
    const float CENTER_SCALE = 1.3f;
    const float SIDE_SCALE = 1.0f;
    const float TWEEN_DURATION = 0.3f; // Animation duration in seconds
};

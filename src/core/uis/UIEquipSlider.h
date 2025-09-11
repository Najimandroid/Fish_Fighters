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

class UIEquipSlider : public UIElement
{
public:
    UIEquipSlider(std::shared_ptr<DataLoader> dataLoader);

    void init_icons();
    void move_to_index(int newIndex);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

private:
    struct EquipSlot
    {
        sf::FloatRect bounds;
        sf::RectangleShape background;
        std::shared_ptr<UIElement> ghost = nullptr;
    };

    struct DragInfo
    {
        int uid = -1;
        int cost = 0;
        const sf::Texture* texture = nullptr;
        sf::Vector2f iconSize{ 0.f, 0.f };
    };

    struct TempGhost
    {
        int uid = -1;
        int cost = 0;

        sf::Texture texture;
        sf::Sprite sprite;
        sf::RectangleShape shape;
        sf::Text costText;
        bool active = false;

        float currentScale = 1.0f;
        tweeny::tween<float> scaleTween = tweeny::from(1.f).to(1.f).during(1);

        TempGhost(const sf::Font& font, const sf::Texture& placeholder)
            : texture(placeholder)
            , sprite(texture)
            , costText(font)
            , active(false)
            , currentScale(1.f)
            , scaleTween(tweeny::from(1.f).to(1.f).during(1))
        {
            costText.setFillColor(sf::Color::Yellow);
            costText.setOutlineColor(sf::Color::Black);
            costText.setOutlineThickness(2.f);

            // center origins will be set when sizes are known
        }

        TempGhost(const TempGhost&) = delete;
        TempGhost& operator=(const TempGhost&) = delete;
        TempGhost(TempGhost&&) noexcept = default;
        TempGhost& operator=(TempGhost&&) noexcept = default;
    };

    // drag helpers
    void start_drag(const DragInfo& dragInfo, const sf::Vector2f& startPos);
    void update_drag_position(const sf::Vector2f& pos);
    void end_drag(const sf::Vector2f& pos);

private:
    int m_currentIndex = 0;
    std::vector<int> m_availableEquips;
    std::vector<std::shared_ptr<UIEquipIcon>> m_equipIcons;
    std::unordered_map<int, sf::Texture> m_unitTextures;

    std::array<EquipSlot, 10> m_slots;

    std::shared_ptr<DataLoader> m_dataLoader;

    // resources (loaded early)
    sf::Font m_sharedFont;
    sf::Texture m_placeholderTexture;

    // Temp ghost is deferred-constructed after loading font+placeholder
    std::unique_ptr<TempGhost> m_tempGhost;

    // drag state
    bool m_mouseDown = false;
    bool m_dragging = false;
    sf::Vector2f m_mouseDownPos{ 0.f, 0.f };
    sf::Clock m_holdClock;
    static constexpr int HOLD_THRESHOLD_MS = 50; 

    // layout constants
    const sf::Vector2f baseIconSize = { 350.0f, 350.0f };
    const float ICON_SPACING = 475.0f;
    const float CENTER_OFFSET_X = 40.f;
    const float CENTER_SCALE = 1.3f;
    const float SIDE_SCALE = 1.0f;
    const float TWEEN_DURATION = 0.3f; // seconds (we use frame counts when calling during())
};

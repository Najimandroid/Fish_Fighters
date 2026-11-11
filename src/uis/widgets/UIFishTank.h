#pragma once

#include "UIElement.h"

#include <memory>
#include <random>
#include <vector>

class DataLoader;
struct EntityData;

/*
 * UIFishTank
 * ----------
 * The Fish Tank's porthole as a live tank (the hub): the player's lineup swims in the glass, each fish in its
 * current form with its battle walking frames. They swim across, turn around at the glass, bob and change
 * depth; now and then one lets out bubbles. Clicking a fish makes it attack (its attack frames) with a "blub".
 * The porthole's ring is drawn over them (porthole_ring.png), so they stay behind the glass.
 *
 * The glass and the ring are the hub background's (assets/images/textures/backgrounds/menus/fish_tank.png).
 *
 * Example usage:
 *   add_ui_element(background);
 *   add_ui_element(std::make_shared<UIFishTank>(dataLoader));   // Over the background, under the buttons
 */
class UIFishTank : public UIElement
{
public:
    explicit UIFishTank(std::shared_ptr<DataLoader> dataLoader);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override;   // The glass

private:
    struct Swimmer
    {
        std::shared_ptr<EntityData> data;
        const sf::Texture* texture = nullptr;
        sf::Vector2i frameSize;
        float scale = 1.f;
        sf::Vector2f position;          // Center of the frame
        float speed = 50.f;             // px/s; > 0 swims right (the fish face left: flipped)
        float targetY = 0.f;            // The depth it drifts to
        float retargetIn = 0.f;         // Seconds before it picks another depth (and maybe turns around)
        float bobPhase = 0.f;
        float frameTime = 0.f;
        int frame = 0;
        bool isAttacking = false;
        float bubblesIn = 0.f;          // Seconds before it lets out a few bubbles
    };

    struct Bubble
    {
        sf::Vector2f position;
        float radius = 4.f;
        float age = 0.f;
        float wobble = 0.f;
    };

    // Half the glass's width at the height y, minus margin (0 when y is outside)
    float glass_half_width(float y, float margin) const;
    sf::Sprite sprite_of(const Swimmer& swimmer) const;
    void release_bubbles(const Swimmer& swimmer, int count);
    float random(float from, float to);

    std::shared_ptr<DataLoader> m_dataLoader;
    std::vector<Swimmer> m_swimmers;
    std::vector<Bubble> m_bubbles;
    std::mt19937 m_random{ std::random_device{}() };
    float m_time = 0.f;
};

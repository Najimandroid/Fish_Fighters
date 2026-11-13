#pragma once

#include "UIElement.h"
#include "UIButtonElement.h"

#include <memory>
#include <random>
#include <string>
#include <vector>

class DataLoader;

/*
 * UIClamRoom
 * ----------
 * The Clam Room's giant clam (the gacha): feed it Shiny Scales, get fish. Every opening is a little show:
 *   1. the scales fly from the counter into the clam
 *   2. the clam hops and shakes harder and harder (rigid: a hard shell never squashes)
 *   3. it pops open: a flash, a burst of bubbles, light rays
 *   4. the fish rise out in bubbles: NEW! for a fish joining, LEVEL n for one gaining a level (MAX: the scale
 *      comes back)
 *   5. a click and it closes again
 * OPEN costs 1 Shiny Scale, OPEN x10 ten (ten fish in a 5 x 2 grid). A click skips ahead. The rewards are given
 * (and saved) the moment the scales are spent: the show only presents them.
 *
 * The fish come from every fish of the game, equally likely (PlayerData::receive_fish gives them).
 * Textures: ui/clam_closed.png, ui/clam_opened.png (same 300 x 300 canvas), ui/bubble.png, ui/icon_shiny_scale.png,
 * and each fish's sheet (its first frame, cropped to the fish: the icons are close-ups cut at the edges).
 */
class UIClamRoom : public UIElement
{
public:
    explicit UIClamRoom(std::shared_ptr<DataLoader> dataLoader);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override { return { { 0.f, 0.f }, { 1920.f, 1080.f } }; }

    // While a clam is opening (the back button waits)
    bool is_busy() const { return m_phase != Phase::IDLE; }

private:
    enum class Phase { IDLE, FEEDING, CHARGING, OPENING, REVEALING, REVEALED, CLOSING };

    struct Reveal
    {
        std::string name;
        std::string texturePath;  // The fish's sheet...
        sf::IntRect frame;        // ...its first frame, cropped to the fish
        std::string tag;          // NEW!, LEVEL n or MAX
        sf::Color tagColor;
        sf::Vector2f target;      // Where it floats once out
        float scale = 1.f;
        float appearAt = 0.f;     // Seconds into REVEALING
        bool hasSounded = false;
    };

    struct Bubble { sf::Vector2f position; sf::Vector2f velocity; float radius = 6.f; float age = 0.f; float life = 1.5f; };

    void open(int count);
    void go_to(Phase phase);
    void skip();
    void spawn_bubbles(int count, float speed);
    sf::Vector2f mouth() const;
    float random(float from, float to);

    void render_clam(sf::RenderWindow& window);
    void render_reveals(sf::RenderWindow& window);
    void render_scales(sf::RenderWindow& window);

    std::shared_ptr<DataLoader> m_dataLoader;
    UIButtonElement m_openOne;
    UIButtonElement m_openTen;
    sf::Text m_text;

    Phase m_phase = Phase::IDLE;
    float m_phaseTime = 0.f;
    float m_time = 0.f;
    int m_count = 1;              // Scales fed this time
    int m_hop = -1;               // Last hop that landed (sounds once per landing)
    bool m_isShaking = false;
    std::vector<Reveal> m_reveals;
    std::vector<Bubble> m_bubbles;
    std::mt19937 m_random{ std::random_device{}() };
};

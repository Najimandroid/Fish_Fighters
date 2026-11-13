#pragma once

#include "UIElement.h"
#include "UIButtonElement.h"

#include <memory>
#include <random>
#include <string>
#include <vector>

class DataLoader;
class UIPlayerShellsInfo;

/*
 * UIClamRoom
 * ----------
 * The Clam Room's giant clam (the gacha): feed it Shiny Scales, get fish. Every opening is a little show:
 *   1. the scales fly from the counter into the clam
 *   2. the clam hops and shakes harder and harder (rigid: a hard shell never squashes)
 *   3. it pops open: a flash, a burst of bubbles, light rays
 *   4. the fish rise out in bubbles, swimming: NEW! for a fish joining, +1 EXTRA LEVEL for one already owned (like
 *      The Battle Cats' "+1": stronger, the price and the form don't change), +n SHELLS when its extra levels
 *      are maxed (PlayerData::MAX_EXTRA_LEVELS): its shells fly out of the bubble into the shells pill, which
 *      counts them as they land
 *   5. a click and it closes again
 * A click on the idle clam makes it heave and drop back with a thud (clam_tap), once a second at most: it's heavy.
 * OPEN costs 1 Shiny Scale, OPEN x10 ten (ten fish in a 5 x 2 grid). A click skips ahead. The rewards are given
 * (and saved) the moment the scales are spent: the show only presents them.
 *
 * The fish come from every fish of the game, equally likely (PlayerData::receive_fish gives them).
 * The clam has a shadow on the ground like the fish and enemies (effects/shadow.png), smaller and fainter when it jumps.
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

    // The title bar's pills: the Shiny Scales fly out of theirs, a spare fish's shells fly into theirs
    void set_counters(std::shared_ptr<UIPlayerShellsInfo> shells, std::shared_ptr<UIPlayerShellsInfo> scales);

    // What flies over the title bar (drawn by UIClamRoomOverlay, after the title bar and the pills)
    void render_overlay(sf::RenderWindow& window);

private:
    enum class Phase { IDLE, FEEDING, CHARGING, OPENING, REVEALING, REVEALED, CLOSING };

    struct Reveal
    {
        std::string name;
        std::string texturePath;  // The fish's sheet...
        sf::IntRect box;          // ...the box its swimming frames use (in a frame)
        int frameWidth = 0;
        int frameCount = 1;       // Swimming frames
        float frameOffset = 0.f;  // Seconds: the fish don't swim in step
        std::string tag;          // NEW!, +1 EXTRA LEVEL or +n SHELLS (extra levels maxed)
        sf::Color tagColor;
        std::string level;        // Under the tag: "LEVEL 7"...
        std::string extra;        // ...and its extra levels after it: "+3" (purple)
        sf::Vector2f target;      // Where it floats once out
        float scale = 1.f;
        float appearAt = 0.f;     // Seconds into REVEALING
        bool hasSounded = false;
    };

    struct Bubble { sf::Vector2f position; sf::Vector2f velocity; float radius = 6.f; float age = 0.f; float life = 1.5f; };

    // A shell of a spare fish (its extra levels maxed) flying out of its bubble into the shells pill
    struct FlyingShell
    {
        size_t reveal = 0;        // Its fish (m_reveals)
        int index = 0;            // Which one of its shells: they leave one after the other
        int amount = 0;           // Shells it brings to the counter
        bool isLast = false;      // The last one of its fish (the collect sound)
        sf::Vector2f bend;        // Its arc: the curve's control point, from (the pill's x, the bubble's y)
        float spin = 0.f;         // Degrees over the flight
        bool hasLanded = false;
    };
    float shell_start(const FlyingShell& shell) const;   // In m_showTime
    void land_shells(bool all);                           // The ones that arrived (all: every one, now)
    sf::Vector2f shells_target() const;
    sf::Vector2f scales_source() const;

    void open(int count);
    void go_to(Phase phase);
    void skip();
    void spawn_bubbles(int count, float speed);
    sf::Vector2f mouth() const;
    sf::FloatRect clam_bounds() const;   // The closed clam on screen (clicks make it rattle)
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
    float m_tapTime = 0.f;        // A click on the idle clam: seconds of its heave, thud and rattle left
    float m_tapCooldown = 0.f;    // Seconds before it can be poked again
    bool m_hasTapLanded = true;   // Its thud played
    std::vector<Reveal> m_reveals;
    std::vector<Bubble> m_bubbles;
    std::vector<FlyingShell> m_flyingShells;
    float m_showTime = 0.f;       // Seconds since the fish started coming out (REVEALING, then REVEALED)
    std::weak_ptr<UIPlayerShellsInfo> m_shellsInfo;
    std::weak_ptr<UIPlayerShellsInfo> m_scalesInfo;
    std::mt19937 m_random{ std::random_device{}() };
};

/*
 * UIClamRoomOverlay
 * -----------------
 * Draws what the Clam Room throws over the title bar (the Shiny Scales leaving their pill, a spare fish's shells
 * going into theirs), so they reach the counters: added after the title bar and the pills.
 */
class UIClamRoomOverlay : public UIElement
{
public:
    explicit UIClamRoomOverlay(std::weak_ptr<UIClamRoom> room) : m_room(std::move(room)) {}
    void update(float) override {}
    void render(sf::RenderWindow& window) override { if (auto room = m_room.lock()) room->render_overlay(window); }

private:
    std::weak_ptr<UIClamRoom> m_room;
};

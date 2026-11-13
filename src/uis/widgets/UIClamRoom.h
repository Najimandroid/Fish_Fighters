#pragma once

#include "UIElement.h"
#include "UIButtonElement.h"

#include <functional>
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
 *   1. the clam opens halfway (ui/clam_semi_opened.png), the scales fly from their pill into its mouth, it hops up
 *      with its mouth full (the gulp) and snaps shut as it lands
 *   2. the clam hops and shakes harder and harder (rigid: a hard shell never squashes)
 *   3. it pops open: a flash, a burst of bubbles, light rays
 *   4. the fish rise out in bubbles, swimming: NEW! for a kind the player doesn't have yet, TO STORAGE for the
 *      others. Every fish goes into the Clam Storage (UIClamStorage), where it's used (unlocks the fish, or +1
 *      extra level) or sold (shells, or a Fish Scale)
 *   5. a click and it closes again
 * The STORAGE button (bottom right, with how many fish wait there) opens the Clam Storage (set_on_storage).
 * A click on the idle clam pokes it where it was clicked: pressed down on that side, it rocks on its round bottom (the
 * other side heaves up; in the middle it hops), drops back with a thud (clam_tap, heard on that side) and rocks back,
 * once a second at most: it's heavy.
 * OPEN costs 1 Shiny Scale, OPEN x10 ten (ten fish in a 5 x 2 grid). A click skips ahead. The fish are stored
 * (and saved) the moment the scales are spent: the show only presents them.
 *
 * The fish come from every fish of the game, equally likely (PlayerData::receive_fish gives them).
 * The clam has a shadow on the ground like the fish and enemies (effects/shadow.png), smaller and fainter when it jumps.
 * Textures: ui/clam_closed.png, ui/clam_semi_opened.png, ui/clam_opened.png (same 300 x 300 canvas), ui/bubble.png, ui/icon_shiny_scale.png,
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

    // The Shiny Scales pill: the scales fly out of it into the clam
    void set_scales_counter(std::shared_ptr<UIPlayerShellsInfo> scales);

    // The STORAGE button's action (opens the Clam Storage)
    void set_on_storage(std::function<void()> onStorage);

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
        std::string tag;          // NEW! or TO STORAGE
        sf::Color tagColor;
        sf::Vector2f target;      // Where it floats once out
        float scale = 1.f;
        float appearAt = 0.f;     // Seconds into REVEALING
        bool hasSounded = false;
    };

    struct Bubble { sf::Vector2f position; sf::Vector2f velocity; float radius = 6.f; float age = 0.f; float life = 1.5f; };

    sf::Vector2f scales_source() const;

    void open(int count);
    void go_to(Phase phase);
    void skip();
    void spawn_bubbles(int count, float speed);                      // From the open clam's mouth
    void spawn_bubbles(int count, float speed, sf::Vector2f from);
    sf::Vector2f semi_mouth() const;                                  // The half-open clam's mouth (the scales go in)
    float scales_landed_time() const;                                 // In FEEDING: the last scale is in
    sf::Vector2f mouth() const;
    sf::FloatRect clam_bounds() const;   // The closed clam on screen (clicks make it rattle)
    float random(float from, float to);

    void render_clam(sf::RenderWindow& window);
    void render_reveals(sf::RenderWindow& window);
    void render_scales(sf::RenderWindow& window);

    std::shared_ptr<DataLoader> m_dataLoader;
    UIButtonElement m_openOne;
    UIButtonElement m_openTen;
    UIButtonElement m_storage;    // To the Clam Storage
    sf::Text m_text;

    Phase m_phase = Phase::IDLE;
    float m_phaseTime = 0.f;
    float m_time = 0.f;
    int m_count = 1;              // Scales fed this time
    int m_hop = -1;               // Last hop that landed (sounds once per landing)
    bool m_isShaking = false;
    bool m_hasGulped = false;     // FEEDING: it shut its mouth on the scales
    float m_tapTime = 0.f;        // A click on the idle clam: seconds of its heave, thud and rattle left
    float m_tapCooldown = 0.f;    // Seconds before it can be poked again
    float m_tapSide = 0.f;        // Where it was poked: -1 its left edge, 0 its middle, 1 its right edge
    bool m_hasTapLanded = true;   // Its thud played
    std::vector<Reveal> m_reveals;
    std::vector<Bubble> m_bubbles;
    std::weak_ptr<UIPlayerShellsInfo> m_scalesInfo;
    std::mt19937 m_random{ std::random_device{}() };
};

/*
 * UIClamRoomOverlay
 * -----------------
 * Draws what the Clam Room throws over the title bar (the Shiny Scales leaving their pill), so they come out of the
 * counter: added after the title bar and the pills.
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

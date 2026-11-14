#include "UIClamRoom.h"
#include "UIPlayerShellsInfo.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"
#include "../../data/BattlefieldLayout.h"
#include "../../data/PlayerData.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace
{
    const float PI = 3.14159265f;

    // The clam: its 300 x 300 canvas drawn 2.2 times bigger, standing on its base
    const float CLAM_SCALE = 2.2f;
    const sf::Vector2f CLAM_ORIGIN{ 150.f, 290.f };       // The base of the drawing in its canvas
    const sf::Vector2f CLAM_BASE{ 960.f, 860.f };         // ...on screen
    const sf::Vector2f MOUTH_IN_CANVAS{ 150.f, 190.f };   // Where the fish come out (the open clam's inside)
    const sf::Vector2f SCALE_COUNTER{ 1430.f, 55.f };     // Where the scales fly from (the Shiny Scales pill)

    // Timing (seconds)
    const float SCALE_FLIGHT = 0.4f, SCALE_GAP = 0.06f;

    // Eating the scales: the clam opens halfway (clam_semi_opened.png), the scales fly into its mouth (a little bump
    // each), it hops up with its mouth full (the gulp), snaps shut as it lands, then a breath before the hops
    const sf::Vector2f SEMI_MOUTH_IN_CANVAS{ 150.f, 210.f };   // The half-open clam's mouth
    const float FEED_OPEN = 0.15f, GULP_HOP = 0.34f, GULP_HEIGHT = 32.f, FEED_SETTLE = 0.3f;
    const float HOP = 0.36f, HOP_HEIGHT = 46.f;
    const float SHAKE_ONE = 0.45f, SHAKE_TEN = 0.8f;
    const float OPEN_POP = 0.45f;
    const float REVEAL_GAP = 0.18f;
    const float CLOSE = 1.f;           // Closing: the bubbles go back into its mouth (CLOSE_INHALE), it shuts (CLOSE_SHUT)
    const float CLOSE_INHALE = 0.58f;
    const float CLOSE_SHUT = 0.64f;


    // The clam's shadow on the ground (like the fish and enemies): a bit wider than the closed shell, centered on its
    // bottom edge so half of it shows under it, drawn twice (the room is dark: one would hardly show)
    const float CLAM_SHADOW_WIDTH = 620.f;
    const float CLAM_SHADOW_HEIGHT = 175.f;
    const float CLAM_SHADOW_Y = 856.f;

    // A poke tips the clam on the side clicked: that side is pressed down, the other side lifts; its bottom is round,
    // so it rocks almost in place (it rises by TAP_ROCK of a sharp edge's pivot); the middle makes it hop straight up
    const float CLAM_HALF_WIDTH = 130.f * 2.2f;   // The closed shell, from its middle
    const float TAP_TIP = 8.f;                    // Degrees at the edge
    const float TAP_ROCK = 0.3f;

    const sf::Color GLOW{ 190, 130, 255 };
    const sf::Color RAYS{ 255, 240, 200 };
    const sf::Color CORAL{ 255, 128, 104 };
    const sf::Color TEAL{ 72, 217, 230 };

    float ease_out_back(float t) { const float c = 1.70158f; t -= 1.f; return 1.f + (c + 1.f) * t * t * t + c * t * t; }

    // A click on the closed clam (it's heavy): it heaves up on one side, drops back with a thud (clam_tap, at the
    // landing), rattles to a stop; then it can't be poked again before TAP_COOLDOWN
    const float TAP_SHAKE = 0.6f, TAP_LIFT = 0.13f, TAP_DROP = 0.07f, TAP_HEIGHT = 14.f;
    const float TAP_COOLDOWN = 1.1f;
    const float WALK_FRAME_SECONDS = 0.12f;  // The fish swim in their bubbles (like in the Fish Tank)

    // STORAGE: the Clam Storage's chest as a button, its name small under it (bottom right)
    const sf::FloatRect STORAGE_ICON{ { 1700.f, 880.f }, { 140.f, 120.f } };

    // Into the Clam Storage: the clam pops open (TO_STORAGE_OPEN), the view dives into its mouth from DIVE_START
    // (ZOOM_MAX times closer), going dark pink from INSIDE_FADE. Out: the view zooms out of the open clam (from
    // dark pink) until OUT_ZOOM, the clam snaps shut at FROM_STORAGE_SHUT
    const float TO_STORAGE_OPEN = 0.18f, DIVE_START = 0.3f, INSIDE_FADE = 0.75f, TO_STORAGE = 1.25f;
    const float OUT_ZOOM = 0.75f, FROM_STORAGE_SHUT = 0.8f, FROM_STORAGE = 1.1f;
    const float ZOOM_MAX = 6.f;
    const sf::Color INSIDE_PINK{ 70, 22, 52 };   // The Clam Storage's dark pink
}

UIClamRoom::UIClamRoom(std::shared_ptr<DataLoader> dataLoader) :
    m_dataLoader(std::move(dataLoader)),
    m_openOne({ 360.f, 126.f }, { 580.f, 918.f }, "OPEN", UIOfferButton::Kind::CURRENCY, "icon_shiny_scale", 1),
    m_openTen({ 360.f, 126.f }, { 980.f, 918.f }, "OPEN x10", UIOfferButton::Kind::CURRENCY, "icon_shiny_scale", 10),
    m_text(UISkin::heavy_font())
{
    m_openOne.set_text_size(46);
    m_openTen.set_text_size(40);
    m_openTen.set_style(UIButtonElement::Style::Secondary);
    m_openOne.set_click_sound("");   // The scales make the sound
    m_openTen.set_click_sound("");
    m_openOne.set_hotkey(sf::Keyboard::Key::Space);
    m_openOne.set_attention(true);
    m_openOne.set_callback([this]() { open(1); });
    m_openTen.set_callback([this]() { open(10); });
}

float UIClamRoom::random(float from, float to)
{
    return std::uniform_real_distribution<float>(from, std::max(from, to))(m_random);
}

sf::Vector2f UIClamRoom::mouth() const
{
    return CLAM_BASE + (MOUTH_IN_CANVAS - CLAM_ORIGIN) * CLAM_SCALE;
}

sf::Vector2f UIClamRoom::semi_mouth() const
{
    return CLAM_BASE + (SEMI_MOUTH_IN_CANVAS - CLAM_ORIGIN) * CLAM_SCALE;
}

float UIClamRoom::scales_landed_time() const
{
    return FEED_OPEN + SCALE_FLIGHT + SCALE_GAP * static_cast<float>(m_count - 1);
}

sf::FloatRect UIClamRoom::clam_bounds() const
{
    // The closed shell fills the canvas from y 150 to its base, x 20 to 280
    const sf::Vector2f topLeft = CLAM_BASE + (sf::Vector2f{ 20.f, 150.f } - CLAM_ORIGIN) * CLAM_SCALE;
    return { topLeft, sf::Vector2f{ 260.f, 140.f } * CLAM_SCALE };
}

void UIClamRoom::go_to(Phase phase)
{
    m_phase = phase;
    m_phaseTime = 0.f;
    m_hasCue = false;
}

void UIClamRoom::enter_from_storage()
{
    go_to(Phase::FROM_STORAGE);
}

float UIClamRoom::zoom() const
{
    if (m_phase == Phase::TO_STORAGE)
    {
        const float u = std::clamp((m_phaseTime - DIVE_START) / (TO_STORAGE - DIVE_START), 0.f, 1.f);
        return 1.f + (ZOOM_MAX - 1.f) * std::pow(u, 2.2f);   // Slow, then faster and faster
    }
    if (m_phase == Phase::FROM_STORAGE)
    {
        const float u = std::clamp(m_phaseTime / OUT_ZOOM, 0.f, 1.f);
        return ZOOM_MAX - (ZOOM_MAX - 1.f) * (1.f - std::pow(1.f - u, 2.2f));   // Fast, then slowing down
    }
    return 1.f;
}

float UIClamRoom::inside_fade() const
{
    if (m_phase == Phase::TO_STORAGE) return std::clamp((m_phaseTime - INSIDE_FADE) / (TO_STORAGE - INSIDE_FADE), 0.f, 1.f);
    if (m_phase == Phase::FROM_STORAGE) return 1.f - std::clamp(m_phaseTime / 0.35f, 0.f, 1.f);
    return 0.f;
}

void UIClamRoom::set_scales_counter(std::shared_ptr<UIPlayerShellsInfo> scales)
{
    m_scalesInfo = scales;
}

void UIClamRoom::set_on_storage(std::function<void()> onStorage)
{
    m_onStorage = std::move(onStorage);
}

sf::Vector2f UIClamRoom::scales_source() const
{
    auto pill = m_scalesInfo.lock();
    return pill ? pill->get_icon_center() : SCALE_COUNTER;
}

/*
 * Spends the scales, gives the fish (saved at once), and starts the show
 */
void UIClamRoom::open(int count)
{
    auto player = m_dataLoader->get_player_data().lock();
    if (m_phase != Phase::IDLE || !player) return;
    if (!player->spend_shiny_scales(count))
    {
        play_sound("ui_error");
        return;
    }

    std::set<int> uids;
    for (const auto& [key, data] : m_dataLoader->get_units_database()) uids.insert(key.first);
    const std::vector<int> pool(uids.begin(), uids.end());
    if (pool.empty()) return;

    m_reveals.clear();
    for (int i = 0; i < count; ++i)
    {
        const int uid = pool[std::uniform_int_distribution<size_t>(0, pool.size() - 1)(m_random)];
        const PlayerData::FishReward reward = player->receive_fish(uid);   // Into the Clam Storage

        Reveal reveal;
        auto data = m_dataLoader->get_unit_data(uid, player->get_unit_form(uid));
        reveal.name = data && !data->name.empty() ? data->name : "?";
        if (data && !data->texture.empty())
        {
            const UISkin::SwimFrames& swim = UISkin::swim_frames(data->texture, data->frameCount, data->knockbackFrameIndex);
            reveal.texturePath = data->texture;
            reveal.box = swim.box;
            reveal.frameWidth = swim.frameWidth;
            reveal.frameCount = swim.count;
            reveal.frameOffset = static_cast<float>(i) * 0.37f;   // Not all in step
        }

        // NEW! for a kind the player doesn't have yet; every fish goes into the Clam Storage
        reveal.tag = reward.isNew ? "NEW!" : "TO STORAGE";
        reveal.tagColor = reward.isNew ? CORAL : UISkin::TITLE;
        if (count == 1)
        {
            reveal.target = { 960.f, 470.f };
            reveal.scale = 1.6f;
            reveal.appearAt = 0.1f;
        }
        else
        {
            reveal.target = { 480.f + static_cast<float>(i % 5) * 240.f, i < 5 ? 290.f : 615.f };
            reveal.scale = 0.85f;
            reveal.appearAt = 0.1f + static_cast<float>(i) * REVEAL_GAP;
        }
        m_reveals.push_back(reveal);
    }
    m_dataLoader->save_progress();

    m_count = count;
    m_hop = -1;
    m_isShaking = false;
    m_hasGulped = false;
    go_to(Phase::FEEDING);
}

/*
 * A click (or Space / Enter) once every fish is out: the clam closes
 */
void UIClamRoom::close_clam()
{
    // The show itself can't be skipped (opening a clam is rare: it's to be enjoyed); once every fish is out, a
    // click closes the clam
    if (m_phase == Phase::REVEALED)
    {
        go_to(Phase::CLOSING);
        m_hasShut = false;
        play_sound("fish_blub", 0.75f);   // The bubbles being drawn back in (the hop sound when it shuts)
    }
}

void UIClamRoom::spawn_bubbles(int count, float speed)
{
    spawn_bubbles(count, speed, mouth());
}

void UIClamRoom::spawn_bubbles(int count, float speed, sf::Vector2f from)
{
    for (int i = 0; i < count; ++i)
    {
        Bubble bubble;
        bubble.position = from + sf::Vector2f{ random(-120.f, 120.f), random(-30.f, 30.f) };
        const float angle = random(-PI * 0.85f, -PI * 0.15f);   // Upward, fanning out
        const float velocity = random(180.f, 520.f) * speed;
        bubble.velocity = { std::cos(angle) * velocity, std::sin(angle) * velocity };
        bubble.radius = random(5.f, 16.f);
        bubble.life = random(0.9f, 1.8f);
        m_bubbles.push_back(bubble);
    }
}

void UIClamRoom::update(float deltaTime)
{
    m_time += deltaTime;
    m_phaseTime += deltaTime;
    if (m_phase == Phase::IDLE)
    {
        // A poke: the thud when it lands back
        m_tapCooldown = std::max(0.f, m_tapCooldown - deltaTime);
        m_tapTime = std::max(0.f, m_tapTime - deltaTime);
        if (!m_hasTapLanded && TAP_SHAKE - m_tapTime >= TAP_LIFT + TAP_DROP)
        {
            m_hasTapLanded = true;
            play_sound("clam_tap", 1.f, m_tapSide * 0.4f);   // Heard on the side it was poked
        }
    }
    else
    {
        m_tapTime = 0.f;
        m_tapCooldown = 0.f;
        m_hasTapLanded = true;
    }

    auto player = m_dataLoader->get_player_data().lock();
    const int scales = player ? player->shinyScales : 0;
    m_openOne.set_enabled(m_phase == Phase::IDLE && scales >= 1);
    m_openTen.set_enabled(m_phase == Phase::IDLE && scales >= 10);
    m_openOne.update(deltaTime);
    m_openTen.update(deltaTime);

    switch (m_phase)
    {
    case Phase::FEEDING:
    {
        // Half open, it eats: a scale lands in its mouth every SCALE_GAP (its sound, rising for a x10)
        const float flying = m_phaseTime - FEED_OPEN;
        const int landed = static_cast<int>((flying - SCALE_FLIGHT) / SCALE_GAP) + 1;
        if (flying >= SCALE_FLIGHT && m_hop < m_count - 1 && landed - 1 > m_hop)
        {
            m_hop = std::min(landed, m_count) - 1;
            play_sound("scale_use", 1.f + 0.04f * static_cast<float>(m_hop));
        }

        // Then the gulp: a hop with its mouth full, shut as it lands (a thud, a "blub", bubbles escaping)
        const float shut = scales_landed_time() + GULP_HOP;
        if (!m_hasGulped && m_phaseTime >= shut)
        {
            m_hasGulped = true;
            play_sound("clam_hop", 0.95f);
            play_sound("fish_blub", 0.8f);
            spawn_bubbles(8, 0.4f, semi_mouth());
        }
        if (m_phaseTime >= shut + FEED_SETTLE) { m_hop = -1; go_to(Phase::CHARGING); }
        break;
    }
    case Phase::CHARGING:
    {
        const int hops = m_count == 1 ? 3 : 4;
        const int hop = static_cast<int>(m_phaseTime / HOP);
        if (hop > m_hop && hop >= 1 && hop <= hops) { m_hop = hop; play_sound("clam_hop", 0.9f + 0.08f * static_cast<float>(hop)); }
        if (!m_isShaking && m_phaseTime >= hops * HOP) { m_isShaking = true; play_sound("clam_shake"); }
        if (m_phaseTime >= hops * HOP + (m_count == 1 ? SHAKE_ONE : SHAKE_TEN))
        {
            go_to(Phase::OPENING);
            play_sound("clam_open");
            spawn_bubbles(m_count == 1 ? 36 : 60, 1.f);
        }
        break;
    }
    case Phase::OPENING:
        if (m_phaseTime >= OPEN_POP * 0.5f) go_to(Phase::REVEALING);
        break;
    case Phase::REVEALING:
    {
        for (size_t i = 0; i < m_reveals.size(); ++i)
        {
            Reveal& reveal = m_reveals[i];
            if (!reveal.hasSounded && m_phaseTime >= reveal.appearAt)
            {
                reveal.hasSounded = true;
                play_sound("fish_reveal", 1.f + 0.05f * static_cast<float>(i));
                spawn_bubbles(m_count == 1 ? 14 : 5, 0.6f);
            }
        }
        if (m_phaseTime >= m_reveals.back().appearAt + 0.5f) go_to(Phase::REVEALED);
        break;
    }
    case Phase::CLOSING:
        if (!m_hasShut && m_phaseTime >= CLOSE_SHUT) { m_hasShut = true; play_sound("clam_hop", 1.1f); }
        if (m_phaseTime >= CLOSE) { m_reveals.clear(); go_to(Phase::IDLE); }
        break;
    case Phase::TO_STORAGE:
        // It pops open ("blub", a few bubbles), the view dives in; once inside, the Clam Storage
        if (!m_hasCue && m_phaseTime >= TO_STORAGE_OPEN)
        {
            m_hasCue = true;
            play_sound("fish_blub", 0.8f);
            spawn_bubbles(12, 0.5f);
        }
        if (m_phaseTime >= TO_STORAGE && m_onStorage)
        {
            auto onStorage = std::move(m_onStorage);   // Once (it changes the screen)
            m_onStorage = nullptr;
            onStorage();
        }
        break;
    case Phase::FROM_STORAGE:
        // Out of the clam: it snaps shut behind us
        if (!m_hasCue && m_phaseTime >= FROM_STORAGE_SHUT)
        {
            m_hasCue = true;
            play_sound("clam_hop");
        }
        if (m_phaseTime >= FROM_STORAGE) go_to(Phase::IDLE);
        break;
    default:
        break;
    }

    for (Bubble& bubble : m_bubbles)
    {
        bubble.age += deltaTime;
        bubble.velocity *= std::pow(0.35f, deltaTime);   // Water slows them
        bubble.velocity.y -= 120.f * deltaTime;          // They rise
        bubble.position += bubble.velocity * deltaTime;
    }
    std::erase_if(m_bubbles, [](const Bubble& bubble) { return bubble.age > bubble.life; });
}

void UIClamRoom::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    // Diving into the clam (or out of it): the room seen through a view closing in on its mouth
    const sf::View baseView = window.getView();
    const float dive = zoom();
    if (dive > 1.001f)
    {
        sf::View zoomed = baseView;
        const float closeness = (dive - 1.f) / (ZOOM_MAX - 1.f);
        const sf::Vector2f inside = mouth() + sf::Vector2f{ 0.f, -30.f };
        zoomed.setCenter(baseView.getCenter() + (inside - baseView.getCenter()) * closeness);
        zoomed.setSize(baseView.getSize() / dive);
        window.setView(zoomed);
    }

    // (The room's colors and its beam of light are UIClamBackdrop's, under this)

    // A soft magic glow behind the clam, breathing
    const float breath = 0.85f + 0.15f * std::sin(m_time * 1.6f);
    const bool isOpen = m_phase == Phase::OPENING || m_phase == Phase::REVEALING || m_phase == Phase::REVEALED || is_transition();
    for (int ring = 8; ring >= 1; --ring)
    {
        const float radius = 60.f * static_cast<float>(ring) * breath + (isOpen ? 40.f : 0.f);
        sf::CircleShape glow(radius);
        glow.setOrigin({ radius, radius });
        glow.setPosition(mouth() + sf::Vector2f{ 0.f, -40.f });
        glow.setFillColor(sf::Color(GLOW.r, GLOW.g, GLOW.b, static_cast<std::uint8_t>(isOpen ? 22 : 12)));
        window.draw(glow);
    }

    // Light rays turning slowly behind the fish once it's open
    if (m_phase == Phase::REVEALING || m_phase == Phase::REVEALED)
    {
        const float alpha = std::min(1.f, m_phaseTime * 3.f) * (m_phase == Phase::REVEALED ? 1.f : std::min(1.f, m_phaseTime * 2.f));
        const sf::Vector2f center = m_count == 1 ? sf::Vector2f{ 960.f, 470.f } : mouth();
        for (int i = 0; i < 12; ++i)
        {
            const float angle = m_time * 0.4f + static_cast<float>(i) * 2.f * PI / 12.f;
            sf::ConvexShape ray(3);
            ray.setPoint(0, center);
            ray.setPoint(1, center + sf::Vector2f{ std::cos(angle - 0.09f), std::sin(angle - 0.09f) } * 900.f);
            ray.setPoint(2, center + sf::Vector2f{ std::cos(angle + 0.09f), std::sin(angle + 0.09f) } * 900.f);
            ray.setFillColor(sf::Color(RAYS.r, RAYS.g, RAYS.b, static_cast<std::uint8_t>(34 * alpha)));
            window.draw(ray);
        }
    }

    render_clam(window);

    // Bubbles out of the clam
    const sf::Texture& bubbleTexture = UISkin::texture("bubble");
    sf::Sprite bubbleSprite(bubbleTexture);
    const float textureSize = static_cast<float>(std::max(1u, bubbleTexture.getSize().x));
    for (const Bubble& bubble : m_bubbles)
    {
        const float fade = std::clamp(1.f - bubble.age / bubble.life, 0.f, 1.f);
        bubbleSprite.setScale({ bubble.radius * 2.f / textureSize, bubble.radius * 2.f / textureSize });
        bubbleSprite.setOrigin({ textureSize / 2.f, textureSize / 2.f });
        bubbleSprite.setPosition(bubble.position);
        bubbleSprite.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(255 * fade)));
        window.draw(bubbleSprite);
    }

    // The flash of the opening
    if (m_phase == Phase::OPENING || (m_phase == Phase::REVEALING && m_phaseTime < 0.3f))
    {
        const float t = m_phase == Phase::OPENING ? m_phaseTime : m_phaseTime + OPEN_POP * 0.5f;
        UISkin::fill_screen(window, sf::Color(255, 250, 240, static_cast<std::uint8_t>(std::max(0.f, 180.f * (1.f - t / 0.5f)))));
    }

    render_reveals(window);

    // Inside the clam: dark pink (the Clam Storage), then nothing else of the room
    window.setView(baseView);
    if (is_transition())
    {
        UISkin::fill_screen(window, sf::Color(INSIDE_PINK.r, INSIDE_PINK.g, INSIDE_PINK.b, static_cast<std::uint8_t>(255.f * inside_fade())));
        return;
    }

    // The offers, each with its price under it
    m_openOne.render(window);
    m_openTen.render(window);

    // STORAGE: the Clam Storage's chest, with how many fish wait there
    auto player = m_dataLoader->get_player_data().lock();
    int stored = 0;
    if (player) for (const auto& [uid, count] : player->clamStorage) stored += count;
    render_storage_icon(window, stored);

    // What to do
    std::string hint;
    if (m_phase == Phase::IDLE)
        hint = player && player->shinyScales > 0 ? "Feed the giant clam a Shiny Scale..."
            : "No Shiny Scales: raise your School Rank, or craft one from Fish Scales (+)";
    else if (m_phase == Phase::REVEALED) hint = "Tap to continue: your fish wait in the Clam Storage";
    if (!hint.empty())
    {
        UISkin::style_text(m_text, 36, UISkin::TITLE);
        m_text.setString(hint);
        UISkin::set_text_anchor(m_text, { 0.5f, 0.5f });
        m_text.setPosition({ 960.f, 170.f });
        window.draw(m_text);
    }
}

/*
 * The clam, standing on its base: hops, shakes, the jolt of the opening, the close (rigid: it's a hard shell)
 */
void UIClamRoom::render_clam(sf::RenderWindow& window)
{
    const bool isOpen = m_phase == Phase::OPENING || m_phase == Phase::REVEALING || m_phase == Phase::REVEALED
        || (m_phase == Phase::CLOSING && m_phaseTime < CLOSE_SHUT)   // Open until every bubble is back inside
        || (m_phase == Phase::TO_STORAGE && m_phaseTime >= TO_STORAGE_OPEN)
        || (m_phase == Phase::FROM_STORAGE && m_phaseTime < FROM_STORAGE_SHUT);
    const bool isEating = m_phase == Phase::FEEDING && m_phaseTime < scales_landed_time() + GULP_HOP;   // Half open
    const sf::Texture& texture = UISkin::texture(isOpen ? "clam_opened" : isEating ? "clam_semi_opened" : "clam_closed");

    // A hard shell: it never squashes, it jumps, tilts, rattles and jolts as one rigid piece
    float jump = 0.f, angle = 0.f, shiftX = 0.f;
    float pivot = 0.f;   // The part of the jump that only keeps a tipped clam's low edge on the ground (its shadow stays)
    switch (m_phase)
    {
    case Phase::IDLE:
    {
        angle = 1.2f * std::sin(m_time * 1.3f);   // Rocking slightly where it sits
        if (m_tapTime > 0.f)
        {
            // Poked where it was clicked (m_tapSide: -1 its left edge, 0 its middle, 1 its right edge): pressed down on
            // that side, it rocks on its round bottom and the other side heaves up (slow, it's heavy); in the middle
            // it hops straight up. It drops back fast, then rocks back the other way and rattles to a stop
            const float t = TAP_SHAKE - m_tapTime, landing = TAP_LIFT + TAP_DROP;
            float lift = 0.f;   // 0 on the ground, 1 at the top
            if (t < TAP_LIFT) lift = std::sin(t / TAP_LIFT * PI * 0.5f);
            else if (t < landing) { const float u = (t - TAP_LIFT) / TAP_DROP; lift = 1.f - u * u; }   // Falling: faster and faster

            if (t < landing)
            {
                const float tip = m_tapSide * TAP_TIP * lift;
                angle += tip;
                pivot = TAP_ROCK * CLAM_HALF_WIDTH * std::sin(std::abs(tip) * PI / 180.f);   // Its round bottom stays on the ground
                jump = pivot + TAP_HEIGHT * (1.f - std::abs(m_tapSide)) * lift;
            }
            else
            {
                const float s = t - landing, decay = std::max(0.f, 1.f - s / (TAP_SHAKE - landing));
                const float side = std::abs(m_tapSide) > 0.15f ? m_tapSide : 1.f;
                angle -= side * std::sin(s * 30.f) * 2.5f * decay * decay;
                shiftX = std::sin(s * 26.f) * 3.f * decay * decay;
            }
        }
        break;
    }
    case Phase::FEEDING:
    {
        const float landedAt = scales_landed_time(), shut = landedAt + GULP_HOP;
        if (m_phaseTime < FEED_OPEN)
            jump = 8.f * std::sin(m_phaseTime / FEED_OPEN * PI);   // A little jolt as it opens
        else if (m_phaseTime < landedAt)
        {
            // Each scale landing in its mouth bumps it a little
            for (int i = 0; i < m_count; ++i)
            {
                const float age = m_phaseTime - (FEED_OPEN + SCALE_FLIGHT + SCALE_GAP * static_cast<float>(i));
                if (age >= 0.f && age < 0.12f) jump = std::max(jump, 5.f * (1.f - age / 0.12f));
            }
        }
        else if (m_phaseTime < shut)
        {
            // The gulp: a hop with its mouth full
            const float u = (m_phaseTime - landedAt) / GULP_HOP;
            jump = GULP_HEIGHT * std::sin(u * PI);
            angle = 2.5f * std::sin(u * 2.f * PI);
        }
        else
        {
            // Shut: it settles
            const float s = m_phaseTime - shut, decay = std::max(0.f, 1.f - s / FEED_SETTLE);
            angle = std::sin(s * 32.f) * 2.f * decay * decay;
        }
        break;
    }
    case Phase::CHARGING:
    {
        const int hops = m_count == 1 ? 3 : 4;
        if (m_phaseTime < hops * HOP)
        {
            // A hop: up and down in an arc, tilting one way then the other, landing flat
            const int hop = static_cast<int>(m_phaseTime / HOP);
            const float u = std::fmod(m_phaseTime, HOP) / HOP;
            jump = HOP_HEIGHT * std::sin(u * PI);
            angle = (hop % 2 ? -4.f : 4.f) * std::sin(u * PI);
        }
        else
        {
            const float shake = m_phaseTime - hops * HOP, length = m_count == 1 ? SHAKE_ONE : SHAKE_TEN;
            const float ramp = std::min(1.f, shake / length * 1.5f);
            angle = std::sin(shake * 62.f) * 5.f * ramp;
            shiftX = std::sin(shake * 47.f) * 8.f * ramp;
        }
        break;
    }
    case Phase::OPENING:
    case Phase::REVEALING:
    {
        // Bursting open: a jolt up that settles, a wobble
        const float t = m_phase == Phase::OPENING ? m_phaseTime : m_phaseTime + OPEN_POP * 0.5f;
        if (t < OPEN_POP)
        {
            const float decay = 1.f - t / OPEN_POP;
            jump = 18.f * decay * std::abs(std::cos(t / OPEN_POP * PI * 1.5f));
            angle = 3.f * decay * std::sin(t / OPEN_POP * PI * 3.f);
        }
        break;
    }
    case Phase::CLOSING:
        if (m_phaseTime > CLOSE_SHUT)   // A little hop as it snaps shut
            jump = 10.f * std::sin(std::min(1.f, (m_phaseTime - CLOSE_SHUT) / (CLOSE - CLOSE_SHUT)) * PI);
        break;
    case Phase::TO_STORAGE:
        // A pop as it opens for us
        if (m_phaseTime < DIVE_START) jump = 12.f * std::sin(m_phaseTime / DIVE_START * PI);
        break;
    case Phase::FROM_STORAGE:
        // A little hop as it snaps shut behind us
        if (m_phaseTime >= FROM_STORAGE_SHUT)
            jump = 9.f * std::sin(std::min(1.f, (m_phaseTime - FROM_STORAGE_SHUT) / (FROM_STORAGE - FROM_STORAGE_SHUT)) * PI);
        break;
    default:
        break;
    }

    // Its shadow on the ground, like the fish and enemies have: it stays down, smaller and fainter while the clam
    // is in the air (not while it's only tipped on an edge)
    const sf::Texture& shadowTexture = UISkin::texture_file(SHADOW_TEXTURE);
    const sf::Vector2f shadowSize(shadowTexture.getSize());
    if (shadowSize.x > 0.f && shadowSize.y > 0.f)
    {
        const float air = std::clamp((jump - pivot) / 100.f, 0.f, 0.6f);
        const float shrink = 1.f - 0.5f * air;
        sf::Sprite shadow(shadowTexture);
        shadow.setOrigin(shadowSize / 2.f);
        shadow.setScale({ CLAM_SHADOW_WIDTH * shrink / shadowSize.x, CLAM_SHADOW_HEIGHT * shrink / shadowSize.y });
        shadow.setPosition({ CLAM_BASE.x + shiftX, CLAM_SHADOW_Y });
        shadow.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(255.f * (1.f - air))));
        window.draw(shadow);
        window.draw(shadow);
    }

    sf::Sprite clam(texture);
    clam.setOrigin(CLAM_ORIGIN);
    clam.setScale({ CLAM_SCALE, CLAM_SCALE });
    clam.setRotation(sf::degrees(angle));
    clam.setPosition(CLAM_BASE + sf::Vector2f{ shiftX, -jump });
    window.draw(clam);
}

/*
 * STORAGE: the Clam Storage's chest (bigger under the mouse, greyed while the clam is busy), "STORAGE" small under
 * it, a coral badge with how many fish wait there
 */
void UIClamRoom::render_storage_icon(sf::RenderWindow& window, int stored)
{
    const bool isUsable = m_phase == Phase::IDLE;
    const float grow = isUsable && m_isStorageHovered ? 1.08f : 1.f;
    const sf::Vector2f center{ STORAGE_ICON.position.x + STORAGE_ICON.size.x / 2.f, STORAGE_ICON.position.y + 50.f };
    const float size = 112.f * grow;
    UISkin::draw_fitted(window, UISkin::texture("icon_clam_storage"), { center - sf::Vector2f{ size, size } / 2.f, { size, size } },
        isUsable ? sf::Color::White : sf::Color(255, 255, 255, 120));

    UISkin::style_text(m_text, 26, UISkin::TITLE);
    m_text.setString("STORAGE");
    UISkin::set_text_anchor(m_text, { 0.5f, 0.5f });
    m_text.setPosition({ center.x, STORAGE_ICON.position.y + STORAGE_ICON.size.y - 6.f });
    window.draw(m_text);

    if (stored <= 0) return;
    const float radius = 24.f;
    sf::CircleShape badge(radius);
    badge.setOrigin({ radius, radius });
    badge.setPosition(center + sf::Vector2f{ 48.f, -44.f });
    badge.setFillColor(CORAL);
    badge.setOutlineThickness(4.f);
    badge.setOutlineColor(UISkin::OUTLINE);
    window.draw(badge);
    UISkin::style_text(m_text, 26, sf::Color::White);
    m_text.setString(std::to_string(stored));
    UISkin::set_text_anchor(m_text, { 0.5f, 0.5f });
    m_text.setPosition(badge.getPosition());
    window.draw(m_text);
}

/*
 * The fish out of the clam: each rises from the clam's mouth inside a bubble to its place, overshooting a little,
 * with its name and NEW! / LEVEL n / MAX
 */
void UIClamRoom::render_reveals(sf::RenderWindow& window)
{
    if (m_phase != Phase::REVEALING && m_phase != Phase::REVEALED && m_phase != Phase::CLOSING) return;

    const sf::Texture& bubbleTexture = UISkin::texture("bubble");
    const float bubbleSize = static_cast<float>(std::max(1u, bubbleTexture.getSize().x));
    const bool isClosing = m_phase == Phase::CLOSING;
    const float revealTime = m_phase == Phase::REVEALING ? m_phaseTime : 1000.f;
    // Closing: one after the other, the bubbles drift back into the clam's mouth, shrinking as they go in
    const float stagger = m_reveals.size() > 1 ? std::min(0.04f, 0.22f / static_cast<float>(m_reveals.size())) : 0.f;
    const float inhale = CLOSE_INHALE - stagger * static_cast<float>(m_reveals.size());

    for (size_t index = 0; index < m_reveals.size(); ++index)
    {
        const Reveal& reveal = m_reveals[index];
        const float t = std::clamp((revealTime - reveal.appearAt) / 0.45f, 0.f, 1.f);
        if (t <= 0.f) continue;
        const float back = isClosing ? std::clamp((m_phaseTime - stagger * static_cast<float>(index)) / inhale, 0.f, 1.f) : 0.f;
        const float pull = back * back * (3.f - 2.f * back);   // Eases in and out
        if (back >= 1.f) continue;   // Inside
        const float closing = 1.f - 0.8f * pull;                 // Its size on the way in
        const float grow = ease_out_back(t) * closing;
        const float bob = std::sin(m_time * 2.4f + reveal.target.x * 0.01f) * 6.f * (1.f - pull);
        const sf::Vector2f outside = mouth() + (reveal.target - mouth()) * std::min(1.f, t * 1.2f) + sf::Vector2f{ 0.f, bob };
        const sf::Vector2f position = outside + (mouth() - outside) * pull;

        const float size = 200.f * reveal.scale * grow;
        sf::Sprite bubble(bubbleTexture);
        bubble.setOrigin({ bubbleSize / 2.f, bubbleSize / 2.f });
        bubble.setScale({ size * 1.15f / bubbleSize, size * 1.15f / bubbleSize });
        bubble.setPosition(position);
        const float fade = back > 0.8f ? 1.f - (back - 0.8f) / 0.2f : 1.f;   // Gone as it enters
        bubble.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(170 * fade)));
        window.draw(bubble);

        // The whole fish inside, swimming (its walking frames, as drawn on its sheet)
        if (!reveal.texturePath.empty() && reveal.box.size.x > 0 && reveal.box.size.y > 0)
        {
            const int frame = static_cast<int>((m_time + reveal.frameOffset) / WALK_FRAME_SECONDS) % std::max(1, reveal.frameCount);
            sf::Sprite fish(UISkin::texture_file(reveal.texturePath),
                { { frame * reveal.frameWidth + reveal.box.position.x, reveal.box.position.y }, reveal.box.size });
            const sf::Vector2f boxSize(reveal.box.size);
            const float fit = std::min(size * 0.8f / boxSize.x, size * 0.6f / boxSize.y);
            fish.setOrigin(boxSize / 2.f);
            fish.setScale({ fit, fit });
            fish.setPosition(position);
            fish.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(255 * fade)));
            window.draw(fish);
        }

        if (t < 1.f || isClosing) continue;

        // Under the bubble: the name, the tag (NEW!, +1 EXTRA LEVEL, +n SHELLS), the level with its extra levels.
        // Smaller in the x10 grid, where each fish has a 240 px column
        const bool isBig = reveal.scale > 1.f;
        const unsigned int nameSize = isBig ? 40 : 32, tagSize = isBig ? 34 : 25;
        float y = position.y + size * 0.62f;
        auto line = [&](const std::string& string, unsigned int textSize, sf::Color color, float x, sf::Vector2f anchor)
            {
                UISkin::style_text(m_text, textSize, color);
                m_text.setString(string);
                UISkin::set_text_anchor(m_text, anchor);
                m_text.setPosition({ x, y });
                window.draw(m_text);
                return m_text.getGlobalBounds();
            };
        line(reveal.name, nameSize, sf::Color::White, position.x, { 0.5f, 0.f });
        y += static_cast<float>(nameSize) * 1.1f;
        line(reveal.tag, tagSize, reveal.tagColor, position.x, { 0.5f, 0.f });
    }
}

/*
 * Over the title bar: the Shiny Scales leaving their pill, and a spare fish's shells flying out of its bubble into
 * the shells pill (each pops out, then swoops in an arc, faster and faster, spinning)
 */
void UIClamRoom::render_overlay(sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    render_scales(window);
}

/*
 * The Shiny Scales flying from the counter into the clam, spinning, in an arc
 */
void UIClamRoom::render_scales(sf::RenderWindow& window)
{
    if (m_phase != Phase::FEEDING) return;
    const sf::Texture& icon = UISkin::texture("icon_shiny_scale");
    const float iconSize = static_cast<float>(std::max(1u, icon.getSize().x));
    for (int i = 0; i < m_count; ++i)
    {
        // Once the clam is half open, into its mouth (shrinking as they go in)
        const float t = (m_phaseTime - FEED_OPEN - SCALE_GAP * static_cast<float>(i)) / SCALE_FLIGHT;
        if (t <= 0.f || t >= 1.f) continue;
        const float eased = t * t * (3.f - 2.f * t);
        const sf::Vector2f from = scales_source();
        sf::Vector2f position = from + (semi_mouth() - from) * eased;
        position.y -= std::sin(t * PI) * 160.f;   // An arc
        sf::Sprite scale(icon);
        scale.setOrigin({ iconSize / 2.f, iconSize / 2.f });
        const float size = 70.f * (1.f - 0.6f * t);
        scale.setScale({ size / iconSize, size / iconSize });
        scale.setRotation(sf::degrees(t * 540.f));
        scale.setPosition(position);
        window.draw(scale);
    }
}

void UIClamRoom::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    if (m_phase == Phase::IDLE)
    {
        m_openOne.handle_event(event, window);
        m_openTen.handle_event(event, window);

        // STORAGE (the chest): grows under the mouse; a click dives into the clam
        if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
            m_isStorageHovered = STORAGE_ICON.contains(window.mapPixelToCoords(moved->position));

        // A click on the clam: it heaves and thuds (just for fun), not more than once per TAP_COOLDOWN
        if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>(); pressed && pressed->button == sf::Mouse::Button::Left)
        {
            const sf::Vector2f mouse = window.mapPixelToCoords(pressed->position);
            if (STORAGE_ICON.contains(mouse) && m_onStorage)
            {
                play_sound("ui_click");
                m_isStorageHovered = false;
                go_to(Phase::TO_STORAGE);
                return;
            }
            if (clam_bounds().contains(mouse) && m_tapCooldown <= 0.f)
            {
                m_tapTime = TAP_SHAKE;
                m_tapCooldown = TAP_COOLDOWN;
                m_hasTapLanded = false;
                m_tapSide = std::clamp((mouse.x - CLAM_BASE.x) / CLAM_HALF_WIDTH, -1.f, 1.f);   // Which side was poked
            }
        }
        return;
    }
    // Anything else: a click (or Space / Enter) closes the clam once the fish are out
    if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>(); pressed && pressed->button == sf::Mouse::Button::Left) close_clam();
    if (const auto* key = event.getIf<sf::Event::KeyPressed>(); key && (key->code == sf::Keyboard::Key::Space || key->code == sf::Keyboard::Key::Enter)) close_clam();
}

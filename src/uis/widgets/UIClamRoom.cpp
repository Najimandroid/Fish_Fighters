#include "UIClamRoom.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"
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
    const float HOP = 0.36f, HOP_HEIGHT = 46.f;
    const float SHAKE_ONE = 0.45f, SHAKE_TEN = 0.8f;
    const float OPEN_POP = 0.45f;
    const float REVEAL_GAP = 0.18f;
    const float CLOSE = 0.35f;

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

    // A fish's swimming frames (0 .. knockback frame - 1), cropped to the box its pixels use in all of them: the
    // whole fish for its bubble, still while it swims (the icons are close-ups cut at the edges, and a sheet's
    // frame leaves room for the attack effects)
    struct SwimFrames { sf::IntRect box; int frameWidth = 0; int count = 1; };
    SwimFrames swim_frames(const sf::Texture& sheet, int frameCount, int knockbackFrameIndex)
    {
        SwimFrames frames;
        const sf::Vector2u size = sheet.getSize();
        const unsigned int frameWidth = size.x / static_cast<unsigned int>(std::max(1, frameCount));
        frames.frameWidth = static_cast<int>(frameWidth);
        frames.count = std::clamp(knockbackFrameIndex, 1, std::max(1, frameCount));
        const sf::Image image = sheet.copyToImage();
        unsigned int left = frameWidth, top = size.y, right = 0, bottom = 0;
        for (int frame = 0; frame < frames.count; ++frame)
            for (unsigned int y = 0; y < size.y; ++y)
                for (unsigned int x = 0; x < frameWidth; ++x)
                    if (image.getPixel({ static_cast<unsigned int>(frame) * frameWidth + x, y }).a > 24)
                    {
                        left = std::min(left, x); right = std::max(right, x);
                        top = std::min(top, y); bottom = std::max(bottom, y);
                    }
        if (left > right || top > bottom) frames.box = { { 0, 0 }, { static_cast<int>(frameWidth), static_cast<int>(size.y) } };
        else frames.box = { { static_cast<int>(left), static_cast<int>(top) }, { static_cast<int>(right - left + 1), static_cast<int>(bottom - top + 1) } };
        return frames;
    }
}

UIClamRoom::UIClamRoom(std::shared_ptr<DataLoader> dataLoader) :
    m_dataLoader(std::move(dataLoader)),
    m_openOne({ 360.f, 110.f }, { 580.f, 930.f }, "OPEN"),
    m_openTen({ 360.f, 110.f }, { 980.f, 930.f }, "OPEN x10"),
    m_text(UISkin::heavy_font())
{
    m_openOne.set_text_size(48);
    m_openTen.set_text_size(38);
    m_openTen.set_style(UIButtonElement::Style::Secondary);
    m_openOne.set_click_sound("");   // The scales make the sound
    m_openTen.set_click_sound("");
    m_openOne.set_hotkey(sf::Keyboard::Key::Space);
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
    std::map<std::string, SwimFrames> frames;   // A fish drawn twice is cropped once
    for (int i = 0; i < count; ++i)
    {
        const int uid = pool[std::uniform_int_distribution<size_t>(0, pool.size() - 1)(m_random)];
        auto baseData = m_dataLoader->get_unit_data(uid, 0);
        const int spareShells = baseData ? baseData->baseUpgradeCost * PlayerData::SPARE_FISH_SHELLS_FACTOR : 0;
        const PlayerData::FishReward reward = player->receive_fish(uid, spareShells);

        Reveal reveal;
        auto data = m_dataLoader->get_unit_data(uid, reward.form);
        reveal.name = data && !data->name.empty() ? data->name : "?";
        if (data && !data->texture.empty())
        {
            if (!frames.contains(data->texture))
                frames[data->texture] = swim_frames(UISkin::texture_file(data->texture), data->frameCount, data->knockbackFrameIndex);
            const SwimFrames& swim = frames[data->texture];
            reveal.texturePath = data->texture;
            reveal.box = swim.box;
            reveal.frameWidth = swim.frameWidth;
            reveal.frameCount = swim.count;
            reveal.frameOffset = static_cast<float>(i) * 0.37f;   // Not all in step
        }

        // NEW! / +1 EXTRA LEVEL / the shells of a fish whose extra levels are maxed; under it, its level now
        if (reward.isNew)
        {
            reveal.tag = "NEW!";
            reveal.tagColor = CORAL;
        }
        else if (reward.isSpare)
        {
            reveal.tag = "+" + std::to_string(reward.shells) + " SHELLS";
            reveal.tagColor = UISkin::NUMBER;
            reveal.level = "LEVEL " + std::to_string(reward.level);
            reveal.extra = "+" + std::to_string(reward.extraLevels) + " MAX";
        }
        else
        {
            reveal.tag = "+1 EXTRA LEVEL";
            reveal.tagColor = UISkin::EXTRA;
            reveal.level = "LEVEL " + std::to_string(reward.level);
            reveal.extra = "+" + std::to_string(reward.extraLevels);
        }
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
    go_to(Phase::FEEDING);
}

/*
 * A click: the hops and the shake are skipped, the fish all come out at once, or the clam closes
 */
void UIClamRoom::skip()
{
    switch (m_phase)
    {
    case Phase::FEEDING:
    case Phase::CHARGING:
        go_to(Phase::OPENING);
        play_sound("clam_open");
        spawn_bubbles(36, 1.f);
        break;
    case Phase::REVEALING:
        for (Reveal& reveal : m_reveals) reveal.appearAt = std::min(reveal.appearAt, m_phaseTime);
        break;
    case Phase::REVEALED:
        go_to(Phase::CLOSING);
        play_sound("clam_hop", 1.1f);
        break;
    default:
        break;
    }
}

void UIClamRoom::spawn_bubbles(int count, float speed)
{
    const sf::Vector2f from = mouth();
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
            play_sound("clam_tap");
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
        // A scale lands every SCALE_GAP: its sound, rising for a x10
        const int landed = static_cast<int>((m_phaseTime - SCALE_FLIGHT) / SCALE_GAP) + 1;
        if (landed - 1 > m_hop && m_phaseTime >= SCALE_FLIGHT)
        {
            m_hop = std::min(landed, m_count) - 1;
            play_sound("scale_use", 1.f + 0.04f * static_cast<float>(m_hop));
        }
        if (m_phaseTime >= SCALE_FLIGHT + SCALE_GAP * static_cast<float>(m_count - 1) + 0.15f) { m_hop = -1; go_to(Phase::CHARGING); }
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
        if (m_phaseTime >= CLOSE) { m_reveals.clear(); go_to(Phase::IDLE); }
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

    // The room is dim and purple: something magic lives here
    sf::RectangleShape veil({ 1920.f, 1080.f });
    veil.setFillColor(sf::Color(40, 18, 70, 130));
    window.draw(veil);

    // A soft magic glow behind the clam, breathing
    const float breath = 0.85f + 0.15f * std::sin(m_time * 1.6f);
    const bool isOpen = m_phase == Phase::OPENING || m_phase == Phase::REVEALING || m_phase == Phase::REVEALED;
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
    render_scales(window);

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
        sf::RectangleShape flash({ 1920.f, 1080.f });
        flash.setFillColor(sf::Color(255, 250, 240, static_cast<std::uint8_t>(std::max(0.f, 180.f * (1.f - t / 0.5f)))));
        window.draw(flash);
    }

    render_reveals(window);

    // The buttons, a Shiny Scale on each (what they cost)
    m_openOne.render(window);
    m_openTen.render(window);
    const sf::Texture& scaleIcon = UISkin::texture("icon_shiny_scale");
    for (UIButtonElement* button : { &m_openOne, &m_openTen })
    {
        const sf::FloatRect box = button->get_bounds();
        UISkin::draw_fitted(window, scaleIcon, { { box.position.x + 14.f, box.position.y + box.size.y / 2.f - 24.f }, { 48.f, 48.f } },
            button->is_enabled() ? sf::Color::White : sf::Color(255, 255, 255, 120));
    }

    // What to do
    std::string hint;
    if (m_phase == Phase::IDLE)
    {
        auto player = m_dataLoader->get_player_data().lock();
        hint = player && player->shinyScales > 0 ? "Feed the giant clam a Shiny Scale..." : "No Shiny Scales left: win battles to find more!";
    }
    else if (m_phase == Phase::REVEALED) hint = "Click to continue";
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
        || (m_phase == Phase::CLOSING && m_phaseTime < CLOSE * 0.5f);
    const sf::Texture& texture = UISkin::texture(isOpen ? "clam_opened" : "clam_closed");

    // A hard shell: it never squashes, it jumps, tilts, rattles and jolts as one rigid piece
    float jump = 0.f, angle = 0.f, shiftX = 0.f;
    switch (m_phase)
    {
    case Phase::IDLE:
    {
        angle = 1.2f * std::sin(m_time * 1.3f);   // Rocking slightly where it sits
        if (m_tapTime > 0.f)
        {
            // Poked: it heaves up on one side (slow, it's heavy), drops back fast, then rattles to a stop
            const float t = TAP_SHAKE - m_tapTime, landing = TAP_LIFT + TAP_DROP;
            if (t < TAP_LIFT)
            {
                const float u = t / TAP_LIFT;
                jump = TAP_HEIGHT * std::sin(u * PI * 0.5f);
                angle -= 5.f * std::sin(u * PI * 0.5f);
            }
            else if (t < landing)
            {
                const float u = (t - TAP_LIFT) / TAP_DROP, fall = 1.f - u * u;   // Falling: faster and faster
                jump = TAP_HEIGHT * fall;
                angle -= 5.f * fall;
            }
            else
            {
                const float s = t - landing, decay = std::max(0.f, 1.f - s / (TAP_SHAKE - landing));
                angle += std::sin(s * 30.f) * 2.5f * decay * decay;
                shiftX = std::sin(s * 26.f) * 3.f * decay * decay;
            }
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
        jump = 10.f * std::sin(std::min(1.f, m_phaseTime / CLOSE) * PI);   // A little hop as it snaps shut
        break;
    default:
        break;
    }

    sf::Sprite clam(texture);
    clam.setOrigin(CLAM_ORIGIN);
    clam.setScale({ CLAM_SCALE, CLAM_SCALE });
    clam.setRotation(sf::degrees(angle));
    clam.setPosition(CLAM_BASE + sf::Vector2f{ shiftX, -jump });
    window.draw(clam);
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
    const float closing = m_phase == Phase::CLOSING ? std::clamp(1.f - m_phaseTime / CLOSE, 0.f, 1.f) : 1.f;
    const float revealTime = m_phase == Phase::REVEALING ? m_phaseTime : 1000.f;

    for (const Reveal& reveal : m_reveals)
    {
        const float t = std::clamp((revealTime - reveal.appearAt) / 0.45f, 0.f, 1.f);
        if (t <= 0.f) continue;
        const float grow = ease_out_back(t) * closing;
        const float bob = std::sin(m_time * 2.4f + reveal.target.x * 0.01f) * 6.f;
        const sf::Vector2f position = mouth() + (reveal.target - mouth()) * std::min(1.f, t * 1.2f) + sf::Vector2f{ 0.f, bob };

        const float size = 200.f * reveal.scale * grow;
        sf::Sprite bubble(bubbleTexture);
        bubble.setOrigin({ bubbleSize / 2.f, bubbleSize / 2.f });
        bubble.setScale({ size * 1.15f / bubbleSize, size * 1.15f / bubbleSize });
        bubble.setPosition(position);
        bubble.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(170 * closing)));
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
            fish.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(255 * closing)));
            window.draw(fish);
        }

        if (t < 1.f || closing < 1.f) continue;

        // Under the bubble: the name, the tag (NEW!, +1 EXTRA LEVEL, +n SHELLS), the level with its extra levels.
        // Smaller in the x10 grid, where each fish has a 240 px column
        const bool isBig = reveal.scale > 1.f;
        const unsigned int nameSize = isBig ? 40 : 32, tagSize = isBig ? 34 : 25, levelSize = isBig ? 30 : 22;
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
        y += static_cast<float>(tagSize) * 1.15f;
        if (!reveal.level.empty())
        {
            // "LEVEL 7 +3", centered as one
            sf::Text measure(UISkin::heavy_font(), reveal.level + " " + reveal.extra);
            UISkin::style_text(measure, levelSize, UISkin::NUMBER);
            const float left = position.x - measure.getLocalBounds().size.x / 2.f;
            const sf::FloatRect level = line(reveal.level, levelSize, UISkin::NUMBER, left, { 0.f, 0.f });
            line(reveal.extra, levelSize, UISkin::EXTRA, level.position.x + level.size.x + static_cast<float>(levelSize) * 0.3f, { 0.f, 0.f });
        }
    }
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
        const float t = (m_phaseTime - SCALE_GAP * static_cast<float>(i)) / SCALE_FLIGHT;
        if (t <= 0.f || t >= 1.f) continue;
        const float eased = t * t * (3.f - 2.f * t);
        sf::Vector2f position = SCALE_COUNTER + (mouth() - SCALE_COUNTER) * eased;
        position.y -= std::sin(t * PI) * 160.f;   // An arc
        sf::Sprite scale(icon);
        scale.setOrigin({ iconSize / 2.f, iconSize / 2.f });
        const float size = 70.f * (1.f - 0.4f * t);
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

        // A click on the clam: it heaves and thuds (just for fun), not more than once per TAP_COOLDOWN
        if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>(); pressed && pressed->button == sf::Mouse::Button::Left)
        {
            const sf::Vector2f mouse = window.mapPixelToCoords(pressed->position);
            if (clam_bounds().contains(mouse) && m_tapCooldown <= 0.f)
            {
                m_tapTime = TAP_SHAKE;
                m_tapCooldown = TAP_COOLDOWN;
                m_hasTapLanded = false;
            }
        }
        return;
    }
    // Anything else: a click (or Space / Enter) skips ahead
    if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>(); pressed && pressed->button == sf::Mouse::Button::Left) skip();
    if (const auto* key = event.getIf<sf::Event::KeyPressed>(); key && (key->code == sf::Keyboard::Key::Space || key->code == sf::Keyboard::Key::Enter)) skip();
}

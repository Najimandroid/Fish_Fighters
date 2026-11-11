#include "UIBattleIcon.h"
#include "../UISkin.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

namespace
{
    const float FRAME_SCALE = 1.f;                       // Corners of the slot images on screen
    const unsigned int COST_SIZE = 32;
    const sf::Color GREY{ 96, 102, 112 };                // Not available: the fish greyed out
    const std::uint8_t GREY_ALPHA = 170;
    const float GREY_RECEDE = 0.55f;                     // Not enough cash: the grey thins out by that much at the price
    const sf::Color BAR_TRACK{ 20, 30, 40, 230 };        // The cooldown bar, in the price strip
    const sf::Color WATER{ 72, 224, 255, 70 };           // The cash "water level" rising in a slot not affordable yet
    const sf::Color EMPTY_FRAME{ 255, 255, 255, 90 };    // An empty slot: its outline only
    const unsigned int KEY_SIZE = 20;
    const sf::Vector2f KEY_CHIP{ 30.f, 30.f };
    const float READY_FLASH_SECONDS = 0.15f;             // White flash when it can be deployed again
    const float READY_POP_SECONDS = 0.2f;                // 1.0 -> 1.08 -> 1.0
    const float SHAKE_SECONDS = 0.18f;                   // A refused click: 6 px, 3 times
}

/*
 * Initializes the button and loads the icon texture.
 */
UIBattleIcon::UIBattleIcon(sf::Vector2f position, const std::string& iconTexturePath) :
    UIButtonElement({ WIDTH, HEIGHT }, position, ""),
    m_keyText(UISkin::heavy_font())
{
    // Load icon texture
    set_texture(iconTexturePath);
    set_click_sound("");   // The deploy (or its error) makes the sound
    set_fires_on_press(true);   // Real-time play: deploys the moment it's pressed

    // Price text: yellow outlined digits
    UISkin::style_text(m_text, COST_SIZE, UISkin::NUMBER);
    UISkin::style_text(m_keyText, KEY_SIZE, UISkin::TITLE);
    set_cost(m_cost);
}

/*
 * Updates cooldown progress, the ready flash and the shake
 */
void UIBattleIcon::update(float deltaTime)
{
    bool hasCooldownEnded = false;
    if (m_isOnCooldown)
    {
        m_currentCooldown += deltaTime;

        if (m_currentCooldown >= m_maxCooldown)
        {
            // Cooldown finished
            m_isOnCooldown = false;
            m_currentCooldown = 0.0f;
            hasCooldownEnded = true;
        }
    }

    // It just became deployable: flash and pop (the sound only when a cooldown ends, not for every cash step)
    const bool isReady = m_uid >= 0 && !m_isOnCooldown && !m_isDarkened;
    if (isReady && !m_wasReady && !m_isFirstUpdate)
    {
        m_readyTime = 0.f;
        if (hasCooldownEnded && m_maxCooldown >= 5.f)
            play_sound("unit_ready", 1.f, std::clamp((m_shape.getPosition().x + WIDTH / 2.f) / 1920.f * 2.f - 1.f, -1.f, 1.f) * 0.5f);
    }
    m_wasReady = isReady;
    m_isFirstUpdate = false;
    if (m_readyTime >= 0.f && (m_readyTime += deltaTime) > READY_POP_SECONDS) m_readyTime = -1.f;
    if (m_shakeTime >= 0.f && (m_shakeTime += deltaTime) > SHAKE_SECONDS) m_shakeTime = -1.f;

    m_text.setString(m_uid < 0 ? "" : std::to_string(m_cost) + "$");
}

/*
 * Draws the slot, the unit icon inside it, the cash water level or the cooldown shade, the frame over
 * the icon edges, the price, the seconds left and the hotkey
 */
void UIBattleIcon::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    sf::FloatRect rect = get_visual_rect();

    // Pop when it becomes ready, shake when refused
    if (m_readyTime >= 0.f)
    {
        const float t = m_readyTime / READY_POP_SECONDS;
        const float pop = 1.f + 0.08f * std::sin(t * 3.14159265f);
        const sf::Vector2f center = rect.position + rect.size / 2.f;
        rect.size *= pop;
        rect.position = center - rect.size / 2.f;
    }
    if (m_shakeTime >= 0.f)
        rect.position.x += 6.f * std::sin(m_shakeTime / SHAKE_SECONDS * 3.f * 2.f * 3.14159265f) * (1.f - m_shakeTime / SHAKE_SECONDS);

    const float scale = rect.size.x / WIDTH;
    const sf::FloatRect inner = UISkin::slot_inner(rect, FRAME_SCALE * scale);

    // Empty slot: just a faint outline, so the tray shows the real choices
    if (m_uid < 0)
    {
        UISkin::draw_nine_slice(window, UISkin::SLOT_FRAME, rect, FRAME_SCALE * scale, EMPTY_FRAME);
        return;
    }

    UISkin::draw_nine_slice(window, UISkin::SLOT, rect, FRAME_SCALE * scale);

    // Unit icon cropped to the shape of the slot, filling it
    UISkin::draw_cropped(window, m_texture, inner);

    if (m_isOnCooldown || m_isDarkened)
    {
        // Greyed out; while it can't be paid, the grey thins out as the cash rises toward the price
        const float fade = m_isOnCooldown ? 1.f : 1.f - GREY_RECEDE * std::clamp(m_affordability, 0.f, 1.f);
        sf::RectangleShape shade(inner.size);
        shade.setPosition(inner.position);
        shade.setFillColor(sf::Color(GREY.r, GREY.g, GREY.b, static_cast<std::uint8_t>(GREY_ALPHA * fade)));
        window.draw(shade);
    }
    if (m_isDarkened && !m_isOnCooldown)
    {
        // The cash "water level" rising toward the price
        const float level = std::clamp(m_affordability, 0.f, 1.f);
        sf::RectangleShape water({ inner.size.x, inner.size.y * level });
        water.setPosition({ inner.position.x, inner.position.y + inner.size.y * (1.f - level) });
        water.setFillColor(WATER);
        window.draw(water);
    }

    // White flash when it becomes ready
    if (m_readyTime >= 0.f && m_readyTime < READY_FLASH_SECONDS)
    {
        sf::RectangleShape flash(inner.size);
        flash.setPosition(inner.position);
        flash.setFillColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(160.f * (1.f - m_readyTime / READY_FLASH_SECONDS))));
        window.draw(flash);
    }

    // Price on a navy strip along the bottom (like the Equip lineup), always readable and off the face
    const float stripHeight = inner.size.y * 0.3f;
    sf::RectangleShape strip({ inner.size.x, stripHeight });
    strip.setPosition({ inner.position.x, inner.position.y + inner.size.y - stripHeight });
    strip.setFillColor(sf::Color(16, 24, 34, 190));
    window.draw(strip);

    // Frame over the icon edges (hides its corners under the rounded border, and the strip's)
    UISkin::draw_nine_slice(window, UISkin::SLOT_FRAME, rect, FRAME_SCALE * scale);

    if (m_isOnCooldown)
    {
        // The cooldown bar fills the strip's middle, where the price goes back once it's ready
        const float progress = std::clamp(m_currentCooldown / m_maxCooldown, 0.f, 1.f);
        const sf::Vector2f trackSize{ inner.size.x - 24.f * scale, stripHeight * 0.45f };
        const sf::Vector2f trackPosition{ inner.position.x + 12.f * scale, inner.position.y + inner.size.y - stripHeight / 2.f - trackSize.y / 2.f };
        sf::RectangleShape track(trackSize);
        track.setPosition(trackPosition);
        track.setFillColor(BAR_TRACK);
        track.setOutlineThickness(2.f * scale);
        track.setOutlineColor(UISkin::OUTLINE);
        window.draw(track);
        sf::RectangleShape fill({ trackSize.x * progress, trackSize.y });
        fill.setPosition(trackPosition);
        fill.setFillColor(UISkin::COOLDOWN);
        window.draw(fill);
    }
    else
    {
        UISkin::set_text_anchor(m_text, { 0.5f, 0.5f });
        m_text.setScale({ scale * 0.9f, scale * 0.9f });
        m_text.setPosition({ inner.position.x + inner.size.x / 2.f, inner.position.y + inner.size.y - stripHeight / 2.f });
        window.draw(m_text);
    }

    // Hotkey chip at the top left
    if (!m_keyText.getString().isEmpty())
    {
        const sf::Vector2f chipSize = KEY_CHIP * scale;
        const sf::Vector2f chipPosition = rect.position + sf::Vector2f{ 10.f, 10.f } * scale;
        sf::RectangleShape chip(chipSize);
        chip.setPosition(chipPosition);
        chip.setFillColor(sf::Color(16, 24, 34, 200));
        window.draw(chip);
        UISkin::set_text_anchor(m_keyText, { 0.5f, 0.5f });
        m_keyText.setScale({ scale, scale });
        m_keyText.setPosition(chipPosition + chipSize / 2.f);
        window.draw(m_keyText);
    }
}

void UIBattleIcon::set_affordability(float fraction)
{
    m_affordability = fraction;
}

void UIBattleIcon::set_key_label(const std::string& label)
{
    m_keyText.setString(label);
}

void UIBattleIcon::shake()
{
    m_shakeTime = 0.f;
}

// Getters
int UIBattleIcon::get_uid() const { return m_uid; }
bool UIBattleIcon::get_if_on_cooldown() const { return m_isOnCooldown; }
int UIBattleIcon::get_cost() const { return m_cost; }

// Setters
void UIBattleIcon::set_uid(int uid) { m_uid = uid; }
void UIBattleIcon::set_max_cooldown(float cooldown) { m_maxCooldown = cooldown; }

/*
 * Sets the unit deployment cost (a negative cost hides the price)
 */
void UIBattleIcon::set_cost(int cost)
{
    m_cost = cost;
    m_text.setString(m_cost < 0 ? "" : std::to_string(m_cost) + "$");
}

/*
 * Begins the cooldown: the slot is shaded and the bar fills up
 */
void UIBattleIcon::start_cooldown()
{
    m_isOnCooldown = true;
    m_currentCooldown = 0.0f;
}

/*
 * Loads a texture from file (placeholder if missing). render() crops and scales it into the slot
 */
void UIBattleIcon::set_texture(const std::string& texturePath)
{
    if (!m_texture.loadFromFile(texturePath))
        m_texture.loadFromFile("assets/images/textures/icons/placeholder.png");
    m_texture.setSmooth(true);
}

/*
 * Darkens the icon when unavailable, e.g., insufficient cash
 */
void UIBattleIcon::set_darkened(bool isDarkened)
{
    m_isDarkened = isDarkened;
}

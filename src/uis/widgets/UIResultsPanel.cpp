#include "UIResultsPanel.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace
{
    const sf::Vector2f SCREEN_SIZE{ 1920.f, 1080.f };
    const float PANEL_WIDTH = 900.f;
    const sf::Color DIM{ 16, 24, 34 };                  // The UI navy
    const sf::Color CORAL{ 255, 128, 104 };             // "FIRST CLEAR!", "NEW FISH!", the enemy base bar
    const sf::Color TRACK{ 40, 56, 74 };
    const float DIM_SECONDS = 0.25f;
    const float SLIDE_SECONDS = 0.35f;                  // The panel slides up 60 px (ease out back)
    const float BANNER_START = 0.15f, BANNER_SECONDS = 0.3f;
    const float COUNT_START = 0.6f, COUNT_SECONDS = 0.7f, TICK_SECONDS = 0.05f;
    const float NEW_FISH_TIME = 1.45f;

    // Buttons: one row, centered (main 380 wide, second 260, round 96, 24 apart)
    const sf::Vector2f PRIMARY_SIZE{ 380.f, 100.f }, SECONDARY_SIZE{ 260.f, 88.f };
    const float ROUND_SIZE = 96.f, BUTTON_GAP = 24.f;
    const float ROW_LEFT = SCREEN_SIZE.x / 2.f - (PRIMARY_SIZE.x + SECONDARY_SIZE.x + ROUND_SIZE + 2.f * BUTTON_GAP) / 2.f;

    // The panel is as tall as what it shows (a replay has only the shells), a bit above the middle
    sf::FloatRect panel_rect(const BattleResult& result)
    {
        float height = 640.f;
        if (result.isVictory)
        {
            height = 530.f;
            if (result.isFirstClear) height += 50.f;
            if (result.newUnitUID >= 0) height += 150.f;
        }
        return { { (SCREEN_SIZE.x - PANEL_WIDTH) / 2.f, (SCREEN_SIZE.y - height) / 2.f - 20.f }, { PANEL_WIDTH, height } };
    }

    float row_y(const sf::FloatRect& panel)
    {
        return panel.position.y + panel.size.y - 44.f - PRIMARY_SIZE.y;
    }

    const char* TIPS[] = {
        "Cash Up early: a bigger wallet buys bigger fish.",
        "Upgrade your fish in the Fish Tank: every level makes them stronger.",
        "Cheap fish make a good wall in front of your strong ones.",
        "Replay cleared stages for shells, then come back stronger.",
        "Enemies drop cash when they fall: keep the front line busy.",
    };

    float ease_out_back(float x)
    {
        const float c1 = 1.70158f, c3 = c1 + 1.f;
        return 1.f + c3 * std::pow(x - 1.f, 3.f) + c1 * std::pow(x - 1.f, 2.f);
    }

    std::uint8_t alpha_of(float fraction)
    {
        return static_cast<std::uint8_t>(255.f * std::clamp(fraction, 0.f, 1.f));
    }

    sf::Color with_alpha(sf::Color color, std::uint8_t alpha)
    {
        color.a = static_cast<std::uint8_t>(color.a * alpha / 255);
        return color;
    }

    // An outlined text, faded with alpha
    void draw_text(sf::RenderWindow& window, const std::string& string, unsigned int size, sf::Color fill, sf::Vector2f position,
        sf::Vector2f anchor, std::uint8_t alpha, float scale = 1.f, float rotation = 0.f)
    {
        sf::Text text(UISkin::heavy_font(), string);
        UISkin::style_text(text, size, fill);
        UISkin::set_text_anchor(text, anchor);
        text.setFillColor(with_alpha(fill, alpha));
        text.setOutlineColor(with_alpha(UISkin::OUTLINE, alpha));
        text.setScale({ scale, scale });
        text.setRotation(sf::degrees(rotation));
        text.setPosition(position);
        window.draw(text);
    }
}

UIResultsPanel::UIResultsPanel(const BattleResult& result, const std::string& stageName, std::shared_ptr<DataLoader> dataLoader, Actions actions) :
    UIElement(SCREEN_SIZE, { 0.f, 0.f }),
    m_result(result), m_panel(panel_rect(result)), m_stageName(stageName), m_actions(std::move(actions)),
    m_primaryButton(PRIMARY_SIZE, { ROW_LEFT, row_y(m_panel) }, m_actions.primaryLabel),
    m_secondaryButton(SECONDARY_SIZE, { ROW_LEFT + PRIMARY_SIZE.x + BUTTON_GAP, row_y(m_panel) + (PRIMARY_SIZE.y - SECONDARY_SIZE.y) / 2.f }, m_actions.secondaryLabel),
    m_backButton(ROUND_SIZE, { ROW_LEFT + PRIMARY_SIZE.x + SECONDARY_SIZE.x + 2.f * BUTTON_GAP, row_y(m_panel) + (PRIMARY_SIZE.y - ROUND_SIZE) / 2.f }, "icon_back")
{
    m_primaryButton.set_text_size(46);
    m_primaryButton.set_callback(m_actions.primary);
    m_primaryButton.set_hotkey(sf::Keyboard::Key::Enter);
    m_secondaryButton.set_text_size(38);
    m_secondaryButton.set_style(UIButtonElement::Style::Secondary);
    m_secondaryButton.set_callback(m_actions.secondary);
    m_backButton.set_click_sound("ui_back");
    m_backButton.set_callback(m_actions.back);
    m_backButton.set_hotkey(sf::Keyboard::Key::Escape);

    if (!m_result.isVictory)
    {
        std::random_device device;
        m_tip = TIPS[device() % std::size(TIPS)];
    }
    if (m_result.newUnitUID >= 0 && dataLoader)
    {
        if (m_newFishTexture.loadFromFile(dataLoader->get_unit_icon_texture_path(m_result.newUnitUID, 0)))
            m_newFishTexture.setSmooth(true);
    }
    if (m_result.shells <= 0) m_isCountDone = true;
}

bool UIResultsPanel::are_buttons_ready() const
{
    return m_time >= SLIDE_SECONDS;
}

void UIResultsPanel::update(float deltaTime)
{
    m_time += deltaTime;
    m_primaryButton.update(deltaTime);
    m_secondaryButton.update(deltaTime);
    m_backButton.update(deltaTime);

    if (!m_result.isVictory) return;

    // The shells count up, ticking, then land with the collect sound
    if (!m_isCountDone && m_time >= COUNT_START)
    {
        const float progress = std::min(1.f, (m_time - COUNT_START) / COUNT_SECONDS);
        const float eased = 1.f - (1.f - progress) * (1.f - progress);
        m_shownShells = static_cast<int>(std::round(eased * m_result.shells));
        if (m_time >= m_nextTick && progress < 1.f)
        {
            play_sound("reward_tick", 1.f + 0.25f * progress);
            m_nextTick = m_time + TICK_SECONDS;
        }
        if (progress >= 1.f)
        {
            m_isCountDone = true;
            play_sound("shell_collect");
        }
    }

    if (!m_isNewFishShown && m_result.newUnitUID >= 0 && m_time >= NEW_FISH_TIME)
    {
        m_isNewFishShown = true;
        play_sound("unlock");
    }
}

void UIResultsPanel::render(sf::RenderWindow& window)
{
    // The battlefield dims
    sf::RectangleShape dim(SCREEN_SIZE);
    dim.setFillColor(sf::Color(DIM.r, DIM.g, DIM.b, static_cast<std::uint8_t>(150.f * std::clamp(m_time / DIM_SECONDS, 0.f, 1.f))));
    window.draw(dim);

    // The panel slides up into place
    const float slide = std::clamp(m_time / SLIDE_SECONDS, 0.f, 1.f);
    const std::uint8_t alpha = alpha_of(slide * 2.f);
    sf::FloatRect panel = m_panel;
    panel.position.y += 60.f * (1.f - ease_out_back(slide));
    UISkin::draw_nine_slice(window, UISkin::PANEL_DARK, panel, 1.2f, sf::Color(255, 255, 255, alpha));

    // Banner: big, slightly tilted, settling from 1.4x
    const float bannerProgress = std::clamp((m_time - BANNER_START) / BANNER_SECONDS, 0.f, 1.f);
    const float bannerScale = 1.4f - 0.4f * (1.f - (1.f - bannerProgress) * (1.f - bannerProgress));
    const sf::Vector2f bannerPosition{ SCREEN_SIZE.x / 2.f, panel.position.y + 110.f };
    if (m_result.isVictory)
        draw_text(window, "VICTORY!", 120, UISkin::NUMBER, bannerPosition, { 0.5f, 0.5f }, alpha_of(bannerProgress * 3.f), bannerScale, -2.f);
    else
        draw_text(window, "DEFEAT...", 110, UISkin::TITLE, bannerPosition, { 0.5f, 0.5f }, alpha_of(bannerProgress * 3.f), bannerScale, 2.f);

    draw_text(window, m_stageName, 34, UISkin::TITLE, { SCREEN_SIZE.x / 2.f, panel.position.y + 205.f }, { 0.5f, 0.5f }, alpha);

    if (m_result.isVictory) render_victory(window, panel, alpha);
    else render_defeat(window, panel, alpha);

    if (are_buttons_ready())
    {
        m_primaryButton.render(window);
        m_secondaryButton.render(window);
        m_backButton.render(window);
    }
}

void UIResultsPanel::render_victory(sf::RenderWindow& window, const sf::FloatRect& panel, std::uint8_t alpha)
{
    // "+150 [shell]", centered as a group, yellow like every shell amount
    const float rowY = panel.position.y + 305.f;
    const float iconSize = 72.f;
    // Width of the final number, so the group doesn't move while counting
    sf::Text widest(UISkin::heavy_font(), "+" + std::to_string(m_result.shells));
    UISkin::style_text(widest, 64, UISkin::NUMBER);
    const float groupWidth = widest.getLocalBounds().size.x + 16.f + iconSize;
    const float left = SCREEN_SIZE.x / 2.f - groupWidth / 2.f;
    draw_text(window, "+" + std::to_string(m_shownShells), 64, UISkin::NUMBER, { left, rowY }, { 0.f, 0.5f }, alpha);
    UISkin::draw_fitted(window, UISkin::texture("icon_shell"),
        { { left + widest.getLocalBounds().size.x + 16.f, rowY - iconSize / 2.f }, { iconSize, iconSize } }, sf::Color(255, 255, 255, alpha));

    if (m_result.isFirstClear && m_isCountDone)
        draw_text(window, "FIRST CLEAR!", 32, CORAL, { SCREEN_SIZE.x / 2.f, rowY + 62.f }, { 0.5f, 0.5f }, alpha);

    // The new fish: its slot pops in, with what to do next
    if (m_isNewFishShown && m_newFishTexture.getSize().x > 0)
    {
        const float pop = std::clamp((m_time - NEW_FISH_TIME) / 0.25f, 0.f, 1.f);
        const float scale = 0.8f * ease_out_back(pop);
        const sf::Vector2f slotSize{ 166.f * scale, 124.f * scale };
        const float centerY = panel.position.y + 470.f;
        const float textLeft = SCREEN_SIZE.x / 2.f - 120.f;
        const sf::FloatRect slot{ { textLeft - 24.f - slotSize.x, centerY - slotSize.y / 2.f }, slotSize };
        if (scale > 0.05f)
        {
            UISkin::draw_nine_slice(window, UISkin::SLOT, slot, scale);
            UISkin::draw_cropped(window, m_newFishTexture, UISkin::slot_inner(slot, scale));
            UISkin::draw_nine_slice(window, UISkin::SLOT_FRAME, slot, scale);
        }
        draw_text(window, "NEW FISH!", 40, CORAL, { textLeft, centerY - 22.f }, { 0.f, 0.5f }, alpha_of(pop * 2.f));
        draw_text(window, "Unlock it in Upgrade", 28, UISkin::TEXT, { textLeft, centerY + 24.f }, { 0.f, 0.5f }, alpha_of(pop * 2.f));
    }
}

void UIResultsPanel::render_defeat(sf::RenderWindow& window, const sf::FloatRect& panel, std::uint8_t alpha)
{
    // How close it was: the enemy base's health left
    const int left = static_cast<int>(std::ceil(m_result.enemyBaseHealth));
    const int max = static_cast<int>(std::round(m_result.enemyBaseMaxHealth));
    draw_text(window, "Enemy base: " + std::to_string(left) + " / " + std::to_string(max) + " HP left", 32, UISkin::TEXT,
        { SCREEN_SIZE.x / 2.f, panel.position.y + 285.f }, { 0.5f, 0.5f }, alpha);

    const sf::FloatRect bar{ { SCREEN_SIZE.x / 2.f - 300.f, panel.position.y + 320.f }, { 600.f, 26.f } };
    sf::RectangleShape track(bar.size);
    track.setPosition(bar.position);
    track.setFillColor(with_alpha(TRACK, alpha));
    track.setOutlineThickness(4.f);
    track.setOutlineColor(with_alpha(UISkin::OUTLINE, alpha));
    window.draw(track);
    const float fraction = m_result.enemyBaseMaxHealth > 0.f ? std::clamp(m_result.enemyBaseHealth / m_result.enemyBaseMaxHealth, 0.f, 1.f) : 0.f;
    sf::RectangleShape fill({ bar.size.x * fraction, bar.size.y });
    fill.setPosition(bar.position);
    fill.setFillColor(with_alpha(CORAL, alpha));
    window.draw(fill);

    // A tip for next time
    draw_text(window, UISkin::wrap_text(m_tip, 30, 720.f), 30, UISkin::TITLE, { SCREEN_SIZE.x / 2.f, panel.position.y + 420.f }, { 0.5f, 0.5f }, alpha);
}

void UIResultsPanel::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!are_buttons_ready()) return;

    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyPressed->code == sf::Keyboard::Key::R && m_actions.retry)
        {
            play_sound("ui_click");
            m_actions.retry();
            return;
        }
    }

    m_primaryButton.handle_event(event, window);
    m_secondaryButton.handle_event(event, window);
    m_backButton.handle_event(event, window);
}

sf::FloatRect UIResultsPanel::get_bounds() const
{
    return sf::FloatRect{ m_position, m_size };
}

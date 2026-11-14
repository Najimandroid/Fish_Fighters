#include "UITutorial.h"
#include "UICashUp.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"
#include "../../core/stages/Stage.h"
#include "../../data/PlayerData.h"

#include <algorithm>
#include <cmath>

namespace
{
    // What the hints point at (the battle HUD's places, 1920 x 1080)
    const sf::FloatRect WALLET{ { 1456.f, 22.f }, { 440.f, 104.f } };        // UIStageCash's plate
    const sf::FloatRect DEPLOY_TRAY{ { 520.f, 938.f }, { 880.f, 124.f } };   // UIDeploymentIcons' slots
    const sf::FloatRect CASH_UP{ { 24.f, 826.f }, { 230.f, 230.f } };        // UICashUp

    const unsigned int TEXT_SIZE = 32;
    const float TEXT_WIDTH = 620.f;
    const float PADDING = 26.f;
    const float ARROW = 26.f;              // The arrow's half width and its length
    const float GAP = 18.f;                // Between the target and the arrow's tip
    const float FADE_SECONDS = 0.25f;
    const float WALLET_SECONDS = 4.5f;
    const float CASH_UP_SECONDS = 8.f;     // The upgrade hint goes after that even if it isn't bought
    const float CASH_UP_WAIT_MAX = 25.f;   // Never affordable that long: skip to the last hint
    const float BASE_SECONDS = 6.f;
    const float INTRO_SECONDS = 2.4f;      // UIBattleIntro's length
    const sf::Color HIGHLIGHT{ 72, 224, 255 };
}

bool UITutorial::is_wanted(const DataLoader& dataLoader, int stageUid)
{
    auto player = dataLoader.get_player_data().lock();
    auto chapter = dataLoader.get_chapter_data(1);
    if (!player || !chapter || chapter->stages.empty()) return false;
    return chapter->stages.front().UID == stageUid && !player->has_completed_stage(stageUid);
}

UITutorial::UITutorial(std::shared_ptr<Stage> stage, std::shared_ptr<UICashUp> cashUp) :
    m_stage(stage),
    m_cashUp(cashUp),
    m_text(UISkin::regular_font())
{
    UISkin::style_body_text(m_text, TEXT_SIZE);
    m_text.setLineSpacing(1.1f);
    if (cashUp) m_cashUpLevel = cashUp->get_level();
    go_to(Step::WALLET);
    m_stepTime = -INTRO_SECONDS;   // The first hint waits for the battle start (UIBattleIntro)
}

void UITutorial::go_to(Step step)
{
    m_step = step;
    m_stepTime = 0.f;
    const Hint hint = hint_of(step);
    m_text.setString(UISkin::wrap_text(hint.text, TEXT_SIZE, TEXT_WIDTH, UISkin::regular_font()));
    if (step == Step::DEPLOY || step == Step::BASE) play_sound("ui_select");   // The cash up hint sounds when it shows, the first one is silent
}

UITutorial::Hint UITutorial::hint_of(Step step) const
{
    switch (step)
    {
    case Step::WALLET:  return { "This is your wallet: it fills up by itself. Spend it to send fish!", WALLET, false };
    case Step::DEPLOY:  return { "Tap a fish to send it into battle! Its number key works too.", DEPLOY_TRAY, true };
    case Step::CASH_UP: return { "Upgrade your WALLET: it holds more and fills up faster.", CASH_UP, true };
    case Step::BASE:    return { "Destroy the enemy base on the left to win. Don't let them reach your fish bowl!", {}, false };
    default:            return { "", {}, false };
    }
}

void UITutorial::update(float deltaTime)
{
    m_stepTime += deltaTime;

    auto stage = m_stage.lock();
    if (!stage || m_step == Step::DONE) return;
    if (stage->is_finishing()) { go_to(Step::DONE); return; }

    auto cashUp = m_cashUp.lock();
    switch (m_step)
    {
    case Step::WALLET:
        if (m_stepTime >= WALLET_SECONDS) go_to(Step::DEPLOY);
        break;

    case Step::DEPLOY:
    {
        size_t units = 0;
        for (const auto& [layer, list] : stage->get_units()) units += list.size();
        if (units > 0) { m_waitTime = 0.f; go_to(Step::CASH_UP); m_stepTime = -1.f; }   // -1: not shown yet
        break;
    }

    case Step::CASH_UP:
        // Waits (hidden) until the upgrade can be bought, then shows until it is, or for a while
        if (m_stepTime < 0.f)
        {
            m_stepTime = -1.f;
            m_waitTime += deltaTime;
            if (cashUp && cashUp->can_upgrade()) { m_stepTime = 0.f; play_sound("ui_select"); }
            else if (!cashUp || m_waitTime >= CASH_UP_WAIT_MAX) go_to(Step::BASE);
            break;
        }
        if ((cashUp && cashUp->get_level() > m_cashUpLevel) || m_stepTime >= CASH_UP_SECONDS) go_to(Step::BASE);
        break;

    case Step::BASE:
        if (m_stepTime >= BASE_SECONDS) go_to(Step::DONE);
        break;

    default:
        break;
    }
}

void UITutorial::render(sf::RenderWindow& window)
{
    if (!m_isVisible || m_step == Step::DONE || m_stepTime < 0.f) return;

    const Hint hint = hint_of(m_step);
    const float alpha = std::clamp(m_stepTime / FADE_SECONDS, 0.f, 1.f);
    const float pulse = 0.5f + 0.5f * std::sin(m_stepTime * 6.f);
    const bool hasTarget = hint.target.size.x > 0.f;

    // A pulsing frame around the target
    if (hasTarget)
    {
        const float grow = 6.f + 4.f * pulse;
        sf::RectangleShape frame(hint.target.size + sf::Vector2f{ grow, grow } * 2.f);
        frame.setPosition(hint.target.position - sf::Vector2f{ grow, grow });
        frame.setFillColor(sf::Color::Transparent);
        frame.setOutlineThickness(6.f);
        frame.setOutlineColor(sf::Color(HIGHLIGHT.r, HIGHLIGHT.g, HIGHLIGHT.b, static_cast<std::uint8_t>(255 * alpha * (0.55f + 0.45f * pulse))));
        window.draw(frame);
    }

    // The bubble: centered on the target (kept on screen), above or below it; without a target, at the top
    const sf::FloatRect textBounds = m_text.getLocalBounds();
    const sf::Vector2f size{ std::max(textBounds.size.x, 200.f) + PADDING * 2.f, textBounds.size.y + PADDING * 2.f + 8.f };
    sf::Vector2f position;
    if (hasTarget)
    {
        const float centerX = hint.target.position.x + hint.target.size.x / 2.f;
        position.x = std::clamp(centerX - size.x / 2.f, 24.f, 1920.f - 24.f - size.x);
        position.y = hint.isBubbleAbove ? hint.target.position.y - GAP - ARROW - size.y
                                        : hint.target.position.y + hint.target.size.y + GAP + ARROW;
    }
    else
    {
        position = { (1920.f - size.x) / 2.f, 170.f };
    }
    const float bob = hasTarget ? 5.f * std::sin(m_stepTime * 3.f) * (hint.isBubbleAbove ? -1.f : 1.f) : 0.f;
    position.y += bob;

    const sf::Color tint(255, 255, 255, static_cast<std::uint8_t>(255 * alpha));
    UISkin::draw_nine_slice(window, UISkin::PANEL_DARK, { position, size }, 0.6f, tint);

    // The arrow from the bubble to the target
    if (hasTarget)
    {
        const float tipX = std::clamp(hint.target.position.x + hint.target.size.x / 2.f, position.x + 40.f, position.x + size.x - 40.f);
        const float baseY = hint.isBubbleAbove ? position.y + size.y - 4.f : position.y + 4.f;
        const float tipY = hint.isBubbleAbove ? baseY + ARROW + GAP * 0.5f : baseY - ARROW - GAP * 0.5f;
        sf::ConvexShape arrow(3);
        arrow.setPoint(0, { tipX - ARROW, baseY });
        arrow.setPoint(1, { tipX + ARROW, baseY });
        arrow.setPoint(2, { tipX, tipY });
        arrow.setFillColor(sf::Color(HIGHLIGHT.r, HIGHLIGHT.g, HIGHLIGHT.b, tint.a));
        arrow.setOutlineThickness(4.f);
        arrow.setOutlineColor(sf::Color(UISkin::OUTLINE.r, UISkin::OUTLINE.g, UISkin::OUTLINE.b, tint.a));
        window.draw(arrow);
    }

    UISkin::set_text_color(m_text, sf::Color(255, 255, 255, tint.a));
    UISkin::set_text_anchor(m_text, { 0.f, 0.f });
    m_text.setPosition(position + sf::Vector2f{ PADDING, PADDING + 4.f });
    window.draw(m_text);
}

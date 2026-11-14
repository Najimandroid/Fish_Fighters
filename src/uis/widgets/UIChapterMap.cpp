#include "UIChapterMap.h"
#include "../UISkin.h"
#include "../EntityPortraits.h"

#include "../../core/stages/Stage.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
    // The selected stage is brought to this point of the screen (between the title bar and the info panel)
    const sf::Vector2f VIEW_CENTER{ 960.f, 480.f };

    // Info panel at the bottom, between the back button and the BATTLE button (UIManager)
    const sf::FloatRect INFO_PANEL{ { 180.f, 862.f }, { 1260.f, 190.f } };
    const float MAP_TOP = 112.f, MAP_BOTTOM = INFO_PANEL.position.y - 6.f;   // Where a press can drag the map (not the bars)
    // The stages' names along the top (tabs), under the title bar
    const float STRIP_Y = 124.f;
    const sf::Vector2f STRIP_TAB{ 330.f, 56.f };
    const float STRIP_SPACING = STRIP_TAB.x + 40.f;   // From a tab's middle to the next one's: one stage of sliding
    const float MIDDLE_TAB_SCALE = 1.16f;             // The tab in the middle is a bit bigger than the others

    const float MARKER_SIZE = 66.f;
    const float SELECTED_MARKER_SIZE = 84.f;
    const sf::Color CLEARED_TINT{ 150, 235, 150 };   // Turns the yellow marker green
    const sf::Color LOCKED_TINT{ 150, 156, 166 };    // Grey disc: what comes next, not playable yet
    const int LOCKED_SHOWN = 2;                      // How many locked stages ahead the map shows
    const std::uint8_t LOCKED_PATH_ALPHA = 100;      // Their path is faint

    // Keeps the map covering the whole screen, a phone's extra width too (no black past its edges); a map smaller
    // than the screen stays centered
    float clamp_axis(float offset, float mapSize, float screenStart, float screenSize)
    {
        const float lowest = screenStart + screenSize - mapSize, highest = screenStart;
        return lowest > highest ? (lowest + highest) / 2.f : std::clamp(offset, lowest, highest);
    }
    sf::Vector2f clamp_to_map(sf::Vector2f offset, sf::Vector2f mapSize)
    {
        const sf::FloatRect screen = UISkin::screen_rect();
        return { clamp_axis(offset.x, mapSize.x, screen.position.x, screen.size.x),
                 clamp_axis(offset.y, mapSize.y, screen.position.y, screen.size.y) };
    }
}

UIChapterMap::UIChapterMap(std::shared_ptr<DataLoader> dataLoader)
    : m_playerData(dataLoader->get_player_data()), m_dataLoader(dataLoader), m_background(m_backgroundTexture),
    m_marker(UISkin::texture("button_round")),
    m_markerNumber(UISkin::heavy_font()),
    m_infoPanel(INFO_PANEL.size, INFO_PANEL.position, UISkin::PANEL_DARK, 1.2f),
    m_stageName(UISkin::heavy_font()),
    m_stageDetails(UISkin::heavy_font()),
    m_rewardText(UISkin::heavy_font()),
    m_clearedText(UISkin::heavy_font(), "CLEARED!"),
    m_enemiesTitle(UISkin::heavy_font(), "ENEMIES")
{
    UISkin::style_text(m_enemiesTitle, 28, UISkin::TITLE);
    m_offsetTween = tweeny::from(0.f, 0.f).to(0.f, 0.f).during(1).via(tweeny::easing::linear);

    m_marker.setOrigin(sf::Vector2f(m_marker.getTexture().getSize()) / 2.f);
    UISkin::style_text(m_markerNumber, 34);

    m_selectionRing.setFillColor(sf::Color::Transparent);
    m_selectionRing.setOutlineColor(sf::Color::White);
    m_selectionRing.setOutlineThickness(6.f);

    UISkin::style_text(m_stageName, 48, UISkin::NUMBER);
    UISkin::style_text(m_stageDetails, 30);
    UISkin::style_text(m_rewardText, 30, UISkin::NUMBER);
    UISkin::style_text(m_clearedText, 36, CLEARED_TINT);
}

void UIChapterMap::load_chapter(int chapterUID)
{
    auto chapter = m_dataLoader->get_chapter_data(chapterUID);
    if (!chapter)
    {
        std::cerr << "UIChapterMap::load_chapter - Chapter " << chapterUID << " not found\n";
        return;
    }

    if (!m_backgroundTexture.loadFromFile(chapter->mapTexture))
    {
        std::cerr << "Failed to load map texture: " << chapter->mapTexture << "\n";
    }
    else
    {
        m_backgroundTexture.setSmooth(true);   // Drawn at 2x: smooth instead of stepped edges
        m_background.setTextureRect(sf::IntRect({ 0, 0 }, static_cast<sf::Vector2i>(m_backgroundTexture.getSize())));
        m_background.setPosition({ 0.f, 0.f });
        m_background.setScale({ 2.f, 2.f });
    }

    m_currentChapter = chapter;
    build_stage_icons();
}

void UIChapterMap::select_stage(int stageUID)
{
    if (stageUID != m_selectedStage) play_sound("ui_select");
    m_selectedStage = stageUID;
    if (auto player = m_playerData.lock())
        player->lastStage = stageUID;   // The map opens here next time
    std::cout << "Selected stage " << stageUID << "\n";

    auto it = std::find_if(m_stageIcons.begin(), m_stageIcons.end(),
        [&](const StageIcon& icon) { return icon.stageUID == m_selectedStage; });

    if (it != m_stageIcons.end())
    {
        const sf::Vector2f targetOffset = offset_for(static_cast<int>(it - m_stageIcons.begin()));

        describe_selected_stage();

        m_offsetTween = tweeny::from(m_viewOffset.x, m_viewOffset.y)
            .to(targetOffset.x, targetOffset.y)
            .during(30)
            .via(tweeny::easing::quarticOut);
    }
}

void UIChapterMap::build_stage_icons()
{
    m_stageIcons.clear();

    auto player = m_playerData.lock();
    if (!player || !m_currentChapter) return;

    // layout: wrap after N columns (helps pour chapitres longs)
    const int columns = 10; // change si tu veux plus/moins par ligne
    int lockedShown = 0;

    for (size_t i = 0; i < m_currentChapter->stages.size(); ++i)
    {
        const auto& stageMapData = m_currentChapter->stages[i];  // <-- Add this
        int stageUID = stageMapData.UID;

        // get stage data (we only need UID here, but keep it robust)
        auto stageData = m_dataLoader->get_stage_data(stageUID);
        if (!stageData)
        {
            // si stage introuvable, skip mais loguer si nécessaire
            std::cerr << "UIChapterMap::build_stage_icons - stage " << stageUID << " not found\n";
            continue;
        }

        StageIcon icon;
        icon.stageUID = stageUID;
        icon.number = static_cast<int>(i) + 1;

        // completed = si joueur a fini ce stage
        icon.completed = player->has_completed_stage(stageUID);

        // unlocked = premier stage OU précédent complété OU déjà complété (sécurité)
        bool prevCompleted = false;
        if (i == 0)
            prevCompleted = true;
        else
            prevCompleted = player->has_completed_stage(m_currentChapter->stages[i - 1].UID);

        icon.unlocked = prevCompleted || icon.completed;

        // Click area of the marker (render() draws the marker itself)
        const float radius = SELECTED_MARKER_SIZE / 2.f;

        icon.circle.setRadius(radius);
        icon.circle.setOrigin({ radius, radius });
        icon.circle.setPosition(stageMapData.position);

        // The playable ones, and the next locked ones as a hint of what's ahead
        if (icon.unlocked || icon.completed) {
            m_stageIcons.push_back(std::move(icon));
        }
        else if (lockedShown < LOCKED_SHOWN) {
            lockedShown++;
            m_stageIcons.push_back(std::move(icon));
        }
    }

    // si la stage sélectionnée précédente n'appartient pas à ce chapitre, la clear
    if (m_selectedStage != -1)
    {
        bool stillInChapter = false;
        for (auto [uid, position] : m_currentChapter->stages)
            if (uid == m_selectedStage) { stillInChapter = true; break; }
        if (!stillInChapter) m_selectedStage = -1;
    }

    // The last stage played, if it's in this chapter (and unlocked)
    if (m_selectedStage == -1 && player->lastStage != -1)
    {
        for (const StageIcon& icon : m_stageIcons)
            if (icon.stageUID == player->lastStage) { m_selectedStage = icon.stageUID; break; }
    }

    if (m_selectedStage == -1 && !m_currentChapter->stages.empty())
    {
        // Chercher le premier stage non complété
        for (auto[uid, position] : m_currentChapter->stages) {
            if (!player->has_completed_stage(uid))
            {
                m_selectedStage = uid;
                break;
            }
        }
        // Si tout est complété, fallback sur le premier
        if (m_selectedStage == -1)
            m_selectedStage = m_currentChapter->stages[0].UID;
    }

    // Center camera on the selected stage
    if (m_selectedStage != -1) 
    {
        auto it = std::find_if(m_stageIcons.begin(), m_stageIcons.end(),
            [&](const StageIcon& icon) { return icon.stageUID == m_selectedStage; });

        if (it != m_stageIcons.end()) 
        {
            // Compute immediate offset
            sf::Vector2f targetOffset = clamp_to_map(VIEW_CENTER - it->circle.getPosition(), m_background.getGlobalBounds().size);

            m_offsetTween = tweeny::from(m_viewOffset.x, m_viewOffset.y)
                .to(targetOffset.x, targetOffset.y)
                .during(1.f)
                .via(tweeny::easing::quarticOut);
        }
    }

    // Adding connections between stage circles
    m_stageConnections.clear();

    for (size_t i = 0; i + 1 < m_stageIcons.size(); ++i)
    {
        const auto& a = m_stageIcons[i];
        const auto& b = m_stageIcons[i + 1];

        // A path from each stage to the next one (faint towards a locked one)
        if (a.completed || !b.unlocked)
        {
            m_stageConnections.push_back(
                make_dotted_line(a.circle.getPosition(), b.circle.getPosition(), 28.f, 6.f)
            );
            if (!b.unlocked)
                for (auto& dot : m_stageConnections.back())
                {
                    dot.setFillColor(sf::Color(255, 255, 255, LOCKED_PATH_ALPHA));
                    dot.setOutlineColor(sf::Color(UISkin::OUTLINE.r, UISkin::OUTLINE.g, UISkin::OUTLINE.b, LOCKED_PATH_ALPHA));
                }
        }
    }

    // Name and rewards of the selected stage
    describe_selected_stage();
}

/*
 * The info panel: stage name, enemy base health, rewards (shells and a new fish), CLEARED when done
 */
void UIChapterMap::describe_selected_stage()
{
    auto stage = m_dataLoader->get_stage_data(m_selectedStage);
    if (!stage) return;

    m_stageName.setString(stage->name);
    m_stageDetails.setString("Enemy base: " + std::to_string(static_cast<int>(stage->baseHealth)) + " HP");

    int shells = 0;
    bool hasNewFish = false;
    for (const auto& reward : stage->rewards)
    {
        if (reward->type == "shells") shells += reward->amount;
        else if (reward->type == "unit") hasNewFish = true;
    }
    m_rewardText.setString(std::to_string(shells) + " shells" + (hasNewFish ? "   + a new fish!" : ""));

    // Every enemy of the stage once, in the order of the waves
    m_stageEnemies.clear();
    for (const auto& wave : stage->enemies)
    {
        if (std::find(m_stageEnemies.begin(), m_stageEnemies.end(), wave->UID) == m_stageEnemies.end()
            && m_dataLoader->get_enemy_data(wave->UID))
            m_stageEnemies.push_back(wave->UID);
    }

    const float left = INFO_PANEL.position.x + 40.f;
    const float top = INFO_PANEL.position.y;
    UISkin::set_text_anchor(m_stageName, { 0.f, 0.5f });
    m_stageName.setPosition({ left, top + 48.f });
    UISkin::set_text_anchor(m_stageDetails, { 0.f, 0.5f });
    m_stageDetails.setPosition({ left, top + 106.f });
    UISkin::set_text_anchor(m_rewardText, { 0.f, 0.5f });
    m_rewardText.setPosition({ left + 52.f, top + 150.f });   // After the shell icon
    // CLEARED right after the name (the enemies use the right of the panel)
    const sf::FloatRect nameBounds = m_stageName.getGlobalBounds();
    UISkin::set_text_anchor(m_clearedText, { 0.f, 0.5f });
    m_clearedText.setPosition({ nameBounds.position.x + nameBounds.size.x + 24.f, top + 50.f });
}

/*
 * A navy padlock on a locked marker: the shackle (a ring, its lower half hidden by the body), the body, a keyhole
 */
void UIChapterMap::draw_padlock(sf::RenderWindow& window, const sf::RenderStates& states, sf::Vector2f center, float size) const
{
    const float unit = size / 66.f;
    sf::CircleShape shackle(9.f * unit);
    shackle.setOrigin({ 9.f * unit, 9.f * unit });
    shackle.setPosition(center + sf::Vector2f{ 0.f, -6.f * unit });
    shackle.setFillColor(sf::Color::Transparent);
    shackle.setOutlineThickness(4.f * unit);
    shackle.setOutlineColor(UISkin::OUTLINE);
    window.draw(shackle, states);

    sf::RectangleShape body({ 28.f * unit, 20.f * unit });
    body.setOrigin({ 14.f * unit, 0.f });
    body.setPosition(center + sf::Vector2f{ 0.f, -3.f * unit });
    body.setFillColor(UISkin::OUTLINE);
    window.draw(body, states);

    sf::CircleShape keyhole(3.f * unit);
    keyhole.setOrigin({ 3.f * unit, 3.f * unit });
    keyhole.setPosition(center + sf::Vector2f{ 0.f, 6.f * unit });
    keyhole.setFillColor(LOCKED_TINT);
    window.draw(keyhole, states);
}

/*
 * A white check on a navy disc, at the corner of a cleared marker
 */
void UIChapterMap::draw_check(sf::RenderWindow& window, const sf::RenderStates& states, sf::Vector2f center) const
{
    sf::CircleShape disc(15.f);
    disc.setOrigin({ 15.f, 15.f });
    disc.setPosition(center);
    disc.setFillColor(UISkin::OUTLINE);
    window.draw(disc, states);

    auto stroke = [&](sf::Vector2f from, sf::Vector2f to)
        {
            const sf::Vector2f d = to - from;
            sf::RectangleShape line({ std::sqrt(d.x * d.x + d.y * d.y) + 4.f, 5.f });
            line.setOrigin({ 2.f, 2.5f });
            line.setPosition(center + from);
            line.setRotation(sf::radians(std::atan2(d.y, d.x)));
            line.setFillColor(sf::Color::White);
            window.draw(line, states);
        };
    stroke({ -7.f, 0.f }, { -2.f, 6.f });
    stroke({ -2.f, 6.f }, { 8.f, -6.f });
}

std::vector<sf::CircleShape> UIChapterMap::make_dotted_line(sf::Vector2f from, sf::Vector2f to, float spacing, float radius)
{
    std::vector<sf::CircleShape> dots;

    sf::Vector2f dir = to - from;
    float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);

    if (length == 0.f) return dots;

    dir /= length; // normalize
    int count = static_cast<int>(length / spacing);

    for (int i = 1; i < count; ++i) // skip endpoints
    {
        sf::Vector2f pos = from + dir * (i * spacing);

        sf::CircleShape dot(radius);
        dot.setOrigin({ radius, radius });
        dot.setPosition(pos);
        dot.setFillColor(sf::Color::White);
        dot.setOutlineColor(UISkin::OUTLINE);
        dot.setOutlineThickness(3.f);

        dots.push_back(dot);
    }

    return dots;
}


void UIChapterMap::update(float deltaTime)
{
    if (!m_isVisible) return;

    if (m_offsetTween.progress() != 1.f)
    {
        auto [ox, oy] = m_offsetTween.step(1);
        m_viewOffset = { ox, oy };
    }

    // Flung: the strip and the map glide on and slow down, then land (select_stage puts the map in place)
    if (m_glide.isActive)
    {
        const bool isGliding = m_glide.step(deltaTime);
        follow_drag(m_glide.position);
        if (!isGliding) select_stage(m_selectedStage);
    }

    // The names strip slides to the selected stage (while a finger holds it or it glides, it follows that instead)
    const int index = selected_index();
    if (index >= 0 && !m_isFollowing && !m_glide.isActive)
        m_stripSlide += (static_cast<float>(index) - m_stripSlide) * std::min(1.f, deltaTime * 10.f);
}

void UIChapterMap::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    sf::RenderStates states;
    states.transform.translate(m_viewOffset);

    window.draw(m_background, states);

    for (auto& connection : m_stageConnections)
        for (auto& dot : connection)
            window.draw(dot, states);

    // Markers: yellow, green once cleared, the selected one bigger with a white ring
    const sf::Vector2f markerTexture(m_marker.getTexture().getSize());
    for (auto& icon : m_stageIcons)
    {
        const bool isSelected = icon.stageUID == m_selectedStage;
        const float size = isSelected ? SELECTED_MARKER_SIZE : MARKER_SIZE;
        const sf::Vector2f center = icon.circle.getPosition();

        if (isSelected)
        {
            m_selectionRing.setRadius(size / 2.f + 4.f);
            m_selectionRing.setOrigin({ m_selectionRing.getRadius(), m_selectionRing.getRadius() });
            m_selectionRing.setPosition(center);
            window.draw(m_selectionRing, states);
        }

        m_marker.setScale({ size / markerTexture.x, size / markerTexture.y });
        m_marker.setPosition(center);
        if (!icon.unlocked && !icon.completed)
        {
            sf::CircleShape disc(size / 2.f - 4.f);
            disc.setOrigin({ disc.getRadius(), disc.getRadius() });
            disc.setPosition(center);
            disc.setFillColor(LOCKED_TINT);
            disc.setOutlineThickness(4.f);
            disc.setOutlineColor(UISkin::OUTLINE);
            window.draw(disc, states);
            draw_padlock(window, states, center, size);
            continue;
        }
        m_marker.setColor(icon.completed ? CLEARED_TINT : sf::Color::White);
        window.draw(m_marker, states);
        if (icon.completed) draw_check(window, states, center + sf::Vector2f{ size * 0.36f, -size * 0.36f });

        m_markerNumber.setString(std::to_string(icon.number));
        m_markerNumber.setScale({ size / SELECTED_MARKER_SIZE, size / SELECTED_MARKER_SIZE });
        UISkin::set_text_anchor(m_markerNumber, { 0.5f, 0.5f });
        m_markerNumber.setPosition(center);
        window.draw(m_markerNumber, states);
    }

    // Info panel of the selected stage
    m_infoPanel.render(window);
    window.draw(m_stageName);
    window.draw(m_stageDetails);
    UISkin::draw_fitted(window, UISkin::texture("icon_shell"),
        { { INFO_PANEL.position.x + 40.f, INFO_PANEL.position.y + 130.f }, { 40.f, 40.f } });
    window.draw(m_rewardText);

    auto player = m_playerData.lock();
    if (player && player->has_completed_stage(m_selectedStage))
        window.draw(m_clearedText);

    render_stage_enemies(window, player.get());
    render_strip(window);
}

/*
 * The enemies of the selected stage in slots on the right of the info panel, like The Battle Cats:
 * black until the player has met them in battle
 */
void UIChapterMap::render_stage_enemies(sf::RenderWindow& window, const PlayerData* player)
{
    if (m_stageEnemies.empty()) return;

    const float right = INFO_PANEL.position.x + INFO_PANEL.size.x - 34.f;
    const float slotsTop = INFO_PANEL.position.y + 76.f;
    const float gap = 10.f;
    const float maxWidth = 640.f;
    const float count = static_cast<float>(m_stageEnemies.size());
    const float slotSize = std::min(88.f, (maxWidth - gap * (count - 1.f)) / count);
    const float left = right - count * slotSize - (count - 1.f) * gap;

    UISkin::set_text_anchor(m_enemiesTitle, { 0.f, 0.5f });
    m_enemiesTitle.setPosition({ left, INFO_PANEL.position.y + 50.f });
    window.draw(m_enemiesTitle);

    for (size_t i = 0; i < m_stageEnemies.size(); ++i)
    {
        auto enemy = m_dataLoader->get_enemy_data(m_stageEnemies[i]);
        if (!enemy) continue;

        const sf::FloatRect slot{ { left + static_cast<float>(i) * (slotSize + gap), slotsTop }, { slotSize, slotSize } };
        const float frameScale = slotSize / 124.f;
        const bool isKnown = player && player->has_seen_enemy(enemy->UID);

        UISkin::draw_nine_slice(window, UISkin::SLOT, slot, frameScale);
        const sf::FloatRect inner = UISkin::slot_inner(slot, frameScale);
        EntityPortraits::draw_enemy(window, *enemy, m_dataLoader->get_enemy_icon_texture_path(enemy->UID), inner, !isKnown);
        UISkin::draw_nine_slice(window, UISkin::SLOT_FRAME, slot, frameScale);
    }
}

/*
 * The next playable stage (direction +1) or the previous one (-1) from the one selected; nothing past the ends
 */
void UIChapterMap::step_stage(int direction)
{
    const int index = selected_index();
    if (index < 0) return;
    m_glide.isActive = false;   // The keys take over from a glide
    for (int i = index + direction; i >= 0 && i < static_cast<int>(m_stageIcons.size()); i += direction)
        if (m_stageIcons[i].unlocked || m_stageIcons[i].completed)
        {
            select_stage(m_stageIcons[i].stageUID);
            return;
        }
}

int UIChapterMap::selected_index() const
{
    for (int i = 0; i < static_cast<int>(m_stageIcons.size()); ++i)
        if (m_stageIcons[i].stageUID == m_selectedStage) return i;
    return -1;
}

// The first and the last stage that can be played (unlocked or cleared); -1 both when none can
void UIChapterMap::playable_range(int& first, int& last) const
{
    first = last = -1;
    for (int i = 0; i < static_cast<int>(m_stageIcons.size()); ++i)
        if (m_stageIcons[i].unlocked || m_stageIcons[i].completed)
        {
            if (first < 0) first = i;
            last = i;
        }
}

sf::Vector2f UIChapterMap::offset_for(int index) const
{
    return clamp_to_map(VIEW_CENTER - m_stageIcons[index].circle.getPosition(), m_background.getGlobalBounds().size);
}

/*
 * A finger sliding sideways: the strip follows it (one stage per tab's width), and the map glides between the
 * stages the strip is between. Past the first or last playable stage it gives a little, then stops
 */
void UIChapterMap::follow_drag(float dragX)
{
    int first, last;
    playable_range(first, last);
    if (first < 0) return;

    float slide = static_cast<float>(m_followFrom) - dragX / STRIP_SPACING;
    if (slide < first) slide = first - std::min(0.4f, (first - slide) * 0.35f);
    if (slide > last) slide = last + std::min(0.4f, (slide - last) * 0.35f);
    m_stripSlide = slide;

    const float onMap = std::clamp(slide, static_cast<float>(first), static_cast<float>(last));
    const int from = static_cast<int>(std::floor(onMap)), to = std::min(from + 1, last);
    const sf::Vector2f offset = offset_for(from) + (offset_for(to) - offset_for(from)) * (onMap - static_cast<float>(from));
    m_viewOffset = offset;
    m_offsetTween = tweeny::from(offset.x, offset.y).to(offset.x, offset.y).during(1);
    m_offsetTween.step(1);
}

int UIChapterMap::playable_near(int index) const
{
    int first, last;
    playable_range(first, last);
    if (first < 0) return -1;
    int target = std::clamp(index, first, last);
    // A locked stage between playable ones: the nearest playable one back towards where the slide started
    while (target != m_followFrom && !(m_stageIcons[target].unlocked || m_stageIcons[target].completed))
        target += target > m_followFrom ? -1 : 1;
    return target;
}

// Where a slow slide settles when let go: the nearest playable stage (a short flick from rest still moves one)
int UIChapterMap::snap_index(float dragX, bool allowsFlick) const
{
    int target = static_cast<int>(std::lround(static_cast<float>(m_followFrom) - dragX / STRIP_SPACING));
    if (allowsFlick && target == m_followFrom && std::abs(dragX) > UISwipe::DISTANCE) target += dragX < 0.f ? 1 : -1;
    return playable_near(target);
}

/*
 * Let go: flung (faster than UISwipe::GLIDE_SPEED), the strip and the map glide on and slow down, landing on the
 * stage that speed carries them to (selected at once: its panel shows while they glide); slower, they settle on the
 * nearest playable stage
 */
void UIChapterMap::release_slide(float dragX, float speed)
{
    const bool isFlung = std::abs(speed) > UISwipe::GLIDE_SPEED;
    const int target = isFlung ? playable_near(UIGlide::landing(static_cast<float>(m_followFrom), dragX, speed, STRIP_SPACING))
        : snap_index(dragX, !m_caughtGlide && std::abs(m_dragBase) < 20.f);
    if (target < 0) return;
    if (isFlung) m_glide.start(dragX, static_cast<float>(m_followFrom - target) * STRIP_SPACING);
    select_stage(m_stageIcons[target].stageUID);
}

/*
 * The strip's tab of a stage (its slide follows the selection or the finger): the one in the middle grows to
 * MIDDLE_TAB_SCALE, smoothly as it comes
 */
sf::FloatRect UIChapterMap::strip_tab_rect(int index) const
{
    const float slot = static_cast<float>(index) - m_stripSlide;
    const float closeness = std::max(0.f, 1.f - std::abs(slot));
    const float scale = 1.f + (MIDDLE_TAB_SCALE - 1.f) * closeness * closeness * (3.f - 2.f * closeness);
    const sf::Vector2f size = STRIP_TAB * scale;
    const sf::Vector2f center{ 960.f + slot * STRIP_SPACING, STRIP_Y + STRIP_TAB.y / 2.f };
    return { center - size / 2.f, size };
}

/*
 * The stages' names along the top (like a stage select's tabs): the selected one in the middle, yellow; cleared ones
 * ticked; the next locked ones grey. Its neighbours show which way to slide
 */
void UIChapterMap::render_strip(sf::RenderWindow& window)
{
    if (m_stageIcons.empty()) return;
    const sf::FloatRect screen = UISkin::screen_rect();

    // A soft dark band behind it, so the names read over any part of the map
    sf::VertexArray band(sf::PrimitiveType::TriangleStrip, 4);
    const float top = STRIP_Y - 10.f, bottom = STRIP_Y + STRIP_TAB.y + 12.f;
    band[0] = sf::Vertex{ { screen.position.x, top }, sf::Color(8, 20, 34, 150) };
    band[1] = sf::Vertex{ { screen.position.x + screen.size.x, top }, sf::Color(8, 20, 34, 150) };
    band[2] = sf::Vertex{ { screen.position.x, bottom }, sf::Color(8, 20, 34, 0) };
    band[3] = sf::Vertex{ { screen.position.x + screen.size.x, bottom }, sf::Color(8, 20, 34, 0) };
    window.draw(band);

    // While sliding or gliding, the yellow tab is the one in the middle; the middle one is drawn last, over its
    // neighbours
    const int middle = std::clamp(static_cast<int>(std::lround(m_stripSlide)), 0, static_cast<int>(m_stageIcons.size()) - 1);
    std::vector<int> order;
    for (int i = 0; i < static_cast<int>(m_stageIcons.size()); ++i)
        if (i != middle) order.push_back(i);
    order.push_back(middle);

    sf::Text name(UISkin::heavy_font());
    for (const int i : order)
    {
        const StageIcon& icon = m_stageIcons[i];
        const sf::FloatRect tab = strip_tab_rect(i);
        if (tab.position.x > screen.position.x + screen.size.x || tab.position.x + tab.size.x < screen.position.x) continue;
        const bool isSelected = (m_isFollowing || m_glide.isActive) ? i == middle : icon.stageUID == m_selectedStage;
        const bool isPlayable = icon.unlocked || icon.completed;
        const float scale = tab.size.y / STRIP_TAB.y;

        sf::ConvexShape shape = UISkin::rounded_rect({ tab.position + sf::Vector2f{ UISkin::SHADOW_SHIFT, UISkin::SHADOW_DROP } * 0.6f, tab.size }, 20.f);
        shape.setFillColor(UISkin::SHADOW_TINT);
        window.draw(shape);
        shape = UISkin::rounded_rect(tab, 20.f);
        shape.setFillColor(isSelected ? UISkin::NUMBER : isPlayable ? sf::Color(26, 44, 64, 235) : sf::Color(58, 64, 74, 215));
        shape.setOutlineThickness(4.f);
        shape.setOutlineColor(UISkin::OUTLINE);
        window.draw(shape);

        // Its name (fitted to the tab), "Locked" for the ones ahead
        std::string label = "Locked";
        if (isPlayable)
            if (auto stage = m_dataLoader->get_stage_data(icon.stageUID)) label = std::to_string(icon.number) + "  " + stage->name;
        unsigned int size = static_cast<unsigned int>(std::lround(28.f * scale));
        const unsigned int smallest = static_cast<unsigned int>(std::lround(20.f * scale));
        UISkin::style_text(name, size, isSelected ? UISkin::OUTLINE : isPlayable ? UISkin::TEXT : sf::Color(170, 176, 186));
        name.setString(label);
        const float room = tab.size.x - (icon.completed ? 62.f : 28.f) * scale;
        while (size > smallest && name.getLocalBounds().size.x > room) UISkin::style_text(name, --size, name.getFillColor());
        if (isSelected) name.setOutlineThickness(0.f);
        UISkin::set_text_anchor(name, { 0.5f, 0.5f });
        name.setPosition({ tab.position.x + tab.size.x / 2.f + (icon.completed ? 16.f * scale : 0.f), tab.position.y + tab.size.y / 2.f - 2.f * scale });
        window.draw(name);

        // Cleared: a green tick at its left
        if (icon.completed)
        {
            const sf::Vector2f c{ tab.position.x + 24.f * scale, tab.position.y + tab.size.y / 2.f };
            sf::CircleShape dot(12.f);
            dot.setOrigin({ 12.f, 12.f });
            dot.setPosition(c);
            dot.setFillColor(sf::Color(96, 196, 96));
            dot.setOutlineThickness(3.f);
            dot.setOutlineColor(UISkin::OUTLINE);
            window.draw(dot);
            for (const auto& [from, to] : { std::pair{ sf::Vector2f{ -6.f, 0.f }, sf::Vector2f{ -2.f, 5.f } }, std::pair{ sf::Vector2f{ -2.f, 5.f }, sf::Vector2f{ 6.f, -5.f } } })
            {
                const sf::Vector2f delta = to - from;
                sf::RectangleShape stroke({ std::hypot(delta.x, delta.y) + 3.f, 3.f });
                stroke.setOrigin({ 1.5f, 1.5f });
                stroke.setPosition(c + from);
                stroke.setRotation(sf::radians(std::atan2(delta.y, delta.x)));
                stroke.setFillColor(sf::Color::White);
                window.draw(stroke);
            }
        }
    }
}

void UIChapterMap::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    // Sliding sideways (on the map or its strip) slides the strip and the map with the finger, through the playable
    // stages, and settles on the nearest one when let go, like a stage select; flung, they glide on (a tap catches
    // them). A tap picks a stage (its tab in the strip or its marker on the map)
    if (const auto* mouseButtonPressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        const sf::Vector2f point = window.mapPixelToCoords(mouseButtonPressed->position);
        const bool wasGliding = m_glide.isActive && mouseButtonPressed->button == sf::Mouse::Button::Left;
        if (wasGliding) m_glide.isActive = false;
        if (mouseButtonPressed->button == sf::Mouse::Button::Left && point.y > MAP_TOP && point.y < MAP_BOTTOM)
        {
            // On the map: a glide stops under the finger, which can slide it on from there
            m_swipe.press(point);
            m_followFrom = std::max(0, selected_index());
            m_dragBase = (static_cast<float>(m_followFrom) - m_stripSlide) * STRIP_SPACING;
            m_caughtGlide = wasGliding;
            if (wasGliding) follow_drag(m_dragBase);
            m_isFollowing = false;
        }
        else if (wasGliding)
            select_stage(m_selectedStage);   // Elsewhere (BATTLE!, the panel): it lands on the selected stage at once
    }
    else if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
    {
        m_swipe.move(window.mapPixelToCoords(mouseMoved->position));
        const sf::Vector2f drag = m_swipe.last - m_swipe.start;
        if (m_swipe.isDown && !m_stageIcons.empty()
            && (m_isFollowing || (std::abs(drag.x) > 12.f && std::abs(drag.x) > 1.4f * std::abs(drag.y))))
        {
            m_isFollowing = true;
            follow_drag(m_dragBase + drag.x);
        }
    }
    else if (const auto* mouseButtonReleased = event.getIf<sf::Event::MouseButtonReleased>(); mouseButtonReleased && m_swipe.isDown)
    {
        const sf::Vector2f point = window.mapPixelToCoords(mouseButtonReleased->position);
        const float dragX = m_dragBase + point.x - m_swipe.start.x;
        m_swipe.release(point);
        if (m_isFollowing || m_caughtGlide)
        {
            // Let go (or a glide caught by a tap): flung, the strip and the map glide on; else they settle on the
            // nearest playable stage (back where they were if it's the same)
            m_isFollowing = false;
            release_slide(dragX, m_swipe.velocity.x);
            m_caughtGlide = false;
            return;
        }
        for (int i = 0; i < static_cast<int>(m_stageIcons.size()); ++i)
            if (strip_tab_rect(i).contains(point))
            {
                if (m_stageIcons[i].unlocked || m_stageIcons[i].completed) select_stage(m_stageIcons[i].stageUID);
                return;
            }
        const sf::Vector2f worldPos = point - m_viewOffset;
        for (auto& icon : m_stageIcons)
            if ((icon.unlocked || icon.completed) && icon.circle.getGlobalBounds().contains(worldPos))
                select_stage(icon.stageUID);
    }
    else if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyPressed->code == sf::Keyboard::Key::Right) step_stage(1);
        else if (keyPressed->code == sf::Keyboard::Key::Left) step_stage(-1);
    }
}



int UIChapterMap::get_selected_stage_uid()
{
    return m_selectedStage;
}

#include "UIChapterMap.h"
#include "../UISkin.h"
#include "../EntityPortraits.h"

#include "../../core/stages/Stage.h"

#include <algorithm>
#include <iostream>

namespace
{
    // The selected stage is brought to this point of the screen (between the title bar and the info panel)
    const sf::Vector2f VIEW_CENTER{ 960.f, 480.f };

    // Info panel at the bottom, between the back button and the BATTLE button (UIManager)
    const sf::FloatRect INFO_PANEL{ { 180.f, 862.f }, { 1260.f, 190.f } };

    const float MARKER_SIZE = 66.f;
    const float SELECTED_MARKER_SIZE = 84.f;
    const sf::Color CLEARED_TINT{ 150, 235, 150 };   // Turns the yellow marker green

    // Keeps the map covering the whole screen (no black past its edges)
    sf::Vector2f clamp_to_map(sf::Vector2f offset, sf::Vector2f mapSize)
    {
        return {
            std::clamp(offset.x, std::min(0.f, 1920.f - mapSize.x), 0.f),
            std::clamp(offset.y, std::min(0.f, 1080.f - mapSize.y), 0.f)
        };
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
        sf::Vector2f targetOffset = clamp_to_map(VIEW_CENTER - it->circle.getPosition(), m_background.getGlobalBounds().size);

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

        if (icon.unlocked || icon.completed) {
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

        // Only draw connection if stage i is completed
        if (a.completed)
        {
            m_stageConnections.push_back(
                make_dotted_line(a.circle.getPosition(), b.circle.getPosition(), 28.f, 6.f)
            );
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
    m_rewardText.setString(std::to_string(shells) + (hasNewFish ? "   + a new fish!" : ""));

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
        m_marker.setColor(icon.completed ? CLEARED_TINT : sf::Color::White);
        window.draw(m_marker, states);

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
        const float padding = inner.size.x * 0.08f;
        EntityPortraits::draw_enemy(window, *enemy,
            { inner.position + sf::Vector2f{ padding, padding }, inner.size - sf::Vector2f{ padding, padding } * 2.f }, !isKnown);
        UISkin::draw_nine_slice(window, UISkin::SLOT_FRAME, slot, frameScale);
    }
}

void UIChapterMap::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (const auto* mouseButtonPressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mouseButtonPressed->button == sf::Mouse::Button::Left)
        {
            sf::Vector2f worldPos = window.mapPixelToCoords(mouseButtonPressed->position);
            worldPos -= m_viewOffset;

            for (auto& icon : m_stageIcons)
            {
                if ((icon.unlocked || icon.completed) && icon.circle.getGlobalBounds().contains(worldPos))
                {
                    select_stage(icon.stageUID);
                }
            }
        }
    }
    else if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (m_stageIcons.empty()) return;

        // find current index
        auto it = std::find_if(m_stageIcons.begin(), m_stageIcons.end(),
            [&](const StageIcon& icon) { return icon.stageUID == m_selectedStage; });

        if (it == m_stageIcons.end()) return;

        int index = static_cast<int>(std::distance(m_stageIcons.begin(), it));

        if (keyPressed->code == sf::Keyboard::Key::Right)
        {
            // move forward until an unlocked/completed stage
            for (int i = index + 1; i < (int)m_stageIcons.size(); ++i)
            {
                if (m_stageIcons[i].unlocked || m_stageIcons[i].completed)
                {
                    select_stage(m_stageIcons[i].stageUID);
                    break;
                }
            }
        }
        else if (keyPressed->code == sf::Keyboard::Key::Left)
        {
            // move backward until an unlocked/completed stage
            for (int i = index - 1; i >= 0; --i)
            {
                if (m_stageIcons[i].unlocked || m_stageIcons[i].completed)
                {
                    select_stage(m_stageIcons[i].stageUID);
                    break;
                }
            }
        }
    }
}



int UIChapterMap::get_selected_stage_uid()
{
    return m_selectedStage;
}

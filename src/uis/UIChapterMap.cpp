#include "UIChapterMap.h"

#include "../core/stages/Stage.h"

#include <iostream>

UIChapterMap::UIChapterMap(std::shared_ptr<DataLoader> dataLoader)
    : m_playerData(dataLoader->get_player_data()), m_dataLoader(dataLoader), m_background(m_backgroundTexture),
    m_stageTitle(std::make_shared<UITextElement>(sf::Vector2f{ 500.f, 200.f }, sf::Vector2f{ 0.f, 0.f }, "Unkownk Stage", 48))
{
    m_offsetTween = tweeny::from(0.f, 0.f).to(0.f, 0.f).during(1).via(tweeny::easing::linear);
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
    m_selectedStage = stageUID;
    std::cout << "Selected stage " << stageUID << "\n";

    auto it = std::find_if(m_stageIcons.begin(), m_stageIcons.end(),
        [&](const StageIcon& icon) { return icon.stageUID == m_selectedStage; });

    if (it != m_stageIcons.end())
    {
        sf::Vector2f screenCenter(1920.f / 2.f, 1080.f / 2.f);
        sf::Vector2f targetOffset = screenCenter - it->circle.getPosition();

        m_stageTitle->set_text(m_dataLoader->get_stage_data(m_selectedStage)->name);

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

        // completed = si joueur a fini ce stage
        icon.completed = player->has_completed_stage(stageUID);

        // unlocked = premier stage OU précédent complété OU déjà complété (sécurité)
        bool prevCompleted = false;
        if (i == 0)
            prevCompleted = true;
        else
            prevCompleted = player->has_completed_stage(m_currentChapter->stages[i - 1].UID);

        icon.unlocked = prevCompleted || icon.completed;

        const float radius = 30.f;

        icon.circle.setRadius(radius);
        icon.circle.setOrigin({ radius, radius });
        icon.circle.setPosition(stageMapData.position);

        if (icon.completed)
            icon.circle.setFillColor(sf::Color::Green);
        else if (icon.unlocked)
            icon.circle.setFillColor(sf::Color::White);

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
            sf::Vector2f screenCenter(1920.f / 2.f, 1080.f / 2.f);
            sf::Vector2f targetOffset = screenCenter - it->circle.getPosition();

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
                make_dotted_line(a.circle.getPosition(), b.circle.getPosition(), 20.f, 3.f)
            );
        }
    }

    // Set stage title
    m_stageTitle->set_text(m_dataLoader->get_stage_data(m_selectedStage)->name);
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

    for (auto& icon : m_stageIcons)
    {
        if (icon.stageUID == m_selectedStage)
        {
            icon.circle.setOutlineColor(sf::Color::Yellow);
            icon.circle.setOutlineThickness(3.f);
        }
        else if (icon.completed)
        {
            icon.circle.setOutlineColor(sf::Color::Black);
            icon.circle.setOutlineThickness(1.5f);
        }
        else
        {
            icon.circle.setOutlineColor(sf::Color::Red);
            icon.circle.setOutlineThickness(1.5f);
        }
    }
}

void UIChapterMap::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    sf::RenderStates states;
    states.transform.translate(m_viewOffset);

    window.draw(m_background, states);
    m_stageTitle->render(window);

    for (auto& connection : m_stageConnections)
        for (auto& dot : connection)
            window.draw(dot, states);

    for (auto& icon : m_stageIcons)
        window.draw(icon.circle, states);
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

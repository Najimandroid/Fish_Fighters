#pragma once

#include "UITextElement.h"

#include "../../core/DataLoader.h"

#include "tweeny.h"

#include <memory>
#include <vector>

/*
 * UI for displaying a chapter map.
 * Shows stages as clickable icons, greying out locked ones and highlighting completed ones.
 */

class UIChapterMap : public UIElement
{
public:
    UIChapterMap(std::shared_ptr<DataLoader> dataLoader);

    // Load the chapter data to display
    void load_chapter(int chapterUID);

    // Select the given stage
    void select_stage(int stageUID);

    // Update logic (hover effects, animations, etc.)
    void update(float deltaTime) override;

    // Render the chapter map and stage icons
    void render(sf::RenderWindow& window) override;

    // Handle mouse clicks to select a stage
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

    // Getters
    int get_selected_stage_uid();

private:

    void build_stage_icons();

    std::vector<sf::CircleShape> make_dotted_line(sf::Vector2f from, sf::Vector2f to, float spacing, float radius);

private:

    std::shared_ptr<DataLoader> m_dataLoader;
    std::weak_ptr<PlayerData> m_playerData;

    std::shared_ptr<ChapterData> m_currentChapter;

    std::shared_ptr<UITextElement> m_stageTitle;

    struct StageIcon 
    {
        int stageUID;

        sf::CircleShape circle; // Placeholder visual

        bool completed = false;
        bool unlocked = false;
    };

    std::vector<StageIcon> m_stageIcons;
    int m_selectedStage = -1; // Stage UID

    std::vector<std::vector<sf::CircleShape>> m_stageConnections;

    sf::Sprite m_background;
    sf::Texture m_backgroundTexture;

    tweeny::tween<float, float> m_offsetTween;
    sf::Vector2f m_viewOffset{ 0.f, 0.f };      // current camera offset
    sf::Vector2f m_targetOffset{ 0.f, 0.f };    // target camera offset
    float m_moveSpeed = 5.f;
};

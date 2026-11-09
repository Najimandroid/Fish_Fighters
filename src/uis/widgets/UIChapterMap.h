#pragma once

#include "UIPanel.h"

#include "../../core/DataLoader.h"

#include "tweeny.h"

#include <memory>
#include <vector>

/*
 * UI for displaying a chapter map.
 * Shows the unlocked stages as round numbered markers on the map (yellow, green once cleared,
 * a white ring on the selected one) joined by dotted lines, and a panel with the selected stage:
 * its name, the enemy base's health and the rewards.
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

    // Name, enemy base health and rewards of the selected stage in the info panel
    void describe_selected_stage();

    std::vector<sf::CircleShape> make_dotted_line(sf::Vector2f from, sf::Vector2f to, float spacing, float radius);

private:

    std::shared_ptr<DataLoader> m_dataLoader;
    std::weak_ptr<PlayerData> m_playerData;

    std::shared_ptr<ChapterData> m_currentChapter;

    struct StageIcon
    {
        int stageUID;
        int number = 1;         // Its place in the chapter (shown on the marker)

        sf::CircleShape circle; // Position and click area of the marker

        bool completed = false;
        bool unlocked = false;
    };

    std::vector<StageIcon> m_stageIcons;
    int m_selectedStage = -1; // Stage UID

    std::vector<std::vector<sf::CircleShape>> m_stageConnections;

    sf::Sprite m_background;
    sf::Texture m_backgroundTexture;

    // Markers and the info panel
    sf::Sprite m_marker;            // button_round, tinted per stage
    sf::Text m_markerNumber;
    sf::CircleShape m_selectionRing;
    UIPanel m_infoPanel;
    sf::Text m_stageName;
    sf::Text m_stageDetails;        // Enemy base health
    sf::Text m_rewardText;          // Shells (after a shell icon) and a new fish
    sf::Text m_clearedText;         // CLEARED, on a completed stage

    tweeny::tween<float, float> m_offsetTween;
    sf::Vector2f m_viewOffset{ 0.f, 0.f };      // current camera offset
    sf::Vector2f m_targetOffset{ 0.f, 0.f };    // target camera offset
    float m_moveSpeed = 5.f;
};

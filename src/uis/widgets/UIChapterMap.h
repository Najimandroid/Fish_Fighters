#pragma once

#include "UIPanel.h"
#include "UISwipe.h"

#include "../../core/DataLoader.h"

#include "tweeny.h"

#include <memory>
#include <vector>

/*
 * UI for displaying a chapter map.
 * Shows the unlocked stages as round numbered markers on the map (yellow, green once cleared,
 * a white ring on the selected one) joined by dotted lines, and a panel with the selected stage:
 * its name, the enemy base's health, the rewards and its enemies (black until met, like The Battle Cats).
 * A strip of the stages' names runs along the top (the one in the middle a bit bigger); sliding a finger sideways
 * slides it and the map through the playable stages, and they settle on the nearest when let go.
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

    // Name, enemy base health, rewards and enemies of the selected stage in the info panel
    void describe_selected_stage();

    // The stage's enemies in slots (black until met in battle)
    void render_stage_enemies(sf::RenderWindow& window, const PlayerData* player);

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

    std::vector<StageIcon> m_stageIcons;   // Playable stages, then the next locked ones (not clickable)

    void draw_padlock(sf::RenderWindow& window, const sf::RenderStates& states, sf::Vector2f center, float size) const;
    void draw_check(sf::RenderWindow& window, const sf::RenderStates& states, sf::Vector2f center) const;
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
    sf::Text m_enemiesTitle;        // "ENEMIES" above the enemy slots
    std::vector<int> m_stageEnemies;// Enemy UIDs of the selected stage, each once

    tweeny::tween<float, float> m_offsetTween;
    sf::Vector2f m_viewOffset{ 0.f, 0.f };      // current camera offset

    // Sliding a finger sideways slides the names strip and the map with it, through the playable stages; let go, it
    // settles on the nearest one (like the Upgrade cards)
    UISwipe m_swipe;
    float m_stripSlide = 0.f;                    // The strip's position, in tabs (follows the selected index)
    bool m_isFollowing = false;                  // The strip and the map are following the finger
    int m_followFrom = 0;                        // The selected index when the slide started
    void step_stage(int direction);
    int selected_index() const;
    void playable_range(int& first, int& last) const;
    sf::Vector2f offset_for(int index) const;   // The camera offset bringing that stage to the middle
    void follow_drag(float dragX);
    int snap_index(float dragX) const;           // Where the slide settles: the nearest playable stage
    sf::FloatRect strip_tab_rect(int index) const;
    void render_strip(sf::RenderWindow& window);
    sf::Vector2f m_targetOffset{ 0.f, 0.f };    // target camera offset
    float m_moveSpeed = 5.f;
};

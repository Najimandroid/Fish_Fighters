#pragma once

#include "UIUpgradeIcon.h"
#include "UIButtonElement.h"

class DataLoader;

/*
 * UIUpgradeSlider
 * ----------------
 * A horizontal slider that displays a list of upgradeable units.
 * Allows the player to navigate through units, view details, and upgrade them.
 * Works with DataLoader to fetch player data and unit info.
 */

class UIUpgradeSlider : public UIElement
{
public:

    // Constructor
    // dataLoader: shared pointer to DataLoader for player/unit data
    UIUpgradeSlider(std::shared_ptr<DataLoader> dataLoader);

    // Initialize icons based on available upgrades and player data
    void init_icons();

    // Moves slider to display unit at new index
    void move_to_index(int newIndex);

    // Calculate the upgrade cost of a unit given its UID and current level
    int calculate_upgrade_cost(int uid, int level);

    // UI update/render/event methods
    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

private:

    int m_currentIndex = 0; // Current selected upgrade index

    std::vector<int> m_availableUpgrades; // List of all unit UIDs to display
    std::vector<std::shared_ptr<UIUpgradeIcon>> m_upgradeIcons; // Icon objects for the slider

    UIButtonElement m_upgradeButton; // Button to upgrade selected unit

    std::shared_ptr<DataLoader> m_dataLoader; // Reference to data loader

    // Lower panel for UI description
    sf::Font m_font;
    sf::Text m_description;
    sf::RectangleShape m_lowerPart;

    // Icon layout parameters
    const sf::Vector2f baseIconSize = { 350.0f, 350.0f };
    const float ICON_SPACING = 475.0f;
    const float CENTER_OFFSET_X = 40.f;
    const float CENTER_SCALE = 1.3f;
    const float SIDE_SCALE = 1.0f;
    const float TWEEN_DURATION = 0.3f; // Animation duration in seconds
};

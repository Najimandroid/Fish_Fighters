#pragma once

#include "UIUpgradeIcon.h"
#include "UIButtonElement.h"
#include "UIOfferButton.h"
#include "UISwipe.h"
#include "UIRoundButton.h"
#include "UIPanel.h"
#include "UIAbilityList.h"

class DataLoader;

/*
 * UIUpgradeSlider
 * ----------------
 * The Upgrade screen's carousel, like The Battle Cats' upgrade screen: unit cards in a row
 * (the selected one big in the middle), arrows on both sides, a big UPGRADE! button, and a panel
 * with the selected unit's name, description and abilities (UIAbilityList: hover one to see what it does).
 * Left / Right keys, the arrows or a click on a side card move it.
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
    // force: glide back to it even if it's the current one (after a drag)
    void move_to_index(int newIndex, bool force = false);
    void follow_drag(float dragX);       // The cards follow a finger sliding them
    int snap_index(float dragX) const;   // Where they settle when let go

    // Calculate the upgrade cost of a unit given its UID and current level
    int calculate_upgrade_cost(int uid, int level);
    static int calculate_upgrade_cost(const DataLoader& dataLoader, int uid, int level);

    // UI update/render/event methods
    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

private:

    // Upgrades (or unlocks) the selected unit if the player has enough shells
    void upgrade_selected();

    int m_currentIndex = 0; // Current selected upgrade index
    std::vector<int> m_availableUpgrades; // List of all unit UIDs to display
    std::vector<std::shared_ptr<UIUpgradeIcon>> m_upgradeIcons; // Icon objects for the slider

    UIOfferButton m_upgradeButton;    // UPGRADE! (UNLOCK!) with its price on it, in shells
    UISwipe m_swipe;                  // Swiping the cards sideways
    bool m_isFollowing = false;       // The cards follow the finger
    UIRoundButton m_previousButton;   // Arrows on both sides of the cards
    UIRoundButton m_nextButton;

    std::shared_ptr<DataLoader> m_dataLoader; // Reference to data loader

    // Lower panel: name and description of the selected unit
    UIPanel m_descriptionPanel;
    sf::Text m_unitName;
    sf::Text m_description;
    UIAbilityList m_abilities;  // Under the description: "Strong against Jelly"... (hover: what it does)
    int m_describedUid = -1;   // The description is wrapped again only when the selection changes
    int m_describedLevel = -1;

    // What the upgrade gives: { name, now, next } rows, the evolution line; what it costs.
    // isExtra: the Clam Room's extra levels ("+3", in UISkin::EXTRA), one value: the upgrade doesn't change them
    struct StatRow { std::string name, now, next; bool isBetter = false; bool isExtra = false; };
    std::vector<StatRow> m_statRows;
    std::string m_evolutionLine;
    int m_cost = 0;
    bool m_isMaxLevel = false;
    void describe_upgrade(int uid, int level);
    void render_upgrade_info(sf::RenderWindow& window);

    // Card layout: bottom-aligned on CARDS_BOTTOM, the selected card centered
    const sf::Vector2f baseIconSize = { 300.0f, 300.0f };
    const float CARDS_BOTTOM = 640.f;
    const float ICON_SPACING = 440.0f;
    const float CENTER_SCALE = 1.3f;
    const float SIDE_SCALE = 1.0f;
    const float CENTER_OFFSET_X = (CENTER_SCALE - SIDE_SCALE) * 300.f / 2.f; // Keeps the bigger card centered
    const float TWEEN_DURATION = 0.3f; // Animation duration in seconds
};

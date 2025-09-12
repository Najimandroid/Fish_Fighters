#pragma once

#include "UIBattleIcon.h"
#include "../core/DataLoader.h"

#include <set>

/*
 * UIDeploymentIcons
 * ------------------
 * A UI container that displays a grid of deployable unit icons for the player
 * during a stage. Each icon represents an equipped unit the player can spawn.
 */
class Stage;

class UIDeploymentIcons : public UIElement
{
public:

    // Constructor
    // Initializes the deployment icons with references to the player data loader and current stage.
    UIDeploymentIcons(std::shared_ptr<DataLoader> dataLoader, std::shared_ptr<Stage> stage);

    // Initialize the grid of unit icons based on the player's equipped units
    void init_icons();

    // Update each icon (cooldowns, darkening if unavailable)
    void update(float deltaTime) override;

    // Render all deployment icons
    void render(sf::RenderWindow& window) override;

    // Handle user input events for each icon
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

    // Returns the bounding rectangle that contains all icons
    sf::FloatRect get_bounds() const override;

private:

    // Array of UI icons, one per deployable unit slot
    std::array<std::shared_ptr<UIBattleIcon>, 10> m_icons;

    // Reference to the player data (to know which units are equipped)
    std::shared_ptr<DataLoader> m_dataLoader;

    // Weak reference to the current stage (needed to spawn units)
    std::weak_ptr<Stage> m_stage;
};

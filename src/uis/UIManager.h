#pragma once

#include "UIElement.h"
#include "../core/DataLoader.h"

#include <SFML/Graphics.hpp>
#include <vector>
#include <functional>

class Stage;
enum class GameSpeedState;

/*
 * UIManager class
 * ----------------
 * UIManager handles all User Interface (UI) elements in the game.
 * Responsibilities include:
 *   - Initializing UI elements with references to game data and stage
 *   - Updating and rendering all UI elements
 *   - Handling input events (mouse clicks, drags, etc.)
 *   - Generating different UI screens (Fish Tank, Upgrades, Equip, Battle)
 *
 * UI elements can be either:
 *   - Screen-space or
 *   - World-space (attached to the game world, e.g., UIBaseHealthInfo)
 *
 * Example usage:
 *   auto uiManager = std::make_shared<UIManager>();
 *   uiManager->init(dataLoader, stage);
 *   uiManager->generate_fish_tank_uis();
 *   uiManager->update_uis(deltaTime);
 *   uiManager->render_uis(window, uiCamera, worldCamera);
 */

class UIManager
{
public:

    UIManager() = default;

    // Initialize UIManager with game data and stage reference
    void init(std::shared_ptr<DataLoader> dataLoader, std::shared_ptr<Stage> stage);

    // Add a single UI element to the manager
    void add_ui_element(std::shared_ptr<UIElement> element);

    // Update all UI elements
    void update_uis(float deltaTime);

    // Render all UI elements, handling screen-space vs world-space rendering
    void render_uis(sf::RenderWindow& window, sf::View& uiView, sf::View& worldView);

    // Handle input events for all UI elements
    void handle_ui_events(const sf::Event& event, const sf::RenderWindow& window);

    // Generate specific UI layouts/screens
    void generate_fish_tank_uis();
    void generate_upgrade_menu_uis();
    void generate_equip_menu_uis();
    void generate_battle_uis();
    void generate_chapter_map_uis();
    void generate_victory_uis();
    void generate_defeat_uis();

    // Returns true if a given world position overlaps any UI element
    bool is_mouse_over_ui(const sf::Vector2i& worldPosition) const;

	// Callbacks for getting and setting game speed
    void set_game_speed_callback(std::function<void(GameSpeedState)> callback);
    void set_get_game_speed_callback(std::function<GameSpeedState()> callback);

private:
    std::shared_ptr<DataLoader> m_dataLoader = nullptr;  // Reference to game data
    std::weak_ptr<Stage> m_stage;                        // Reference to current game stage

    std::vector<std::shared_ptr<UIElement>> m_uiElements; // All active UI elements

    std::function<void()> m_pendingAction;              // Deferred action triggered by UI

	std::function<void(GameSpeedState)> m_setGameSpeedCallback; // Callback to set game speed state
    std::function<GameSpeedState()> m_getGameSpeedCallback; // Callback to get current game speed state
};

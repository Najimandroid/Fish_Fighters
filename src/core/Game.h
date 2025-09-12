#pragma once

#include "imgui.h"
#include "imgui-SFML.h"

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "DataLoader.h"
#include "stages/Stage.h"
#include "../uis/UIManager.h"

/*
 * Game class
 * -----------
 * Game class manages the main game loop and all core systems.
 * Responsibilities include:
 *   - Creating and managing the SFML window
 *   - Handling input and events
 *   - Managing the stage and UI systems
 *   - Updating and rendering game objects each frame
 *
 * Example usage:
 *   std::shared_ptr<Game> game = std::make_shared<Game>();
 *   game->run_game_loop();
 */

class Game
{
public:
    // Constructor initializes the window, cameras, and game systems
    Game();

    // Starts the main game loop, updates game state, and renders each frame
    void run_game_loop();

    // Terminates the game and cleans up resources
    void terminate();

    // Polls and processes all window events
    void poll_events();

    // Handles window resizing
    void resize_window(sf::Vector2u newSize);

    // Centers the game window on the user's desktop
    void center_window();

public:
    // Delta time in seconds since the last frame
    float deltaTime = 0.0f;

private:
    // Initializes UI and stage cameras with correct viewports and zoom
    void init_cameras();

    // Renders debug UI when DEBUG_MODE is enabled
    void debug_ui();

private:
    // ----- Game State -----
    bool m_isPaused = false;       // If true, game updates are paused
    bool m_isFaster = false;       // If true, game updates run at double speed

    // ----- Window and Cameras (Views) -----
    sf::RenderWindow m_window;
    sf::View m_uiCamera;           // Camera used to render UI elements
    sf::View m_stageCamera;        // Camera used to render the gameplay/stage

    sf::Clock m_deltaClock;        // Clock to compute deltaTime between frames

    const int m_frameRate = 60;    // Maximum framerate
    const sf::Vector2u m_logicalResolution = { 1920, 1080 }; // Reference resolution

    // ----- Mouse and Inputs -----
    float m_currentZoom = 1.f;     // Current zoom level of the stage camera
    bool m_isDragging = false;     // Indicates if the user is dragging the camera
    sf::Vector2i m_lastMousePos{ 0, 0 };

    // ----- Systems -----
    std::shared_ptr<DataLoader> m_dataLoader; // Loads unit and game data
    std::shared_ptr<Stage> m_stage;           // Handles gameplay and units
    std::shared_ptr<UIManager> m_uiManager;   // Handles UI elements and interactions
};

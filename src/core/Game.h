#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "GameSpeedState.h"
#include "DataLoader.h"
#include "stages/Stage.h"
#include "../uis/UIManager.h"
#include "../audio/AudioManager.h"

#ifdef DEBUG_MODE
#include "../debug/DebugTools.h"
#endif

#include <memory>

/*
 * Game class
 * -----------
 * Game class manages the main game loop and all core systems.
 * Responsibilities include:
 *   - Creating and managing the SFML window
 *   - Handling input and events
 *   - Managing the stage, UI and audio systems
 *   - Updating and rendering game objects each frame
 *   - Owning the developer tools (DEBUG_MODE), which run in their own window
 *
 * Example usage:
 *   Game game = Game();
 *   game.run_game_loop();
 *   game.terminate();
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

	// Set the game speed (0 = paused, 1 = normal, 2 = faster)
	void set_game_speed(GameSpeedState speedState);

	// Get current game speed state
    GameSpeedState get_game_speed() const;

    // Get delta time
    float get_delta_time() const;

private:
    // Initializes UI and stage cameras with correct viewports and zoom
    void init_cameras();

	// Calculates delta time and applies pause/speed modifiers
	void calculate_delta_time();

private:
    // ----- Game State -----
	GameSpeedState m_speedState = GameSpeedState::NORMAL; // Current game speed state

    // ----- Window and Cameras (Views) -----
    sf::RenderWindow m_window;
    sf::View m_uiCamera;           // Camera used to render UI elements
    sf::View m_stageCamera;        // Camera used to render the gameplay/stage

    sf::Clock m_deltaClock;        // Clock to compute deltaTime between frames
    float m_deltaTime = 0.0f;      // Delta time in seconds since the last frame (scaled by game speed)
    float m_realDeltaTime = 0.0f;  // Delta time in seconds since the last frame (not scaled)
    bool m_isStepFrameRequested = false; // Debug: play exactly one frame while paused

    const int m_frameRate = 120;    // Maximum framerate
    const sf::Vector2u m_logicalResolution = { 1920, 1080 }; // Reference resolution

    // ----- Mouse and Inputs -----
    bool m_isDragging = false;     // Indicates if the user is dragging the camera
    sf::Vector2i m_lastMousePos{ 0, 0 };

    // ----- Systems -----
    std::shared_ptr<DataLoader> m_dataLoader; // Loads unit and game data
    std::shared_ptr<Stage> m_stage;           // Handles gameplay and units
    std::shared_ptr<UIManager> m_uiManager;   // Handles UI elements and interactions
    std::shared_ptr<AudioManager> m_audio;    // Music and sound effects

	// ----- Debug -----
#ifdef DEBUG_MODE
    std::unique_ptr<DebugTools> m_debugTools; // Developer tools window (F1)
#endif
};

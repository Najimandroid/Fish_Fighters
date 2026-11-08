#pragma once

#ifdef DEBUG_MODE

#include <functional>
#include <memory>

class DataLoader;
class Stage;
class UIManager;
enum class GameSpeedState;

/*
 * DebugOverlaySettings
 * --------------------
 * What the developer tools draw on top of the battlefield (in the game window).
 */
struct DebugOverlaySettings
{
    bool showHitboxes = false;      // Blue: body
    bool showAttackRanges = false;  // Yellow: starts an attack when an opponent is inside
    bool showDamageZones = false;   // Red: where the hit lands
    bool showLabels = true;         // Name #id, HP, state and statuses above every entity
    bool showSelection = true;      // Outline around the entity selected in the Inspector
};

/*
 * DebugContext
 * ------------
 * Everything the debug panels can reach. Owned by DebugTools, passed to every panel.
 * The game's systems are shared with Game; game speed goes through callbacks so the
 * panels never depend on the Game class itself.
 */
struct DebugContext
{
    // ----- Game systems -----
    std::shared_ptr<DataLoader> dataLoader;
    std::shared_ptr<Stage> stage;
    std::shared_ptr<UIManager> uiManager;

    // ----- Game speed -----
    std::function<GameSpeedState()> getGameSpeed;
    std::function<void(GameSpeedState)> setGameSpeed;
    std::function<void()> requestStepFrame;     // Advances exactly one frame while paused

    // ----- Shared debug state -----
    float frameTime = 0.f;                      // Real duration of the last game frame (not scaled by speed)
    int selectedEntityId = -1;                  // Entity selected in the Inspector (-1 = none)
    DebugOverlaySettings overlays;
};

#endif

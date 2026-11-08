#pragma once

#ifdef DEBUG_MODE

#include "DebugContext.h"
#include "panels/DebugPanel.h"

#include "imgui.h"

#include <SFML/Graphics.hpp>

#include <memory>
#include <vector>

/*
 * DebugTools
 * ----------
 * The developer tools, only built with DEBUG_MODE. Owned by Game.
 *
 * Responsibilities:
 *  - Owns a second OS window ("Developer Tools"), so the game window can stay fullscreen
 *  - Runs Dear ImGui (docking version) in that window only, with the game's font and an ocean theme
 *  - Owns every DebugPanel; each one is a dockable ImGui window, reopened from the Panels menu
 *  - Builds a default dock layout the first time (then ImGui remembers it in debug_tools.ini)
 *  - Draws the battlefield overlays (hitboxes, ranges, labels, selection) in the game window
 *
 * Usage (from Game):
 *   update(frameTime) once per frame, after the game window is displayed
 *   draw_world_overlays(gameWindow) after the stage is drawn, with the stage camera set
 *   toggle_window() when F1 is pressed in the game window
 */
class DebugTools
{
public:
    explicit DebugTools(DebugContext context);
    ~DebugTools();

    // Handles the tools window's events, then draws every open panel
    void update(float frameTime);

    // Draws the enabled overlays on the battlefield (the stage camera must be set on `target`)
    void draw_world_overlays(sf::RenderTarget& target);

    // Shows or hides the tools window
    void toggle_window();

private:
    void apply_theme();
    void draw_menu_bar();
    void build_default_layout(ImGuiID dockspaceId);

private:
    sf::RenderWindow m_window;                          // "Developer Tools" window
    sf::Clock m_imguiClock;                             // Frame time of the tools window

    DebugContext m_context;                             // Shared with every panel
    std::vector<std::unique_ptr<DebugPanel>> m_panels;

    sf::Font m_overlayFont;                             // For the labels drawn on the battlefield
    bool m_isOverlayFontLoaded = false;

    bool m_isVisible = true;
    bool m_isImGuiReady = false;
    bool m_isLayoutResetRequested = false;              // Rebuild the default layout next frame
};

#endif

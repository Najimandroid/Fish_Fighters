#pragma once

#include "ProjectData.h"
#include "TextureCache.h"
#include "AssetLibrary.h"
#include "EditorContext.h"
#include "GameBuilds.h"

#include "panels/ContentPanel.h"
#include "panels/PropertiesPanel.h"
#include "panels/PreviewPanel.h"
#include "panels/ValidationPanel.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>

#include <filesystem>
#include <string>
#include <vector>

/*
 * EditorApp
 * ---------
 * FishEditor: the game data editor, inspired by Battle Cats Ultimate's editors.
 *
 * Owns the window, the project (game_data JSON files and assets) and the dockable panels:
 *   [ Content | Properties | Preview    ]
 *   [         |            | Validation ]
 *
 * Every frame: events, shortcuts, menu, panels, then (between frames) the deferred actions,
 * the file dialog and the validation. Saving checks the data with the game's loader first.
 */
class EditorApp
{
public:
    explicit EditorApp(std::filesystem::path projectFolder);
    ~EditorApp();

    EditorApp(const EditorApp&) = delete;
    EditorApp& operator=(const EditorApp&) = delete;

    int run();

    // Opens the editor on this item (FishEditor --select stage:3)
    void select(const Selection& selection) { m_context.select(selection); }

private:
    // ----- Frame -----
    void process_events();
    void draw_frame();
    void draw_menu_bar();
    void draw_status_bar();
    void draw_panels();
    void draw_modals();
    void handle_shortcuts();
    void build_default_layout(unsigned int dockspaceId);
    void between_frames();
    void update_title();

    // ----- Actions -----
    void validate();
    void request_save();        // Asks first if the game can't load the data
    bool save();
    void undo();
    void redo();
    void reload();
    void request_close();
    void launch_game(const GameBuilds::Build& build);

private:
    std::filesystem::path m_projectFolder;
    sf::RenderWindow m_window;

    ProjectData m_project;
    TextureCache m_textures;
    AssetLibrary m_assets;
    EditorContext m_context;

    ContentPanel m_contentPanel;
    PropertiesPanel m_propertiesPanel;
    PreviewPanel m_previewPanel;
    ValidationPanel m_validationPanel;
    std::vector<EditorPanel*> m_panels;

    std::vector<GameBuilds::Build> m_builds;
    bool m_isUpdatingBuildsOnSave = true;

    bool m_isImGuiReady = false;
    bool m_isLoaded = false;
    bool m_isRunning = true;
    bool m_isLayoutResetRequested = false;
    bool m_isCloseModalRequested = false;
    bool m_isSaveModalRequested = false;
    bool m_isReloadModalRequested = false;
    bool m_isClosingAfterSave = false;

    std::string m_title;
    sf::Clock m_imguiClock;
};

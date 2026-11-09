#include "EditorApp.h"

#include "FileDialog.h"
#include "Validator.h"
#include "../tools/ToolsTheme.h"

#include "imgui.h"
#include "imgui_internal.h" // DockBuilder (default layout), BeginViewportSideBar (status bar)
#include "imgui-SFML.h"

#include <SFML/Window/Event.hpp>

#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

namespace
{
    const char* FONT_PATH = "assets/fonts/MPLUSRounded1c-Medium.ttf";  // Inside the project
    const char* LAYOUT_FILE = "fish_editor.ini";                       // Panel layout, next to the editor
}

// ==================
// Constructor / Destructor
// ==================
EditorApp::EditorApp(fs::path projectFolder) :
    m_projectFolder(std::move(projectFolder)),
    m_project(m_projectFolder),
    m_textures(m_projectFolder),
    m_assets(m_project),
    m_audio(m_project),
    m_context(m_project, m_textures, m_assets, m_audio, m_window)
{
    // 1600 x 950, smaller on small screens (keeps room for the taskbar), centered
    const sf::Vector2u desktop = sf::VideoMode::getDesktopMode().size;
    const sf::Vector2u size(std::min(1600u, desktop.x * 9 / 10), std::min(950u, desktop.y * 8 / 10));
    m_window.create(sf::VideoMode(size), "Fish Editor");
    m_window.setPosition({ static_cast<int>(desktop.x - size.x) / 2, static_cast<int>(desktop.y - size.y) / 3 });
    m_window.setFramerateLimit(60);

    // Window icon (the exe's own icon comes from resources/editor.rc)
    sf::Image icon;
    if (icon.loadFromFile(m_projectFolder / "assets/images/app/editor_icon.png"))
        m_window.setIcon(icon);

    if (!ImGui::SFML::Init(m_window, false))
    {
        std::cerr << "FishEditor: ImGui could not be initialized\n";
        return;
    }
    m_isImGuiReady = true;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = LAYOUT_FILE;
    io.ConfigWindowsMoveFromTitleBarOnly = true; // Dragging on the map moves the stages, not the window

    // The game's font, with ImGui's default as a fallback
    io.Fonts->Clear();
    if (!io.Fonts->AddFontFromFileTTF((m_projectFolder / FONT_PATH).string().c_str(), 18.f)) io.Fonts->AddFontDefault();
    if (!ImGui::SFML::UpdateFontTexture()) std::cerr << "FishEditor: font texture could not be created\n";

    ToolsTheme::apply(); // Same look as the developer tools

    m_isLayoutResetRequested = !fs::exists(LAYOUT_FILE);

    m_panels = { &m_contentPanel, &m_propertiesPanel, &m_previewPanel, &m_validationPanel };

    m_isLoaded = m_project.load();
    if (m_isLoaded) m_context.notify("Opened " + m_projectFolder.string());
    m_builds = GameBuilds::find(m_projectFolder);
}

EditorApp::~EditorApp()
{
    if (m_isImGuiReady) ImGui::SFML::Shutdown(m_window);
}

// ==================
// Main loop
// ==================
int EditorApp::run()
{
    if (!m_isImGuiReady) return 1;

    while (m_isRunning && m_window.isOpen())
    {
        process_events();

        ImGui::SFML::Update(m_window, m_imguiClock.restart());
        draw_frame();

        m_window.clear(sf::Color(12, 22, 28));
        ImGui::SFML::Render(m_window);
        m_window.display();

        between_frames();
        update_title();
    }

    m_window.close();
    return 0;
}

void EditorApp::process_events()
{
    while (const auto event = m_window.pollEvent())
    {
        ImGui::SFML::ProcessEvent(m_window, *event);

        if (event->is<sf::Event::Closed>()) request_close();

        // Images may have been changed outside: reload them
        if (event->is<sf::Event::FocusGained>())
        {
            m_assets.refresh();
            m_textures.clear();
            m_context.mark_changed();
        }
    }
}

void EditorApp::draw_frame()
{
    handle_shortcuts();
    draw_menu_bar();
    draw_status_bar();

    // The whole window is a dockspace: panels can be docked, tabbed or split anywhere
    const ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
    if (m_isLayoutResetRequested)
    {
        build_default_layout(dockspaceId);
        m_isLayoutResetRequested = false;
    }

    if (m_isLoaded)
    {
        draw_panels();
        m_saveDataEditor.draw(m_context);
        draw_build_window();
    }
    else
    {
        // The project couldn't be read: nothing can be edited until it's fixed
        ImGui::Begin("Content");
        ImGui::TextColored(ImVec4(1.f, 0.45f, 0.4f, 1.f), "The project can't be opened:");
        ImGui::TextWrapped("%s", m_project.get_error().c_str());
        ImGui::TextDisabled("Project folder: %s", m_projectFolder.string().c_str());
        if (ImGui::Button("Try again")) reload();
        ImGui::End();
    }

    draw_modals();
}

void EditorApp::draw_panels()
{
    for (EditorPanel* panel : m_panels)
    {
        if (!panel->isOpen) continue;
        if (ImGui::Begin(panel->get_name(), &panel->isOpen)) panel->draw(m_context);
        ImGui::End();
    }
}

void EditorApp::between_frames()
{
    // Structural changes asked by the panels
    for (auto& action : m_context.take_deferred()) action();

    // The file dialog blocks: it's opened here, between two frames
    if (m_context.has_dialog_request())
    {
        m_context.deliver_dialog_result(FileDialog::open_png(m_window, m_context.get_dialog_title().c_str()));
        m_imguiClock.restart();
    }
    else m_context.age_dialog_result();

    // Images the editor wrote (imports, icons, seamless backgrounds, restores) reach the game builds right away:
    // the game reads its own copy of assets, only refreshed by a game rebuild otherwise
    const std::vector<std::string> writtenFiles = m_assets.take_written_files();
    if (m_isUpdatingBuildsOnSave && !writtenFiles.empty() && !m_builds.empty())
    {
        std::string error;
        bool isCopied = true;
        for (const auto& build : m_builds)
            for (const std::string& file : writtenFiles)
                isCopied = GameBuilds::copy_file(m_projectFolder, build, file, error) && isCopied;

        if (isCopied) m_context.notify(m_context.get_status_message() + " (copied to the game builds)");
        else m_context.notify(error, true);
    }

    // Checks the data once the edits pause for a moment
    if (m_validationPanel.isCheckRequested || (m_isLoaded && m_context.is_validation_due()))
    {
        m_validationPanel.isCheckRequested = false;
        validate();
    }
}

void EditorApp::update_title()
{
    std::string title = "Fish Editor - " + m_projectFolder.filename().string();
    if (m_project.has_unsaved_changes()) title += " *";
    if (title != m_title)
    {
        m_title = title;
        m_window.setTitle(m_title);
    }
}

// ==================
// Menu bar, status bar, layout
// ==================
void EditorApp::handle_shortcuts()
{
    // RouteGlobal: a text field being typed in keeps its own Ctrl+Z
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, ImGuiInputFlags_RouteGlobal)) request_save();
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_Repeat)) undo();
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y, ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_Repeat)) redo();
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_Repeat)) redo();
    if (ImGui::Shortcut(ImGuiKey_F5, ImGuiInputFlags_RouteGlobal)) m_validationPanel.isCheckRequested = true;
}

void EditorApp::draw_menu_bar()
{
    if (!ImGui::BeginMainMenuBar()) return;

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Save", "Ctrl+S", false, m_isLoaded && m_project.has_unsaved_changes())) request_save();
        if (ImGui::MenuItem("Reload from disk", nullptr, false, m_isLoaded))
        {
            if (m_project.has_unsaved_changes()) m_isReloadModalRequested = true;
            else reload();
        }
        ImGui::Separator();
        ImGui::MenuItem("Keep the game builds up to date", nullptr, &m_isUpdatingBuildsOnSave);
        ImGui::SetItemTooltip("Copies game_data (not player.json) into out/build/* on save, and every image the editor writes right away,\n"
            "so the game sees the changes without a rebuild");
        ImGui::Separator();
        if (ImGui::MenuItem("Exit")) request_close();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        if (ImGui::MenuItem("Undo", "Ctrl+Z", false, m_project.can_undo())) undo();
        if (ImGui::MenuItem("Redo", "Ctrl+Y", false, m_project.can_redo())) redo();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Game"))
    {
        if (ImGui::MenuItem("Check the data", "F5", false, m_isLoaded)) m_validationPanel.isCheckRequested = true;
        ImGui::Separator();
        if (m_builds.empty()) ImGui::TextDisabled("No build in out/build yet");
        for (const auto& build : m_builds)
        {
            if (ImGui::MenuItem(("Play " + build.name).c_str())) launch_game(build);
            ImGui::SetItemTooltip("Saves, updates %s and starts it", build.name.c_str());
        }
        if (!m_builds.empty()) ImGui::Separator();
        const bool isBuilding = m_gameBuilder.get_state() == GameBuilder::State::RUNNING;
        for (const auto& build : m_builds)
        {
            if (ImGui::MenuItem(("Build " + build.name).c_str(), nullptr, false, !isBuilding)) build_game(build);
            ImGui::SetItemTooltip("Saves, then rebuilds the game (code changes included) like Visual Studio would.\n"
                "The build copies game_data, player.json included: the project's save replaces %s's progress.", build.name.c_str());
        }
        if (ImGui::MenuItem("Build output", nullptr, &m_isBuildWindowOpen)) {}
        ImGui::Separator();
        if (ImGui::MenuItem("Save data..."))
        {
            m_builds = GameBuilds::find(m_projectFolder);
            m_saveDataEditor.open(m_projectFolder, m_builds);
        }
        ImGui::SetItemTooltip("Edit or reset player.json (shells, cleared stages, fish, lineup, enemies met)");
        if (ImGui::MenuItem("Find builds again")) m_builds = GameBuilds::find(m_projectFolder);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Window"))
    {
        for (EditorPanel* panel : m_panels) ImGui::MenuItem(panel->get_name(), nullptr, &panel->isOpen);
        ImGui::Separator();
        if (ImGui::MenuItem("Reset layout"))
        {
            for (EditorPanel* panel : m_panels) panel->isOpen = true;
            m_isLayoutResetRequested = true;
        }
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void EditorApp::draw_status_bar()
{
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
    if (ImGui::BeginViewportSideBar("##StatusBar", ImGui::GetMainViewport(), ImGuiDir_Down, ImGui::GetFrameHeight(), flags))
    {
        if (ImGui::BeginMenuBar())
        {
            // Validation summary
            const ValidationReport& report = m_context.validation;
            if (!report.hasRun) ImGui::TextDisabled("Not checked");
            else if (report.isLoadedByGame) ImGui::TextColored(ImVec4(0.45f, 0.9f, 0.6f, 1.f), "Game loads the data");
            else ImGui::TextColored(ImVec4(1.f, 0.45f, 0.4f, 1.f), "Game can't load the data");
            if (report.hasRun)
            {
                ImGui::SameLine();
                ImGui::TextDisabled("%d error(s)  %d warning(s)", report.count(true), report.count(false));
            }

            ImGui::SameLine(0.f, 30.f);
            ImGui::TextDisabled(m_project.has_unsaved_changes() ? "Unsaved changes" : "Saved");

            // Last message, faded after a while
            if (!m_context.get_status_message().empty() && m_context.get_status_age() < 12.f)
            {
                ImGui::SameLine(0.f, 30.f);
                const ImVec4 color = m_context.is_status_error() ? ImVec4(1.f, 0.45f, 0.4f, 1.f) : ImVec4(0.8f, 0.88f, 0.9f, 1.f);
                ImGui::TextColored(color, "%s", m_context.get_status_message().c_str());
            }
            ImGui::EndMenuBar();
        }
    }
    ImGui::End();
}

void EditorApp::build_default_layout(unsigned int dockspaceId)
{
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);

    // [ Content | Properties | Preview    ]
    // [         |            | Validation ]
    ImGuiID center = dockspaceId;
    const ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.22f, nullptr, &center);
    ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.40f, nullptr, &center);
    const ImGuiID rightBottom = ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.26f, nullptr, &right);

    ImGui::DockBuilderDockWindow("Content", left);
    ImGui::DockBuilderDockWindow("Properties", center);
    ImGui::DockBuilderDockWindow("Preview", right);
    ImGui::DockBuilderDockWindow("Validation", rightBottom);
    ImGui::DockBuilderFinish(dockspaceId);
}

// ==================
// Modals
// ==================
void EditorApp::draw_modals()
{
    // ----- Close with unsaved changes -----
    if (m_isCloseModalRequested)
    {
        ImGui::OpenPopup("Unsaved changes");
        m_isCloseModalRequested = false;
    }
    if (ImGui::BeginPopupModal("Unsaved changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Save the changes before closing?");
        if (ImGui::Button("Save", ImVec2(130.f, 0.f)))
        {
            ImGui::CloseCurrentPopup();
            m_isClosingAfterSave = true;
            request_save();
        }
        ImGui::SameLine();
        if (ImGui::Button("Don't save", ImVec2(130.f, 0.f))) m_isRunning = false;
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(130.f, 0.f))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // ----- Save data the game can't load -----
    if (m_isSaveModalRequested)
    {
        ImGui::OpenPopup("Save anyway?");
        m_isSaveModalRequested = false;
    }
    if (ImGui::BeginPopupModal("Save anyway?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextColored(ImVec4(1.f, 0.45f, 0.4f, 1.f), "The game can't load this data:");
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 40.f);
        ImGui::TextUnformatted(m_context.validation.loaderError.c_str());
        ImGui::PopTextWrapPos();
        ImGui::TextDisabled("Saved like this, the game will stop at loading until it's fixed.");

        if (ImGui::Button("Save anyway", ImVec2(150.f, 0.f)))
        {
            ImGui::CloseCurrentPopup();
            save();
        }
        ImGui::SameLine();
        if (ImGui::Button("Keep editing", ImVec2(150.f, 0.f)))
        {
            m_isClosingAfterSave = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // ----- Reload with unsaved changes -----
    if (m_isReloadModalRequested)
    {
        ImGui::OpenPopup("Reload?");
        m_isReloadModalRequested = false;
    }
    if (ImGui::BeginPopupModal("Reload?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Reload the files from disk and lose the unsaved changes?");
        if (ImGui::Button("Reload", ImVec2(130.f, 0.f)))
        {
            ImGui::CloseCurrentPopup();
            reload();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(130.f, 0.f))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// ==================
// Actions
// ==================
void EditorApp::validate()
{
    m_context.validation = Validator::run(m_project, m_textures, m_audio);
    m_context.clear_validation_due();
}

void EditorApp::request_save()
{
    if (!m_isLoaded) return;

    m_project.commit_all(); // A text field still being typed in
    validate();

    if (!m_context.validation.isLoadedByGame) m_isSaveModalRequested = true;
    else save();
}

bool EditorApp::save()
{
    if (!m_project.save())
    {
        m_context.notify("Save failed: " + m_project.get_error(), true);
        m_isClosingAfterSave = false;
        return false;
    }

    std::string message = "Saved game_data";
    if (m_isUpdatingBuildsOnSave)
    {
        for (const auto& build : m_builds)
        {
            std::string error;
            if (GameBuilds::copy_data(m_projectFolder, build, error)) message += ", updated " + build.name;
            else
            {
                m_context.notify(error, true);
                if (m_isClosingAfterSave) m_isRunning = false;
                return true;
            }
        }
    }
    m_context.notify(message);

    if (m_isClosingAfterSave) m_isRunning = false;
    return true;
}

void EditorApp::undo()
{
    // After the frame: the panels may hold references into the documents
    m_context.defer([this]()
        {
            if (m_project.undo())
            {
                m_context.mark_changed();
                m_context.notify("Undo");
            }
        });
}

void EditorApp::redo()
{
    m_context.defer([this]()
        {
            if (m_project.redo())
            {
                m_context.mark_changed();
                m_context.notify("Redo");
            }
        });
}

void EditorApp::reload()
{
    m_context.defer([this]()
        {
            m_isLoaded = m_project.load();
            m_assets.refresh();
            m_audio.refresh();
            m_textures.clear();
            m_context.select({});
            m_context.mark_changed();
            m_context.notify(m_isLoaded ? "Reloaded from disk" : "Can't reload: " + m_project.get_error(), !m_isLoaded);
        });
}

void EditorApp::request_close()
{
    if (m_isLoaded && m_project.has_unsaved_changes()) m_isCloseModalRequested = true;
    else m_isRunning = false;
}

void EditorApp::build_game(const GameBuilds::Build& build)
{
    // The build copies game_data: it has to be the saved, valid data
    if (m_project.has_unsaved_changes())
    {
        m_project.commit_all();
        validate();
        if (!m_context.validation.isLoadedByGame)
        {
            m_context.notify("Not built: the game can't load the data (see Validation)", true);
            return;
        }
        if (!save()) return;
    }

    std::string error;
    if (!m_gameBuilder.start(build, error))
    {
        m_context.notify(error, true);
        return;
    }
    m_isBuildWindowOpen = true;
    m_context.notify("Building " + build.name + "...");
}

/*
 * The output of the last build, following its end, with Play once it succeeded
 */
void EditorApp::draw_build_window()
{
    if (!m_isBuildWindowOpen) return;

    ImGui::SetNextWindowSize(ImVec2(760.f, 420.f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Build", &m_isBuildWindowOpen))
    {
        const GameBuilder::State state = m_gameBuilder.get_state();
        switch (state)
        {
        case GameBuilder::State::IDLE:      ImGui::TextDisabled("No build yet: Game > Build x64-Debug..."); break;
        case GameBuilder::State::RUNNING:   ImGui::Text("Building %s...", m_gameBuilder.get_build().name.c_str()); break;
        case GameBuilder::State::SUCCEEDED: ImGui::TextColored(ImVec4(0.45f, 0.9f, 0.6f, 1.f), "%s built", m_gameBuilder.get_build().name.c_str()); break;
        case GameBuilder::State::FAILED:    ImGui::TextColored(ImVec4(1.f, 0.45f, 0.4f, 1.f), "%s: the build failed", m_gameBuilder.get_build().name.c_str()); break;
        }
        if (state == GameBuilder::State::SUCCEEDED)
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Play")) launch_game(m_gameBuilder.get_build());
        }

        ImGui::BeginChild("##log", ImVec2(0.f, 0.f), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
        const std::string log = m_gameBuilder.get_log();
        ImGui::TextUnformatted(log.c_str(), log.c_str() + log.size());
        if (state == GameBuilder::State::RUNNING) ImGui::SetScrollHereY(1.f);   // Follows the output
        ImGui::EndChild();
    }
    ImGui::End();
}

void EditorApp::launch_game(const GameBuilds::Build& build)
{
    // Playing always shows the latest data
    if (m_project.has_unsaved_changes())
    {
        m_project.commit_all();
        validate();
        if (!m_context.validation.isLoadedByGame)
        {
            m_context.notify("Not started: the game can't load the data (see Validation)", true);
            return;
        }
        if (!save()) return;
    }

    std::string error;
    if (!GameBuilds::copy_data(m_projectFolder, build, error) || !GameBuilds::launch(build, error))
    {
        m_context.notify(error, true);
        return;
    }
    m_context.notify("Started " + build.name);
}

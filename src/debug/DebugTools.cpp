#ifdef DEBUG_MODE

#include "DebugTools.h"

#include "DebugWidgets.h"
#include "panels/PerformancePanel.h"
#include "panels/StagePanel.h"
#include "panels/SpawnerPanel.h"
#include "panels/InspectorPanel.h"
#include "panels/StatusLogPanel.h"
#include "panels/CombatLogPanel.h"
#include "panels/OverlayPanel.h"

#include "../core/stages/Stage.h"

#include "imgui_internal.h" // DockBuilder (default layout)
#include "imgui-SFML.h"

#include <filesystem>
#include <iostream>

namespace
{
    const char* FONT_PATH = "assets/fonts/MPLUSRounded1c-Medium.ttf";
    const char* LAYOUT_FILE = "debug_tools.ini"; // Saved next to the game, ImGui remembers the panel layout in it
}

// ==================
// Constructor / Destructor
// ==================
DebugTools::DebugTools(DebugContext context) :
    m_context(std::move(context))
{
    m_window.create(sf::VideoMode({ 1400, 900 }), "Fish Fighters - Developer Tools");
    m_window.setPosition({ 60, 60 });

    if (!ImGui::SFML::Init(m_window, false))
    {
        std::cerr << "DebugTools: ImGui could not be initialized\n";
        return;
    }
    m_isImGuiReady = true;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = LAYOUT_FILE;

    // The game's font, with ImGui's default as a fallback
    io.Fonts->Clear();
    if (!io.Fonts->AddFontFromFileTTF(FONT_PATH, 18.f)) io.Fonts->AddFontDefault();
    if (!ImGui::SFML::UpdateFontTexture()) std::cerr << "DebugTools: font texture could not be created\n";

    m_isOverlayFontLoaded = m_overlayFont.openFromFile(FONT_PATH);

    apply_theme();

    // No saved layout yet: build the default one on the first frame
    m_isLayoutResetRequested = !std::filesystem::exists(LAYOUT_FILE);

    // Panels, in the order of the Panels menu
    m_panels.push_back(std::make_unique<InspectorPanel>());
    m_panels.push_back(std::make_unique<SpawnerPanel>());
    m_panels.push_back(std::make_unique<StagePanel>());
    m_panels.push_back(std::make_unique<PerformancePanel>());
    m_panels.push_back(std::make_unique<StatusLogPanel>());
    m_panels.push_back(std::make_unique<CombatLogPanel>());
    m_panels.push_back(std::make_unique<OverlayPanel>());
}

DebugTools::~DebugTools()
{
    if (m_isImGuiReady) ImGui::SFML::Shutdown(m_window);
}

// ==================
// Update
// ==================
void DebugTools::update(float frameTime)
{
    if (!m_isImGuiReady) return;

    m_context.frameTime = frameTime;

    while (const auto event = m_window.pollEvent())
    {
        ImGui::SFML::ProcessEvent(m_window, *event);

        // Closing the window only hides it: F1 in the game brings it back
        if (event->is<sf::Event::Closed>())
        {
            m_isVisible = false;
            m_window.setVisible(false);
        }
    }

    if (!m_isVisible) return;

    ImGui::SFML::Update(m_window, m_imguiClock.restart());

    draw_menu_bar();

    // The whole window is a dockspace: every panel can be docked, tabbed or split anywhere
    const ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
    if (m_isLayoutResetRequested)
    {
        build_default_layout(dockspaceId);
        m_isLayoutResetRequested = false;
    }

    for (auto& panel : m_panels)
    {
        if (!panel->isOpen) continue;

        if (ImGui::Begin(panel->get_name(), &panel->isOpen))
            panel->draw(m_context);
        ImGui::End();
    }

    m_window.clear(sf::Color(12, 22, 28));
    ImGui::SFML::Render(m_window);
    m_window.display();
}

void DebugTools::toggle_window()
{
    m_isVisible = !m_isVisible;
    m_window.setVisible(m_isVisible);
    if (m_isVisible) m_window.requestFocus();
}

// ==================
// Menu bar and layout
// ==================
void DebugTools::draw_menu_bar()
{
    if (!ImGui::BeginMainMenuBar()) return;

    if (ImGui::BeginMenu("Panels"))
    {
        for (auto& panel : m_panels)
            ImGui::MenuItem(panel->get_name(), nullptr, &panel->isOpen);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Layout"))
    {
        if (ImGui::MenuItem("Reset layout"))
        {
            for (auto& panel : m_panels) panel->isOpen = true;
            m_isLayoutResetRequested = true;
        }
        ImGui::EndMenu();
    }

    ImGui::TextDisabled("  F1 in the game shows or hides this window");

    ImGui::EndMainMenuBar();
}

void DebugTools::build_default_layout(ImGuiID dockspaceId)
{
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);

    // [ Inspector | Stage, Performance, Overlays | Spawner ]
    // [           |  Status Log, Combat Log      |         ]
    ImGuiID center = dockspaceId;
    const ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.30f, nullptr, &center);
    const ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.38f, nullptr, &center);
    const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.60f, nullptr, &center);

    ImGui::DockBuilderDockWindow("Inspector", left);
    ImGui::DockBuilderDockWindow("Spawner", right);
    ImGui::DockBuilderDockWindow("Stage", center);
    ImGui::DockBuilderDockWindow("Performance", center);
    ImGui::DockBuilderDockWindow("Overlays", center);
    ImGui::DockBuilderDockWindow("Status Log", bottom);
    ImGui::DockBuilderDockWindow("Combat Log", bottom);

    ImGui::DockBuilderFinish(dockspaceId);
}

// ==================
// Theme
// ==================
void DebugTools::apply_theme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark(&style);

    // Rounded, roomy, easy to read
    style.WindowPadding = ImVec2(10.f, 10.f);
    style.FramePadding = ImVec2(8.f, 5.f);
    style.ItemSpacing = ImVec2(8.f, 7.f);
    style.WindowRounding = 6.f;
    style.ChildRounding = 6.f;
    style.FrameRounding = 5.f;
    style.PopupRounding = 5.f;
    style.GrabRounding = 5.f;
    style.TabRounding = 5.f;
    style.ScrollbarRounding = 8.f;
    style.WindowBorderSize = 0.f;
    style.SeparatorTextBorderSize = 2.f;

    // Deep sea palette: dark blue-green backgrounds, teal accents, warm highlights
    const ImVec4 background = ImVec4(0.05f, 0.09f, 0.11f, 1.f);
    const ImVec4 panel = ImVec4(0.07f, 0.13f, 0.16f, 1.f);
    const ImVec4 frame = ImVec4(0.10f, 0.19f, 0.23f, 1.f);
    const ImVec4 frameHovered = ImVec4(0.13f, 0.27f, 0.32f, 1.f);
    const ImVec4 teal = ImVec4(0.10f, 0.47f, 0.53f, 1.f);
    const ImVec4 tealHovered = ImVec4(0.14f, 0.58f, 0.64f, 1.f);
    const ImVec4 tealActive = ImVec4(0.18f, 0.68f, 0.74f, 1.f);
    const ImVec4 coral = ImVec4(1.f, 0.62f, 0.45f, 1.f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.88f, 0.94f, 0.95f, 1.f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.61f, 0.65f, 1.f);
    colors[ImGuiCol_WindowBg] = panel;
    colors[ImGuiCol_ChildBg] = background;
    colors[ImGuiCol_PopupBg] = panel;
    colors[ImGuiCol_MenuBarBg] = background;
    colors[ImGuiCol_DockingEmptyBg] = background;
    colors[ImGuiCol_Border] = ImVec4(0.16f, 0.27f, 0.31f, 1.f);
    colors[ImGuiCol_FrameBg] = frame;
    colors[ImGuiCol_FrameBgHovered] = frameHovered;
    colors[ImGuiCol_FrameBgActive] = frameHovered;
    colors[ImGuiCol_TitleBg] = background;
    colors[ImGuiCol_TitleBgActive] = frame;
    colors[ImGuiCol_Button] = teal;
    colors[ImGuiCol_ButtonHovered] = tealHovered;
    colors[ImGuiCol_ButtonActive] = tealActive;
    colors[ImGuiCol_Header] = frame;
    colors[ImGuiCol_HeaderHovered] = teal;
    colors[ImGuiCol_HeaderActive] = tealHovered;
    colors[ImGuiCol_CheckMark] = coral;
    colors[ImGuiCol_SliderGrab] = tealActive;
    colors[ImGuiCol_SliderGrabActive] = coral;
    colors[ImGuiCol_Tab] = background;
    colors[ImGuiCol_TabHovered] = tealHovered;
    colors[ImGuiCol_TabSelected] = teal;
    colors[ImGuiCol_TabDimmed] = background;
    colors[ImGuiCol_TabDimmedSelected] = frame;
    colors[ImGuiCol_DockingPreview] = ImVec4(0.18f, 0.68f, 0.74f, 0.5f);
    colors[ImGuiCol_PlotLines] = coral;
    colors[ImGuiCol_PlotHistogram] = teal;
    colors[ImGuiCol_TableHeaderBg] = frame;
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.f, 1.f, 1.f, 0.03f);
    colors[ImGuiCol_SeparatorHovered] = tealHovered;
    colors[ImGuiCol_SeparatorActive] = coral;
    colors[ImGuiCol_ResizeGrip] = frame;
    colors[ImGuiCol_ResizeGripHovered] = teal;
    colors[ImGuiCol_ResizeGripActive] = coral;
}

// ==================
// Battlefield overlays
// ==================
void DebugTools::draw_world_overlays(sf::RenderTarget& target)
{
    if (!m_context.stage->is_loaded()) return;

    const DebugOverlaySettings& overlays = m_context.overlays;

    auto drawZone = [&target](const sf::FloatRect& zone, sf::Color color, float thickness)
        {
            if (zone.size.x <= 0.f) return; // Bases have empty attack and damage zones
            sf::RectangleShape shape(zone.size);
            shape.setPosition(zone.position);
            shape.setFillColor(sf::Color::Transparent);
            shape.setOutlineColor(color);
            shape.setOutlineThickness(thickness);
            target.draw(shape);
        };

    for (const DebugEntityRow& row : DebugWidgets::collect_entities(*m_context.stage))
    {
        const BattleEntity& entity = *row.entity;
        const sf::FloatRect spriteBounds = entity.sprite.getGlobalBounds();

        if (overlays.showHitboxes) drawZone(entity.hitbox, sf::Color(90, 140, 255), 1.f);
        if (overlays.showAttackRanges) drawZone(entity.attackRangeZone, sf::Color(255, 230, 80), 2.f);
        if (overlays.showDamageZones) drawZone(entity.damageZone, sf::Color(255, 90, 90), 3.f);

        if (overlays.showSelection && entity.uniqueId == m_context.selectedEntityId)
            drawZone(spriteBounds, sf::Color::Yellow, 3.f);

        if (overlays.showLabels && m_isOverlayFontLoaded)
        {
            // Name #id / HP and state / statuses, centered above the sprite
            std::string label = DebugWidgets::entity_name(row) + " #" + std::to_string(entity.uniqueId) + "\n"
                + std::to_string(static_cast<int>(entity.currentHealth)) + " HP";
            if (entity.data) label += "  " + entity.stateMachine->get_active_state_id();
            for (const StatusEffect& status : entity.get_statuses())
            {
                char statusText[48];
                std::snprintf(statusText, sizeof(statusText), "\n%s %.1fs", to_string(status.type), status.durationRemaining);
                label += statusText;
            }

            sf::Text text(m_overlayFont, label, 16);
            text.setFillColor(row.side == std::string("Enemy") ? sf::Color(255, 170, 160) : sf::Color(170, 225, 255));
            text.setOutlineColor(sf::Color::Black);
            text.setOutlineThickness(2.f);

            const sf::FloatRect textBounds = text.getLocalBounds();
            text.setOrigin({ textBounds.position.x + textBounds.size.x / 2.f, textBounds.position.y + textBounds.size.y });
            text.setPosition({ spriteBounds.position.x + spriteBounds.size.x / 2.f, spriteBounds.position.y - 6.f });
            target.draw(text);
        }
    }
}

#endif

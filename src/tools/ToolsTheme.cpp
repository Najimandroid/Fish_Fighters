#include "ToolsTheme.h"

#include "imgui.h"

namespace ToolsTheme
{
    void apply()
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
}

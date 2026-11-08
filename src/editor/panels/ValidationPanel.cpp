#include "ValidationPanel.h"

#include "../EditorContext.h"
#include "../EditorWidgets.h"

#include <algorithm>

void ValidationPanel::draw(EditorContext& ctx)
{
    const ValidationReport& report = ctx.validation;

    if (ImGui::Button("Check now (F5)")) isCheckRequested = true;
    ImGui::SameLine();
    ImGui::Checkbox("Warnings", &m_isShowingWarnings);
    ImGui::SameLine(0.f, 24.f);

    if (!report.hasRun)
    {
        ImGui::TextDisabled("Not checked yet");
        return;
    }

    if (report.isLoadedByGame) ImGui::TextColored(ImVec4(0.45f, 0.9f, 0.6f, 1.f), "The game loads this data.");
    else ImGui::TextColored(ImVec4(1.f, 0.45f, 0.4f, 1.f), "The game can't load this data.");
    ImGui::SameLine();
    ImGui::TextDisabled("%d error(s), %d warning(s)", report.count(true), report.count(false));

    ImGui::Separator();
    ImGui::BeginChild("Issues");
    if (report.issues.empty()) ImGui::TextDisabled("Nothing to fix.");

    for (size_t i = 0; i < report.issues.size(); ++i)
    {
        const ValidationIssue& issue = report.issues[i];
        if (!issue.isError && !m_isShowingWarnings) continue;

        ImGui::PushID(static_cast<int>(i));
        EditorWidgets::badge(issue.isError ? "ERROR" : "WARN", issue.isError ? IM_COL32(190, 60, 60, 255) : IM_COL32(170, 130, 40, 255));
        ImGui::SameLine();

        // Selectable as tall as the wrapped message, the text is drawn over it
        const float wrapWidth = std::max(50.f, ImGui::GetContentRegionAvail().x);
        const ImVec2 textSize = ImGui::CalcTextSize(issue.message.c_str(), nullptr, false, wrapWidth);
        const ImVec2 textPosition = ImGui::GetCursorScreenPos();

        const bool canSelect = issue.target.kind != ContentKind::NONE;
        if (ImGui::Selectable("##issue", canSelect && ctx.selection == issue.target, ImGuiSelectableFlags_None, ImVec2(wrapWidth, textSize.y)) && canSelect)
        {
            const Selection target = issue.target;
            ctx.defer([&ctx, target]() { ctx.select(target); });
        }
        if (canSelect) ImGui::SetItemTooltip("Click to open it");

        ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), textPosition,
            ImGui::GetColorU32(ImGuiCol_Text), issue.message.c_str(), nullptr, wrapWidth);
        ImGui::PopID();
    }
    ImGui::EndChild();
}

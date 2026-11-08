#ifdef DEBUG_MODE

#include "PerformancePanel.h"

#include "../DebugWidgets.h"
#include "../../core/GameSpeedState.h"
#include "../../core/stages/Stage.h"

#include <algorithm>

void PerformancePanel::draw(DebugContext& context)
{
    // ----- Frame times -----
    m_frameTimesMs[m_frameIndex] = context.frameTime * 1000.f;
    m_frameIndex = (m_frameIndex + 1) % static_cast<int>(m_frameTimesMs.size());

    float total = 0.f;
    float worst = 0.f;
    for (float frameTime : m_frameTimesMs)
    {
        total += frameTime;
        worst = std::max(worst, frameTime);
    }
    const float average = total / m_frameTimesMs.size();

    ImGui::Text("FPS: %.0f", context.frameTime > 0.f ? 1.f / context.frameTime : 0.f);
    ImGui::Text("Frame: %.2f ms  |  avg %.2f ms  |  worst %.2f ms", context.frameTime * 1000.f, average, worst);
    ImGui::PlotLines("##FrameTimes", m_frameTimesMs.data(), static_cast<int>(m_frameTimesMs.size()), m_frameIndex,
        nullptr, 0.f, std::max(20.f, worst * 1.2f), ImVec2(-FLT_MIN, 80.f));

    // ----- Game speed -----
    ImGui::SeparatorText("Game speed");

    const GameSpeedState speed = context.getGameSpeed();
    if (ImGui::RadioButton("Paused", speed == GameSpeedState::PAUSED)) context.setGameSpeed(GameSpeedState::PAUSED);
    ImGui::SameLine();
    if (ImGui::RadioButton("Normal", speed == GameSpeedState::NORMAL)) context.setGameSpeed(GameSpeedState::NORMAL);
    ImGui::SameLine();
    if (ImGui::RadioButton("x2", speed == GameSpeedState::FASTER)) context.setGameSpeed(GameSpeedState::FASTER);

    ImGui::BeginDisabled(speed != GameSpeedState::PAUSED);
    if (ImGui::Button("Step one frame")) context.requestStepFrame();
    ImGui::EndDisabled();
    DebugWidgets::help_marker("In the game window: P pauses, Left Shift plays at x2, R plays at normal speed, F1 shows or hides this window.");

    if (context.stage->is_loaded())
        ImGui::Text("Stage time: %.1f s", context.stage->get_elapsed_time());
}

#endif

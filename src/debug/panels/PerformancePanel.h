#pragma once

#ifdef DEBUG_MODE

#include "DebugPanel.h"

#include <array>

/*
 * PerformancePanel
 * ----------------
 * FPS, frame time graph, game speed (pause, normal, x2) and frame stepping.
 */
class PerformancePanel : public DebugPanel
{
public:
    PerformancePanel() : DebugPanel("Performance") {}

    void draw(DebugContext& context) override;

private:
    std::array<float, 240> m_frameTimesMs{}; // Last 240 frames, in milliseconds (ring buffer)
    int m_frameIndex = 0;                    // Next slot to write
};

#endif

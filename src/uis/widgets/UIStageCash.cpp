#include "UIStageCash.h"
#include "../UISkin.h"
#include "../../core/stages/Stage.h"
#include <iostream>

/*
 * Initializes the UITextElement with size and scale parameters.
 * Sets text styling (color, outline, size).
 */
UIStageCash::UIStageCash(std::shared_ptr<Stage> stage) :
    UITextElement({ 1.f, 1.f }, { 1.f, 1.f }),
    m_stage(stage)
{
    UISkin::style_text(m_text, 64, UISkin::NUMBER);
}

/*
 * Called every frame to refresh the displayed cash values.
 * Retrieves the current and max cash from the stage and updates text content and position.
 */
void UIStageCash::update(float deltaTime)
{
    if (auto stage = m_stage.lock()) // Lock weak_ptr to access stage safely
    {
        // Set the text to "currentCash / maxCash$"
        m_text.setString(std::to_string(stage->get_cash())
            + " / "
            + std::to_string(stage->get_max_cash()) + "$");
    }

    // Center the text's origin at its top-right corner
    UISkin::set_text_anchor(m_text, { 1.f, 0.f });

    // Position text at top-right with defined offsets
    m_text.setPosition({ 1920.f - OFFSET_X, 0.f + OFFSET_Y });
}

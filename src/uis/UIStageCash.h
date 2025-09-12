#pragma once

#include "UITextElement.h"

class Stage;

/*
 * UIStageCash
 * -----------
 * Displays the player's current cash and maximum cash during a stage.
 * Inherits from UITextElement to leverage text rendering in the UI.
 */

class UIStageCash : public UITextElement
{
public:

    /*
     * @param stage - shared pointer to the current Stage
     * Initializes the weak reference to the stage and prepares the text element.
     */
    UIStageCash(std::shared_ptr<Stage> stage);

    /*
     * Updates the displayed cash value every frame.
     * Positions the text in the top-right corner of the screen.
     */
    void update(float deltaTime) override;

private:

    std::weak_ptr<Stage> m_stage; // Weak pointer to avoid ownership

    // Text offsets from top-right corner
    const float OFFSET_Y = 10.f;
    const float OFFSET_X = 10.f;
};

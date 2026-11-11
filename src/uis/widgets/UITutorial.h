#pragma once

#include "UIElement.h"

#include <memory>
#include <string>

class Stage;
class UICashUp;

/*
 * UITutorial
 * ----------
 * The first battle's hints (chapter 1's first stage, until it's cleared): one bubble at a time, with an arrow
 * and a pulsing frame around what it talks about, each moving on when the player has done it:
 *   1. the wallet fills by itself (a few seconds)
 *   2. click a fish to send it (until a fish is in battle)
 *   3. the wallet upgrade, once it's affordable (until it's bought, or a while)
 *   4. destroy the enemy base, protect yours (a few seconds)
 * It never takes the clicks: what it points at stays usable. Gone when the battle ends.
 *
 * Example usage:
 *   if (UITutorial::is_wanted(dataLoader, stageUid)) add_ui_element(std::make_shared<UITutorial>(stage, cashUp));
 */
class UITutorial : public UIElement
{
public:
    UITutorial(std::shared_ptr<Stage> stage, std::shared_ptr<UICashUp> cashUp);

    // Whether this battle gets the hints: chapter 1's first stage, not cleared yet
    static bool is_wanted(const class DataLoader& dataLoader, int stageUid);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override { return {}; }   // Nothing to click

private:
    enum class Step { WALLET, DEPLOY, CASH_UP, BASE, DONE };

    struct Hint
    {
        std::string text;
        sf::FloatRect target;   // What it points at (screen); empty: no frame, no arrow
        bool isBubbleAbove;     // The bubble above the target (arrow down), else below (arrow up)
    };

    void go_to(Step step);
    Hint hint_of(Step step) const;

    std::weak_ptr<Stage> m_stage;
    std::weak_ptr<UICashUp> m_cashUp;
    Step m_step = Step::WALLET;
    float m_stepTime = 0.f;          // Seconds in this step
    float m_waitTime = 0.f;          // Seconds waiting for the cash up to be affordable (step 3 starts when it is)
    int m_cashUpLevel = 1;
    sf::Text m_text;
};

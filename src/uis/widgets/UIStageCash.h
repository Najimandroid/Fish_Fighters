#pragma once

#include "UITextElement.h"

class Stage;

/*
 * UIStageCash
 * -----------
 * The battle wallet, top right: a dark plate with the cash (big, yellow) and the wallet size (small),
 * a bar showing how full the wallet is, and a pulsing "FULL" tag when it can't hold more (time to spend
 * or to Cash Up).
 */
class UIStageCash : public UITextElement
{
public:
    UIStageCash(std::shared_ptr<Stage> stage);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;

private:
    std::weak_ptr<Stage> m_stage; // Weak pointer to avoid ownership
    sf::Text m_maxText;           // "/ 500$"
    sf::Text m_fullText;          // "FULL"
    float m_fill = 0.f;           // 0 to 1
    float m_time = 0.f;           // The FULL pulse
};

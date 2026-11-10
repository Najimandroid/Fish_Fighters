#pragma once

#include "UIElement.h"

#include <memory>

class Stage;

/*
 * UICashPopups
 * ------------
 * "+50$" in yellow outlined digits where an enemy died and dropped cash (Stage::get_cash_popups),
 * rising and fading out. Drawn in world space, over the battlefield.
 */
class UICashPopups : public UIElement
{
public:
    explicit UICashPopups(std::shared_ptr<Stage> stage);

    void update(float deltaTime) override {}
    void render(sf::RenderWindow& window) override;

    // Nothing to click on
    sf::FloatRect get_bounds() const override { return {}; }

private:
    std::weak_ptr<Stage> m_stage;
};

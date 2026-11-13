#pragma once

#include "UIElement.h"

#include <memory>

class DataLoader;

/*
 * UIShoalPower
 * ------------
 * The Fish Tank's Shoal Power panel (top right, under the title bar): every fish's level and extra levels added up
 * (PlayerData::get_shoal_power), and a bar to its next milestone with the Shiny Scales it gives.
 */
class UIShoalPower : public UIElement
{
public:
    explicit UIShoalPower(std::shared_ptr<DataLoader> dataLoader);

    void update(float deltaTime) override {}
    void render(sf::RenderWindow& window) override;

private:
    std::shared_ptr<DataLoader> m_dataLoader;
};

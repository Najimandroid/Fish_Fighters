#pragma once

#include "UIElement.h"

#include <memory>

class DataLoader;

/*
 * UISchoolRank
 * ------------
 * The Fish Tank's School Rank panel (top right, under the title bar): every fish's level and extra levels added up
 * (PlayerData::get_school_rank), and a bar to its next milestone with the Shiny Scales it gives.
 */
class UISchoolRank : public UIElement
{
public:
    explicit UISchoolRank(std::shared_ptr<DataLoader> dataLoader);

    void update(float deltaTime) override {}
    void render(sf::RenderWindow& window) override;

private:
    std::shared_ptr<DataLoader> m_dataLoader;
};

#pragma once

#include "UIElement.h"

#include <functional>
#include <memory>

class DataLoader;

/*
 * UIEnergyCost
 * ------------
 * What a stage costs to play (StageData::energyCost), on a tag straddling the top edge of the chapter map's BATTLE!:
 * the lightning icon and the cost, red while there isn't that much energy, FREE for a stage that costs nothing.
 * It follows the stage selected (stageUid is asked each frame).
 *
 * Example usage:
 *   auto cost = std::make_shared<UIEnergyCost>(dataLoader, [map]() { return map->get_selected_stage_uid(); }, center);
 */
class UIEnergyCost : public UIElement
{
public:
    UIEnergyCost(std::shared_ptr<DataLoader> dataLoader, std::function<int()> stageUid, sf::Vector2f center);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;

private:
    std::shared_ptr<DataLoader> m_dataLoader;
    std::function<int()> m_stageUid;
    sf::Vector2f m_center;
    sf::Text m_text;
    bool m_isFree = false;
};

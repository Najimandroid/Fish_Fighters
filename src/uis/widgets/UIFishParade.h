#pragma once

#include "UIElement.h"

#include <memory>
#include <vector>

class DataLoader;
struct EntityData;

/*
 * UIFishParade
 * ------------
 * The title screen's live key art: the fish cast marches along the floor of the submarine, right to left like
 * in battle, with their walking frames; each one comes back on the right once it has left the screen.
 *
 * Example usage:
 *   add_ui_element(background);
 *   add_ui_element(std::make_shared<UIFishParade>(dataLoader));   // Over the background, under the logo
 */
class UIFishParade : public UIElement
{
public:
    explicit UIFishParade(std::shared_ptr<DataLoader> dataLoader);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override { return {}; }

private:
    struct Marcher
    {
        std::shared_ptr<EntityData> data;
        const sf::Texture* texture = nullptr;
        sf::Vector2i frameSize;
        float scale = 1.f;
        float x = 0.f;              // Center of the frame
        float frameTime = 0.f;
        int frame = 0;
    };

    std::vector<Marcher> m_marchers;
    float m_loopWidth = 0.f;        // The parade's length: a fish leaving on the left comes back that far right
};

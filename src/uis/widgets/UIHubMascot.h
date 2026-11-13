#pragma once

#include "UIElement.h"

#include <memory>
#include <string>
#include <vector>

class DataLoader;

/*
 * UIHubMascot
 * -----------
 * The Fish keeps the Fish Tank company (like the Cat Base's chatty cat): it floats by the floor with its swim
 * frames and a shadow, and talks in a speech bubble, a letter at a time. The tips come first in the order given
 * (what's worth doing now: the clam is hungry, fish wait in the storage...), then the general ones, one after the
 * other every few seconds.
 * A tap on the fish makes it hop with a "blub" and say the next tip; a tap on the bubble shows the whole tip, or the
 * next one once it's all there.
 *
 * Example usage:
 *   add_ui_element(std::make_shared<UIHubMascot>(dataLoader, std::vector<std::string>{ "The clam looks hungry!" }));
 */
class UIHubMascot : public UIElement
{
public:
    UIHubMascot(std::shared_ptr<DataLoader> dataLoader, std::vector<std::string> tips);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;
    sf::FloatRect get_bounds() const override;   // The fish (the bubble is clickable too, see handle_event)

private:
    void next_tip();
    sf::FloatRect bubble_rect() const;
    sf::Vector2f fish_center() const;

    std::shared_ptr<DataLoader> m_dataLoader;
    std::vector<std::string> m_tips;
    int m_tipIndex = -1;
    sf::Text m_text;
    std::string m_wrapped;          // The tip cut into lines to fit the bubble
    float m_bubbleHeight = 0.f;     // For the whole tip (the bubble doesn't grow while it types)
    float m_tipTime = -0.9f;        // Seconds since this tip appeared (negative: waiting for it)
    float m_hopTime = -1.f;         // Seconds since the fish was tapped (-1: not hopping)
    float m_time = 0.f;
    bool m_isHovered = false;
};

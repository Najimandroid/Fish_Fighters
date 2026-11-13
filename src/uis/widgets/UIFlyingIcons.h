#pragma once

#include "UIElement.h"

#include <memory>
#include <random>
#include <string>
#include <vector>

class UIPlayerShellsInfo;

/*
 * UIFlyingIcons
 * -------------
 * Rewards flying into their counter: a few icons pop out of a place one after the other, swoop in an arc (sideways,
 * then up into the pill from below) and land in a UIPlayerShellsInfo pill, which counts them as they land
 * (hold_back now, receive on each landing: the icon pops). A tick on each landing, a sound for the last one.
 * Drawn over everything: add it after the title bar and the pills.
 */
class UIFlyingIcons : public UIElement
{
public:
    UIFlyingIcons();

    // count icons (texture: a UISkin name) carrying amount in all, from `from` into `pill`, after delay seconds
    void launch(const std::string& texture, sf::Vector2f from, std::shared_ptr<UIPlayerShellsInfo> pill, int amount,
        int count, float delay = 0.f, const std::string& lastSound = "shell_collect");

    // Every icon lands now (leaving the screen)
    void land_all();

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;

private:
    struct Icon
    {
        std::string texture;
        sf::Vector2f from;
        sf::Vector2f bend;          // The arc's control point, from (the pill's x, the start's y)
        std::weak_ptr<UIPlayerShellsInfo> pill;
        int amount = 0;
        int index = 0;
        float age = 0.f;            // Negative: not left yet
        float spin = 0.f;
        std::string landSound;      // The last one's
    };

    sf::Vector2f target_of(const Icon& icon) const;
    void land(Icon& icon, bool quietly);
    float random(float from, float to);

    std::vector<Icon> m_icons;
    std::mt19937 m_random{ std::random_device{}() };
};

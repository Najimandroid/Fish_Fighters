#pragma once

#include "UIElement.h"

#include <functional>

/*
 * UIScreenWipe
 * ------------
 * The way out of a battle, like The Battle Cats' wipe back to the base: a deep-sea band with zigzag edges sweeps in
 * from the right until it covers the screen (the battle still drawn under it), then onCovered runs (UIManager unloads
 * the battle and builds the menu, which the band uncovers by pulling away to the left). Takes the input meanwhile.
 *
 * Example usage:
 *   add_ui_element(std::make_shared<UIScreenWipe>([this]() { ... }));
 */
class UIScreenWipe : public UIElement
{
public:
    static constexpr float SECONDS = 0.45f;   // Each way

    explicit UIScreenWipe(std::function<void()> onCovered);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    bool is_blocking_input() const override { return true; }
    bool dims_letterbox() const override { return false; }
    sf::FloatRect get_bounds() const override { return { { 0.f, 0.f }, { 1920.f, 1080.f } }; }

    // How far a wipe's moving edge has gone (0 to 1) after t seconds: fast, then easing in
    static float progress(float t);

private:
    std::function<void()> m_onCovered;
    float m_time = 0.f;
    bool m_hasCovered = false;
};

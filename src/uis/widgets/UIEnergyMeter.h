#pragma once

#include "UIElement.h"

#include <memory>
#include <string>

class DataLoader;
class UIButtonElement;

/*
 * UIEnergyMeter
 * -------------
 * The energy on a menu's title bar (the Fish Tank, the chapter map), left of the shells, like The Battle Cats' energy
 * on its stage select: a dark pill with the lightning icon, "45/60", the time until the next point comes back, and a
 * gauge. Points coming back pop the icon; a refill rolls the digits up. Its "+" (the refill window) is a button the
 * UIManager adds at the pill's right end: the meter greys it out while the energy is full.
 * Part of the menu's frame (it stays still while the menus change under it).
 *
 * Example usage:
 *   auto meter = std::make_shared<UIEnergyMeter>(dataLoader);
 *   meter->set_refill_button(plusButton);
 */
class UIEnergyMeter : public UIElement
{
public:
    explicit UIEnergyMeter(std::shared_ptr<DataLoader> dataLoader);

    // The "+" drawn inside the pill's right end (enabled only while the energy isn't full)
    void set_refill_button(const std::shared_ptr<UIButtonElement>& button);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;

    // Where the "+" goes: its top left for a size x size button
    static sf::Vector2f refill_button_position(float size);

    // A wait as the meter shows it: "0:42", "12:05", "1:02:30"
    static std::string format_time(int seconds);

    static constexpr float RIGHT_OFFSET = 380.f;   // Its right edge this far from the layout's right edge (the shells' pill is right of it)
    static constexpr float WIDTH = 410.f;

private:
    std::shared_ptr<DataLoader> m_dataLoader;
    std::weak_ptr<UIButtonElement> m_refillButton;
    sf::Text m_digits;
    sf::Text m_timer;
    float m_shown = -1.f;     // The amount the digits show (rolls up to the real one after a refill; -1: not yet)
    int m_lastEnergy = -1;    // To see a point coming back
    float m_pop = 0.f;        // 1 when a point comes back (the icon pops), back to 0
    float m_fraction = 0.f;   // How full the gauge is drawn
    bool m_isTimerShown = true;
};

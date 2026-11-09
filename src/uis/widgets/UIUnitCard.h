#pragma once

#include "UITextureElement.h"
#include "tweeny.h"

/*
 * UIUnitCard
 * ----------
 * A unit card of the Upgrade and Equip carousels, like The Battle Cats' upgrade cards:
 * a dark card with the unit's name on top, its icon in a light blue slot, and two info lines
 * under it (level on the left, a cost on the right). The selected card has a yellow border,
 * the others are shaded. Its position is its bottom left corner (cards grow upward), and the
 * carousels slide and resize it with the tweens.
 *
 * UIUpgradeIcon and UIEquipIcon only choose what the info lines say.
 */

class UIUnitCard : public UITextureElement
{
public:

    static constexpr float HEIGHT_RATIO = 1.11f;  // Card height compared to its width

    // iconTexturePath: the unit's icon (the placeholder if missing)
    UIUnitCard(const std::string& iconTexturePath);

    // The selected card is bigger (set by the carousel), bordered in yellow and not shaded
    void select(bool isSelected);
    bool is_selected() const;

    void set_unit_name(const std::string& name);

    // Only the width counts: the height follows HEIGHT_RATIO
    void set_size(sf::Vector2f size) override;

    // Plays the tweens
    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;

    // The whole card (its position is the bottom left corner)
    sf::FloatRect get_bounds() const override;

public:

    tweeny::tween<float> tweenX;       // Horizontal movement tween
    tweeny::tween<float> tweenWidth;   // Width tween
    tweeny::tween<float> tweenHeight;  // Height tween (follows the width)

protected:

    // Left info line ("LEVEL 5")
    void set_level_line(const std::string& text, sf::Color color);

    // Right info line, after a shell icon when it's a price in shells
    void set_value_line(const std::string& text, sf::Color color, bool showsShell);

    // A unit not unlocked yet is a black silhouette
    void set_locked(bool isLocked);

    // Small red label over the icon ("IN USE"), empty = none
    void set_badge(const std::string& text);

private:

    bool m_isSelected = false;
    bool m_isLocked = false;
    bool m_showsShell = false;

    sf::Text m_name;
    sf::Text m_level;
    sf::Text m_value;
    sf::Text m_badge;
};

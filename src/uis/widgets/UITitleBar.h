#pragma once

#include "UIElement.h"

/*
 * UITitleBar
 * ----------
 * The riveted steel bar across the top of the menus, with the screen's title on the left
 * in big cream outlined letters, like the header of The Battle Cats' Cat Base and Upgrade screens.
 * The shells counter (UIPlayerShellsInfo) sits on its right end.
 *
 * Example usage:
 *   auto title = std::make_shared<UITitleBar>("Upgrade");
 */

class UITitleBar : public UIElement
{
public:

    static constexpr float HEIGHT = 112.f;

    // The band's material follows the screen's place: steel in the submarine, violet in the Clam Room, mother-of-pearl
    // inside the clam (the Clam Storage)
    enum class Material { STEEL, VIOLET, NACRE };

    UITitleBar(const std::string& title, Material material = Material::STEEL);

    void set_title(const std::string& title);

    // Where the screen's "i" goes: right of the title (UIInfoButton's center)
    sf::Vector2f get_info_position() const;

    void update(float deltaTime) override {}
    void render(sf::RenderWindow& window) override;

private:

    sf::Text m_title;
    Material m_material = Material::STEEL;
};

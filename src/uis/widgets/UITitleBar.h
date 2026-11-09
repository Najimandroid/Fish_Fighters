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

    UITitleBar(const std::string& title);

    void set_title(const std::string& title);

    void update(float deltaTime) override {}
    void render(sf::RenderWindow& window) override;

private:

    sf::Text m_title;
};

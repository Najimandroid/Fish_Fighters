#pragma once

#include "UIElement.h"
#include "UITitleBar.h"

/*
 * UIBottomBar
 * -----------
 * The band across the bottom of the menus, like The Battle Cats' bottom bar: the same material as the screen's title
 * bar, from UISkin::CONTENT_BOTTOM to the screen's bottom edge. The round back button sits on its left end; the
 * screen's shortcuts (Equip <-> Upgrade), the future shop buttons and the Shiny Scales (UIPlayerShellsInfo placed in
 * it) go on it. Add it after the screen's content and before the buttons on it.
 *
 * Example usage:
 *   add_ui_element(std::make_shared<UIBottomBar>());
 */
class UIBottomBar : public UIElement
{
public:

    // hasBackButton: the back button sits on its left end (no rivet there)
    explicit UIBottomBar(UITitleBar::Material material = UITitleBar::Material::STEEL, bool hasBackButton = true);

    void update(float deltaTime) override {}
    void render(sf::RenderWindow& window) override;

private:

    UITitleBar::Material m_material;
    bool m_hasBackButton;
};

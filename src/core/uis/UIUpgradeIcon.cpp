#include "UIUpgradeIcon.h"

#include <iostream>

UIUpgradeIcon::UIUpgradeIcon(const std::string& iconTexturePath):
	UITextureElement({ 400.f, 400.f }, {0.f, 0.f}, "assets/images/textures/icons/placeholder.png"),
	m_unitName(m_font), m_unitCurrentLevel(m_font), m_upgradeCost(m_font)
{

    //load icon texture
    if (!m_texture.loadFromFile(iconTexturePath))
    {
        std::cerr << "Failed to load icon: " << iconTexturePath << ", using placeholder.png\n";
        m_texture.loadFromFile("assets/images/textures/icons/placeholder.png");
    }
    m_sprite.setTexture(m_texture);
    apply_scale();

    //font
    if (!m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf"))
    {
        std::cerr << "Failed to load font assets/arial.ttf\n";
    }

    //shape
    m_shape.setSize(m_size);
    m_shape.setPosition(m_position);
    m_shape.setFillColor(sf::Color::White);

    m_blackFilter.setSize(m_size);
    m_blackFilter.setPosition(m_position);
    m_blackFilter.setFillColor(sf::Color(0, 0, 0, 175));

    //texts
    m_unitName.setString("Unit Name");
    m_unitName.setCharacterSize(40);
    m_unitName.setFillColor(sf::Color::White);
    m_unitName.setOutlineColor(sf::Color::Black);
    m_unitName.setOutlineThickness(2.f);
    m_unitName.setPosition({ m_position.x, m_position.y - 30.f });

    m_unitCurrentLevel.setString("Level: -1");
    m_unitCurrentLevel.setCharacterSize(32);
    m_unitCurrentLevel.setFillColor(sf::Color::White);
	m_unitCurrentLevel.setOutlineColor(sf::Color::Black);
	m_unitCurrentLevel.setOutlineThickness(2.f);
    m_unitCurrentLevel.setPosition({ m_position.x + m_size.x + 10.f, m_position.y + m_size.y - 30.f });

    m_upgradeCost.setString("100$");
    m_upgradeCost.setCharacterSize(32);
    m_upgradeCost.setFillColor(sf::Color::White);
    m_upgradeCost.setOutlineColor(sf::Color::Black);
    m_upgradeCost.setOutlineThickness(2.f);
    m_upgradeCost.setPosition({ m_position.x + m_size.x + 10.f, m_position.y + m_size.y });
}

int UIUpgradeIcon::get_uid() const
{
    return m_uid;
}

void UIUpgradeIcon::select(bool isSelected)
{
    m_isSelected = isSelected;
    m_shape.setOutlineThickness(isSelected ? 3.f : 0.f);
    m_shape.setOutlineColor(sf::Color::Yellow);


}

void UIUpgradeIcon::set_unit_name(const std::string& name)
{
    m_unitName.setString(name);
}

void UIUpgradeIcon::set_unit_level(const std::string& level)
{
    m_unitCurrentLevel.setString(level);
}

void UIUpgradeIcon::set_upgrade_cost(const std::string& cost)
{
    m_upgradeCost.setString(cost);
}

void UIUpgradeIcon::update(float deltaTime)
{
    // Tween position X
    if (tweenX.progress() < 1.0f)
        m_position.x = tweenX.step(1);

    // Tween size
    if (tweenWidth.progress() < 1.0f || tweenHeight.progress() < 1.0f)
        set_size({ tweenWidth.step(1), tweenHeight.step(1) });

    sf::Vector2f anchorPos = { m_position.x, m_position.y - m_size.y };

    m_shape.setSize(m_size);
    m_shape.setPosition(anchorPos);

    m_blackFilter.setSize(m_size);
    m_blackFilter.setPosition(anchorPos);

    apply_scale();

    auto texSize = m_texture.getSize();
    float scaledHeight = texSize.y * m_sprite.getScale().y;

    m_sprite.setPosition({ anchorPos.x, anchorPos.y + m_size.y - scaledHeight });

    auto boundsName = m_unitName.getLocalBounds();
    m_unitName.setOrigin({
        boundsName.position.x + boundsName.size.x / 2.f,
        boundsName.position.y
    });

    m_unitName.setPosition({ anchorPos.x + m_size.x / 2.f, anchorPos.y - 40.f });


    m_unitCurrentLevel.setPosition({
        anchorPos.x + m_size.x + 10.f,
        anchorPos.y + m_size.y - 30.f
    });


    m_upgradeCost.setPosition({
        anchorPos.x + m_size.x + 10.f,
        anchorPos.y + m_size.y
    });
}


void UIUpgradeIcon::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    window.draw(m_shape);
    window.draw(m_sprite);
    window.draw(m_unitName);
    window.draw(m_unitCurrentLevel);
    window.draw(m_upgradeCost);
    if (!m_isSelected)
        window.draw(m_blackFilter);
}
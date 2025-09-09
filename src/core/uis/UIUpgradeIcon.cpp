#include "UIUpgradeIcon.h"

#include <iostream>

UIUpgradeIcon::UIUpgradeIcon(const std::string& iconTexturePath):
	UITextureElement({ 400.f, 400.f }, {0.f, 0.f}, "assets/images/textures/icons/placeholder.png"),
	m_unitName(m_font), m_currentLevelText(m_font), m_upgradeCostText(m_font)
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
    m_unitName.setCharacterSize(TITLE_BASE_TEXT_SIZE);
    m_unitName.setFillColor(sf::Color::White);
    m_unitName.setOutlineColor(sf::Color::Black);
    m_unitName.setOutlineThickness(2.f);
    m_unitName.setPosition({ m_position.x, m_position.y - 30.f });

    m_currentLevelText.setString("Level: -1");
    m_currentLevelText.setCharacterSize(INFO_BASE_TEXT_SIZE);
    m_currentLevelText.setFillColor(sf::Color::Green);
    m_currentLevelText.setOutlineColor(sf::Color::Black);
    m_currentLevelText.setOutlineThickness(2.f);
    m_currentLevelText.setPosition({ m_position.x + m_size.x + 10.f, m_position.y + m_size.y - 30.f });

    m_upgradeCostText.setString("100$");
    m_upgradeCostText.setCharacterSize(INFO_BASE_TEXT_SIZE);
    m_upgradeCostText.setFillColor(sf::Color::Cyan);
    m_upgradeCostText.setOutlineColor(sf::Color::Black);
    m_upgradeCostText.setOutlineThickness(2.f);
    m_upgradeCostText.setPosition({ m_position.x + m_size.x + 10.f, m_position.y + m_size.y });
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

	m_unitName.setCharacterSize(isSelected ? TITLE_BASE_TEXT_SIZE * ACTIVE_TEXT_SIZE_SCALE : TITLE_BASE_TEXT_SIZE * INACTIVE_TEXT_SIZE_SCALE);
	m_currentLevelText.setCharacterSize(isSelected ? INFO_BASE_TEXT_SIZE * ACTIVE_TEXT_SIZE_SCALE : INFO_BASE_TEXT_SIZE * INACTIVE_TEXT_SIZE_SCALE);
	m_upgradeCostText.setCharacterSize(isSelected ? INFO_BASE_TEXT_SIZE * ACTIVE_TEXT_SIZE_SCALE : INFO_BASE_TEXT_SIZE * INACTIVE_TEXT_SIZE_SCALE);
}

void UIUpgradeIcon::set_unit_name(const std::string& name)
{
    m_unitName.setString(name);
}

void UIUpgradeIcon::set_unit_level(int level)
{
	m_currentLevel = level;
}

void UIUpgradeIcon::set_upgrade_cost(int cost)
{
    m_upgradeCost = cost;
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
    float offsetYTitle = m_isSelected ? TITLE_BASE_TEXT_SIZE * ACTIVE_TEXT_SIZE_SCALE : TITLE_BASE_TEXT_SIZE * INACTIVE_TEXT_SIZE_SCALE;
    m_unitName.setOrigin({
        boundsName.position.x + boundsName.size.x / 2.f,
        boundsName.position.y + offsetYTitle / 4.f
    });

    m_unitName.setPosition({ anchorPos.x + m_size.x / 2.f, anchorPos.y - 40.f });

    auto spriteBounds = m_sprite.getGlobalBounds();
    float offsetY = m_isSelected ? INFO_BASE_TEXT_SIZE * ACTIVE_TEXT_SIZE_SCALE : INFO_BASE_TEXT_SIZE * INACTIVE_TEXT_SIZE_SCALE;
	offsetY += 5.f; //small padding
    float offsetX = 15.f;

	auto levelBounds = m_currentLevelText.getLocalBounds();
    m_currentLevelText.setString((m_currentLevel > 0) ? "Lv. " + std::to_string(m_currentLevel) : "UNLOCK");
    m_currentLevelText.setOrigin({ levelBounds.size.x, 0.f });
    m_currentLevelText.setPosition({
        spriteBounds.position.x + spriteBounds.size.x - offsetX,
        spriteBounds.position.y + spriteBounds.size.y - offsetY * 2.f
    });

	auto costBounds = m_upgradeCostText.getLocalBounds();
    m_upgradeCostText.setString("Req. " + std::to_string(m_upgradeCost) + "¤");
    m_upgradeCostText.setOrigin({ costBounds.size.x, 0.f });
    m_upgradeCostText.setPosition({
        spriteBounds.position.x + spriteBounds.size.x - offsetX,
        spriteBounds.position.y + spriteBounds.size.y - offsetY
    });
}


void UIUpgradeIcon::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    window.draw(m_shape);
    window.draw(m_sprite);
    window.draw(m_unitName);
    window.draw(m_currentLevelText);
    window.draw(m_upgradeCostText);
    if (!m_isSelected)
        window.draw(m_blackFilter);
}
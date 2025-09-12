#include "UIUpgradeIcon.h"
#include <iostream>

/*
 * Constructor: UIUpgradeIcon
 * --------------------------
 * Initializes the upgrade icon with the given texture path.
 * Sets up default shapes, text elements, and loads the texture and font.
 */
UIUpgradeIcon::UIUpgradeIcon(const std::string& iconTexturePath) :
    UITextureElement({ 400.f, 300.f }, { 0.f, 0.f }, "assets/images/textures/icons/placeholder.png"), // base texture element
    m_unitName(m_font), m_currentLevelText(m_font), m_upgradeCostText(m_font) // initialize text objects with font
{
    // Load the icon texture, fallback to placeholder if failed
    if (!m_texture.loadFromFile(iconTexturePath))
    {
        std::cerr << "Failed to load icon: " << iconTexturePath << ", using placeholder.png\n";
        m_texture.loadFromFile("assets/images/textures/icons/placeholder.png");
    }
    m_sprite.setTexture(m_texture);
    apply_scale(); // scale sprite according to m_size

    // Load font for text
    if (!m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf"))
        std::cerr << "Failed to load font assets/arial.ttf\n";

    // Configure main shape (background for icon)
    m_shape.setSize(m_size);
    m_shape.setPosition(m_position);
    m_shape.setFillColor(sf::Color::White);

    // Configure black overlay for inactive/locked appearance
    m_blackFilter.setSize(m_size);
    m_blackFilter.setPosition(m_position);
    m_blackFilter.setFillColor(sf::Color(0, 0, 0, 175));

    // Configure unit name text
    m_unitName.setString("Unit Name");
    m_unitName.setCharacterSize(TITLE_BASE_TEXT_SIZE);
    m_unitName.setFillColor(sf::Color::White);
    m_unitName.setOutlineColor(sf::Color::Black);
    m_unitName.setOutlineThickness(2.f);
    m_unitName.setPosition({ m_position.x, m_position.y - 30.f });

    // Configure current level text
    m_currentLevelText.setString("Level: -1");
    m_currentLevelText.setCharacterSize(INFO_BASE_TEXT_SIZE);
    m_currentLevelText.setFillColor(sf::Color::Green);
    m_currentLevelText.setOutlineColor(sf::Color::Black);
    m_currentLevelText.setOutlineThickness(2.f);
    m_currentLevelText.setPosition({ m_position.x + m_size.x + 10.f, m_position.y + m_size.y - 30.f });

    // Configure upgrade cost text
    m_upgradeCostText.setString("100$");
    m_upgradeCostText.setCharacterSize(INFO_BASE_TEXT_SIZE);
    m_upgradeCostText.setFillColor(sf::Color::Cyan);
    m_upgradeCostText.setOutlineColor(sf::Color::Black);
    m_upgradeCostText.setOutlineThickness(2.f);
    m_upgradeCostText.setPosition({ m_position.x + m_size.x + 10.f, m_position.y + m_size.y });
}

/*
 * Returns the unique ID of the unit this icon represents.
 */
int UIUpgradeIcon::get_uid() const
{
    return m_uid;
}

/*
 * Updates selection state, outline thickness, and text size based on selection.
 */
void UIUpgradeIcon::select(bool isSelected)
{
    m_isSelected = isSelected;
    m_shape.setOutlineThickness(isSelected ? 3.f : 0.f);
    m_shape.setOutlineColor(sf::Color::Yellow);

    // Scale texts based on selection
    m_unitName.setCharacterSize(isSelected ? TITLE_BASE_TEXT_SIZE * ACTIVE_TEXT_SIZE_SCALE : TITLE_BASE_TEXT_SIZE * INACTIVE_TEXT_SIZE_SCALE);
    m_currentLevelText.setCharacterSize(isSelected ? INFO_BASE_TEXT_SIZE * ACTIVE_TEXT_SIZE_SCALE : INFO_BASE_TEXT_SIZE * INACTIVE_TEXT_SIZE_SCALE);
    m_upgradeCostText.setCharacterSize(isSelected ? INFO_BASE_TEXT_SIZE * ACTIVE_TEXT_SIZE_SCALE : INFO_BASE_TEXT_SIZE * INACTIVE_TEXT_SIZE_SCALE);
}

/*
 * Sets the display name of the unit.
 */
void UIUpgradeIcon::set_unit_name(const std::string& name)
{
    m_unitName.setString(name);
}

/*
 * Sets the current level of the unit.
 */
void UIUpgradeIcon::set_unit_level(int level)
{
    m_currentLevel = level;
}

/*
 * Sets the current upgrade cost of the unit.
 */
void UIUpgradeIcon::set_upgrade_cost(int cost)
{
    m_upgradeCost = cost;
}

/*
 * Sets the size of the icon while maintaining a 4:3 aspect ratio.
 * Updates the shapes and calls apply_scale to adjust sprite accordingly.
 */
void UIUpgradeIcon::set_size(sf::Vector2f size)
{
    float width = size.x;
    float height = width * (3.f / 4.f); // enforce 4:3 ratio

    m_size = { width, height };
    m_shape.setSize(m_size);
    m_blackFilter.setSize(m_size);

    apply_scale();
}

/*
 * Updates the icon every frame.
 * Handles tweening of position and size.
 * Updates positions of sprite and text labels based on selection.
 */
void UIUpgradeIcon::update(float deltaTime)
{
    // Tween horizontal position
    if (tweenX.progress() < 1.0f)
        m_position.x = tweenX.step(1);

    // Tween size
    if (tweenWidth.progress() < 1.0f || tweenHeight.progress() < 1.0f)
        set_size({ tweenWidth.step(1), tweenHeight.step(1) });

    // Anchor position at bottom-left of icon
    sf::Vector2f anchorPos = { m_position.x, m_position.y - m_size.y };

    m_shape.setSize(m_size);
    m_shape.setPosition(anchorPos);
    m_blackFilter.setSize(m_size);
    m_blackFilter.setPosition(anchorPos);

    apply_scale();

    // Center sprite within icon
    sf::FloatRect bounds = m_sprite.getGlobalBounds();
    sf::Vector2f spritePos = anchorPos;
    spritePos.y += (m_size.y - bounds.size.y) / 2.f;
    m_sprite.setPosition(spritePos);

    // Position unit name text above icon
    auto boundsName = m_unitName.getLocalBounds();
    float offsetYTitle = m_isSelected ? TITLE_BASE_TEXT_SIZE * ACTIVE_TEXT_SIZE_SCALE : TITLE_BASE_TEXT_SIZE * INACTIVE_TEXT_SIZE_SCALE;
    m_unitName.setOrigin({
        boundsName.position.x + boundsName.size.x / 2.f,
        boundsName.position.y + offsetYTitle / 4.f
        });
    m_unitName.setPosition({ anchorPos.x + m_size.x / 2.f, anchorPos.y - 40.f });

    // Position level text at bottom-right
    float offsetY = (m_isSelected ? INFO_BASE_TEXT_SIZE * ACTIVE_TEXT_SIZE_SCALE : INFO_BASE_TEXT_SIZE * INACTIVE_TEXT_SIZE_SCALE) + 5.f;
    float offsetX = 15.f;
    auto levelBounds = m_currentLevelText.getLocalBounds();
    m_currentLevelText.setString((m_currentLevel > 0) ? "Lv. " + std::to_string(m_currentLevel) : "UNLOCK");
    m_currentLevelText.setOrigin({ levelBounds.size.x, 0.f });
    m_currentLevelText.setPosition({
        anchorPos.x + m_size.x - offsetX,
        anchorPos.y + m_size.y - offsetY * 2.f
        });

    // Position upgrade cost text below level text
    auto costBounds = m_upgradeCostText.getLocalBounds();
    m_upgradeCostText.setString("Req. " + std::to_string(m_upgradeCost) + "¤");
    m_upgradeCostText.setOrigin({ costBounds.size.x, 0.f });
    m_upgradeCostText.setPosition({
        anchorPos.x + m_size.x - offsetX,
        anchorPos.y + m_size.y - offsetY
        });
}

/*
 * Draws the icon, sprite, text labels, and black overlay if not selected.
 */
void UIUpgradeIcon::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    window.draw(m_shape);
    window.draw(m_sprite);
    window.draw(m_unitName);
    window.draw(m_currentLevelText);
    window.draw(m_upgradeCostText);

    // Draw black overlay if icon is not selected
    if (!m_isSelected)
        window.draw(m_blackFilter);
}

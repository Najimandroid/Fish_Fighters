#include "UIEquipIcon.h"

#include <iostream>

/*
 * Constructor
 * -----------
 * - Loads the icon texture (falls back to placeholder if missing)
 * - Loads the font for text rendering
 * - Initializes shape, black overlay, and text fields
 */
UIEquipIcon::UIEquipIcon(const std::string& iconTexturePath, int uid, int cost) :
    UITextureElement({ 400.f, 300.f }, { 0.f, 0.f }, "assets/images/textures/icons/placeholder.png"),
    m_uid(uid),
    m_texturePath(iconTexturePath),
    m_currentLevelText(m_font), m_deploymentCostText(m_font), m_unitName(m_font)
{
    // Load icon texture
    if (!m_texture.loadFromFile(iconTexturePath))
    {
        std::cerr << "Failed to load icon: " << iconTexturePath << ", using placeholder.png\n";
        m_texture.loadFromFile("assets/images/textures/icons/placeholder.png");
    }
    m_sprite.setTexture(m_texture);
    apply_scale();

    // Load font
    if (!m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf"))
    {
        std::cerr << "Failed to load font assets/MPLUSRounded1c-Medium.ttf\n";
    }

    // Base rectangle (background of the icon)
    m_shape.setSize(m_size);
    m_shape.setPosition(m_position);
    m_shape.setFillColor(sf::Color::White);

    // Semi-transparent dark overlay (used when not selected)
    m_blackFilter.setSize(m_size);
    m_blackFilter.setPosition(m_position);
    m_blackFilter.setFillColor(sf::Color(0, 0, 0, 175));

    // Default text values
    m_unitName.setString("Unit Name");
    m_unitName.setCharacterSize(TITLE_BASE_TEXT_SIZE);
    m_unitName.setFillColor(sf::Color::White);
    m_unitName.setOutlineColor(sf::Color::Black);
    m_unitName.setOutlineThickness(2.f);

    m_currentLevelText.setString("Level: -1");
    m_currentLevelText.setCharacterSize(INFO_BASE_TEXT_SIZE);
    m_currentLevelText.setFillColor(sf::Color::Green);
    m_currentLevelText.setOutlineColor(sf::Color::Black);
    m_currentLevelText.setOutlineThickness(2.f);

    m_deploymentCostText.setString("100$");
    m_deploymentCostText.setCharacterSize(INFO_BASE_TEXT_SIZE);
    m_deploymentCostText.setFillColor(sf::Color::Yellow);
    m_deploymentCostText.setOutlineColor(sf::Color::Black);
    m_deploymentCostText.setOutlineThickness(2.f);
}

/*
 * get_uid
 * -------
 * Returns the unique ID of the unit represented by this icon.
 */
int UIEquipIcon::get_uid() const
{
    return m_uid;
}

/*
 * select
 * ------
 * - Marks the icon as selected/deselected
 * - Changes outline and text scaling depending on state
 */
void UIEquipIcon::select(bool isSelected)
{
    m_isSelected = isSelected;
    m_shape.setOutlineThickness(isSelected ? 3.f : 0.f);
    m_shape.setOutlineColor(sf::Color::Yellow);

    // Larger text when selected, smaller when not
    m_unitName.setCharacterSize(isSelected ? TITLE_BASE_TEXT_SIZE * ACTIVE_TEXT_SIZE_SCALE : TITLE_BASE_TEXT_SIZE * INACTIVE_TEXT_SIZE_SCALE);
    m_currentLevelText.setCharacterSize(isSelected ? INFO_BASE_TEXT_SIZE * ACTIVE_TEXT_SIZE_SCALE : INFO_BASE_TEXT_SIZE * INACTIVE_TEXT_SIZE_SCALE);
    m_deploymentCostText.setCharacterSize(isSelected ? INFO_BASE_TEXT_SIZE * ACTIVE_TEXT_SIZE_SCALE : INFO_BASE_TEXT_SIZE * INACTIVE_TEXT_SIZE_SCALE);
}

/*
 * set_unit_name
 * -------------
 * Updates the displayed unit name.
 */
void UIEquipIcon::set_unit_name(const std::string& name)
{
    m_unitName.setString(name);
}

/*
 * set_unit_level
 * --------------
 * Updates the displayed unit level.
 */
void UIEquipIcon::set_unit_level(int level)
{
    m_currentLevel = level;
}

/*
 * set_deployment_cost
 * -------------------
 * Updates the displayed unit cost.
 */
void UIEquipIcon::set_deployment_cost(int cost)
{
    m_deploymentCost = cost;
}

/*
 * set_size
 * --------
 * - Resizes the icon, keeping a fixed 4:3 aspect ratio
 * - Updates the background shape and filter sizes
 */
void UIEquipIcon::set_size(sf::Vector2f size)
{
    float width = size.x;
    float height = width * (3.f / 4.f); // enforce 4:3 ratio

    m_size = { width, height };
    m_shape.setSize(m_size);
    m_blackFilter.setSize(m_size);

    apply_scale();
}

/*
 * update
 * ------
 * - Advances tween animations for position/size
 * - Updates text and sprite layout relative to current size and position
 */
void UIEquipIcon::update(float deltaTime)
{
    // Tween X position
    if (tweenX.progress() < 1.0f)
        m_position.x = tweenX.step(1);

    // Tween width/height
    if (tweenWidth.progress() < 1.0f || tweenHeight.progress() < 1.0f)
        set_size({ tweenWidth.step(1), tweenHeight.step(1) });

    // Anchor above base Y position
    sf::Vector2f anchorPos = { m_position.x, m_position.y - m_size.y };

    m_shape.setSize(m_size);
    m_shape.setPosition(anchorPos);

    m_blackFilter.setSize(m_size);
    m_blackFilter.setPosition(anchorPos);

    apply_scale();

    // Center sprite inside the shape
    sf::FloatRect bounds = m_sprite.getGlobalBounds();
    sf::Vector2f spritePos = anchorPos;
    spritePos.y += (m_size.y - bounds.size.y) / 2.f;
    m_sprite.setPosition(spritePos);

    // Unit name (title)
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
    m_currentLevelText.setString("Lv. " + std::to_string(m_currentLevel));
    m_currentLevelText.setOrigin({ levelBounds.size.x, 0.f });
    m_currentLevelText.setPosition({
        anchorPos.x + m_size.x - offsetX,
        anchorPos.y + m_size.y - offsetY * 2.f
        });

    // Position upgrade cost text below level text
    auto costBounds = m_deploymentCostText.getLocalBounds();
    m_deploymentCostText.setString("Cost: " + std::to_string(m_deploymentCost) + "$");
    m_deploymentCostText.setOrigin({ costBounds.size.x, 0.f });
    m_deploymentCostText.setPosition({
        anchorPos.x + m_size.x - offsetX,
        anchorPos.y + m_size.y - offsetY
        });
}

/*
 * render
 * ------
 * - Draws icon shape, sprite, and text to the window
 * - If not selected, draws a dark filter over the icon
 */
void UIEquipIcon::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    window.draw(m_shape);
    window.draw(m_sprite);
    window.draw(m_unitName);
    window.draw(m_currentLevelText);
    window.draw(m_deploymentCostText);

    if (!m_isSelected)
        window.draw(m_blackFilter);
}

/*
 * check_drag_start
 * ----------------
 * - Checks if left mouse button was pressed on this icon
 * - If so, fills DragInfo with unit data (uid, cost, texturePath)
 * - Returns true if drag should begin
 */
bool UIEquipIcon::check_drag_start(const sf::Event& event, const sf::RenderWindow& window, DragInfo& outDrag)
{
    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mousePressed->button == sf::Mouse::Button::Left &&
            get_bounds().contains(window.mapPixelToCoords(mousePressed->position)))
        {
            outDrag.uid = m_uid;
            outDrag.cost = m_deploymentCost;
            outDrag.texturePath = m_texturePath;
            return true;
        }
    }
    return false;
}

#include "UIEquipSlider.h"
#include "../core/DataLoader.h"

#include <algorithm>
#include <cmath>
#include <iostream>

/*
 * Constructor
 * -----------
 * - Loads font and placeholder texture
 * - Initializes the drag ghost object
 * - Calls init_icons() to build the equip UI
 */
UIEquipSlider::UIEquipSlider(std::shared_ptr<DataLoader> dataLoader)
    : m_dataLoader(std::move(dataLoader))
{
    if (!m_sharedFont.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf")) {
        std::cerr << "UIEquipSlider: failed to load font\n";
    }

    if (!m_placeholderTexture.loadFromFile("assets/images/textures/icons/placeholder.png")) {
        std::cerr << "UIEquipSlider: failed to load placeholder texture\n";
    }

    // Create temporary drag ghost
    m_tempGhost = std::make_unique<TempGhost>(m_sharedFont, m_placeholderTexture);

    // Initialize equip icons and slots
    init_icons();
}

/*
 * init_icons
 * ----------
 * - Clears old icons and slots
 * - Builds the slot grid (2 rows × 5 columns)
 * - Loads equipped units from player data into slots
 * - Creates the slider icons for all owned units
 */
void UIEquipSlider::init_icons()
{
    m_equipIcons.clear();
    m_availableEquips.clear();

    auto playerData = m_dataLoader->get_player_data().lock();
    if (!playerData) return;

    // Collect all owned unit IDs
    for (auto& kv : playerData->ownedUnits)
        m_availableEquips.push_back(kv.first);

    // Sort IDs for consistent order
    std::sort(m_availableEquips.begin(), m_availableEquips.end());

    // Center of the screen where slider icons are placed
    sf::Vector2f screenCenter(
        1920.f / 2.0f - baseIconSize.x / 2.f,
        1080.f / 2.0f - baseIconSize.y / 2.f + 300.f
    );

    // Grid configuration for equip slots
    const int rows = 2;
    const int cols = 5;
    const float iconWidth = 144.f * 1.15f;
    const float iconHeight = 108.f * 1.15f;
    const float spacingX = 24.f * 1.15f;
    const float spacingY = 24.f * 1.15f;

    // Calculate starting position for grid
    float totalWidth = cols * iconWidth + (cols - 1) * spacingX;
    float startX = (1920.f - totalWidth) / 2.f;
    float startY = 775.f;

    // Create slot rectangles and visuals
    int index = 0;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            sf::Vector2f pos{ startX + col * (iconWidth + spacingX), startY + row * (iconHeight + spacingY) };
            sf::Vector2f size{ iconWidth, iconHeight };

            m_slots[index].bounds = sf::FloatRect(pos, size);
            m_slots[index].ghost = nullptr;

            m_slots[index].background.setSize(size);
            m_slots[index].background.setPosition(pos);
            m_slots[index].background.setFillColor(sf::Color(0, 0, 0, 150));
            m_slots[index].background.setOutlineColor(sf::Color::White);
            m_slots[index].background.setOutlineThickness(2.f);

            index++;
        }
    }

    // Load currently equipped units into the slots
    if (auto player = m_dataLoader->get_player_data().lock()) {
        for (int i = 0; i < static_cast<int>(m_slots.size()); ++i) {
            int uid = player->equippedUnits[i];
            if (uid >= 0) {
                auto unitData = m_dataLoader->get_unit_data(uid);

                // Create UIEquipSlot for equipped unit
                auto slotGhost = std::make_shared<UIEquipSlot>(
                    unitData->UID,
                    unitData->cost,
                    m_dataLoader->get_unit_icon_texture_path(uid),
                    i
                );

                slotGhost->set_position({ m_slots[i].bounds.position.x, m_slots[i].bounds.position.y });
                slotGhost->set_size({ m_slots[i].bounds.size.x, m_slots[i].bounds.size.y });

                // Define unequip callback
                slotGhost->set_callback([this](int slotIndex) {
                    if (auto player = m_dataLoader->get_player_data().lock()) {
                        player->unequip_unit(slotIndex);
                        m_slots[slotIndex].ghost = nullptr;
                    }
                    });

                m_slots[i].ghost = slotGhost;
            }
        }
    }

    // Create slider icons for all available units
    for (int i = 0; i < static_cast<int>(m_availableEquips.size()); ++i)
    {
        int unitUid = m_availableEquips[i];
        int unitLevel = playerData->get_unit_level(unitUid);
        auto unitData = m_dataLoader->get_unit_data(unitUid);

        auto icon = std::make_shared<UIEquipIcon>(
            m_dataLoader->get_unit_icon_texture_path(unitUid),
            unitData->UID,
            unitData->cost
        );

        // Highlight the currently selected icon
        icon->select(i == m_currentIndex);
        icon->set_unit_name(unitData->name);
        icon->set_unit_level(unitLevel);
        icon->set_deployment_cost(unitData->cost);

        // Positioning logic: offset from center
        float xOffset = (i - m_currentIndex) * ICON_SPACING;
        float posX = screenCenter.x + xOffset;
        if (i == m_currentIndex)
            posX -= CENTER_OFFSET_X;
        icon->set_position({ posX, screenCenter.y });

        // Scaling: bigger for center icon
        float scale = (i == m_currentIndex) ? CENTER_SCALE : SIDE_SCALE;
        icon->set_size(baseIconSize * scale);

        // Initialize tweens for smooth transitions
        icon->tweenX = tweeny::from(icon->get_position().x).to(icon->get_position().x).during(1);
        icon->tweenWidth = tweeny::from(icon->get_size().x).to(icon->get_size().x).during(1);
        icon->tweenHeight = tweeny::from(icon->get_size().y).to(icon->get_size().y).during(1);

        m_equipIcons.push_back(icon);
    }
}

/*
 * update
 * ------
 * - Updates icons
 * - Handles hold-to-drag logic (long press to drag icon)
 * - Updates ghost scaling tween
 */
void UIEquipSlider::update(float deltaTime)
{
    for (auto& icon : m_equipIcons)
        icon->update(deltaTime);

    // Detect hold-to-drag
    if (m_mouseDown && !m_dragging)
    {
        if (m_holdClock.getElapsedTime().asMilliseconds() >= HOLD_THRESHOLD_MS)
        {
            if (!m_equipIcons.empty())
            {
                DragInfo dragInfo;
                int uid = m_equipIcons[m_currentIndex]->get_uid();
                dragInfo.uid = uid;
                dragInfo.cost = m_dataLoader->get_unit_data(uid)->cost;
                dragInfo.level = m_dataLoader->get_player_data().lock()->get_unit_level(uid);

                // Load texture into cache if needed
                auto it = m_unitTextures.find(uid);
                if (it == m_unitTextures.end())
                {
                    sf::Texture tex;
                    if (!tex.loadFromFile(m_dataLoader->get_unit_icon_texture_path(uid)))
                        tex = m_placeholderTexture;
                    auto [insertIt, _] = m_unitTextures.emplace(uid, std::move(tex));
                    dragInfo.texture = &insertIt->second;
                }
                else
                {
                    dragInfo.texture = &it->second;
                }

                dragInfo.iconSize = m_equipIcons[m_currentIndex]->get_size();

                start_drag(dragInfo, m_mouseDownPos);
                m_dragging = true;
            }
        }
    }

    if (!m_tempGhost || !m_tempGhost->active)
        return;

    // Advance ghost scaling tween one step per frame
    if (m_tempGhost->scaleTween.progress() < 1.0f)
    {
        m_tempGhost->currentScale = m_tempGhost->scaleTween.step(1);
        m_tempGhost->shape.setScale({ m_tempGhost->currentScale, m_tempGhost->currentScale });
        m_tempGhost->costText.setCharacterSize(m_tempGhost->INFO_BASE_TEXT_SIZE * m_tempGhost->currentScale);
        m_tempGhost->levelText.setCharacterSize(m_tempGhost->INFO_BASE_TEXT_SIZE * m_tempGhost->currentScale);

        // Adjust sprite to fit ghost shape
        sf::FloatRect spriteLocal = m_tempGhost->sprite.getLocalBounds();
        sf::Vector2f shapeBaseSize = m_tempGhost->shape.getSize();
        float effectiveW = shapeBaseSize.x * m_tempGhost->currentScale;
        float effectiveH = shapeBaseSize.y * m_tempGhost->currentScale;

        if (spriteLocal.size.x > 0.f && spriteLocal.size.y > 0.f)
        {
            float sx = effectiveW / spriteLocal.size.x;
            float sy = effectiveH / spriteLocal.size.y;
            m_tempGhost->sprite.setScale({ sx, sy });
        }
    }
    else
    {
        // If tween shrinks to zero, deactivate ghost
        float finalVal = m_tempGhost->scaleTween.peek(1.0f);
        if (std::abs(finalVal) < 1e-6f)
            m_tempGhost->active = false;
    }

    // Updating the ghost's costText and levelText positions
    if (m_tempGhost->active)
    {
        sf::FloatRect shapeBounds = m_tempGhost->shape.getGlobalBounds();

        sf::FloatRect costTextBounds = m_tempGhost->costText.getLocalBounds();
        sf::FloatRect levelTextBounds = m_tempGhost->levelText.getLocalBounds();

        // Cost Text
        m_tempGhost->costText.setOrigin({
            costTextBounds.position.x + costTextBounds.size.x,
            costTextBounds.position.y + costTextBounds.size.y
            });

        m_tempGhost->costText.setPosition({
            shapeBounds.position.x + shapeBounds.size.x - m_tempGhost->INFO_BASE_TEXT_OFFSET * m_tempGhost->currentScale,
            shapeBounds.position.y + shapeBounds.size.y - m_tempGhost->INFO_BASE_TEXT_OFFSET * m_tempGhost->currentScale
            });

        // Level Text
        m_tempGhost->levelText.setOrigin({
            levelTextBounds.position.x + levelTextBounds.size.x,
            levelTextBounds.position.y + levelTextBounds.size.y
            });

        m_tempGhost->levelText.setPosition({
            shapeBounds.position.x + shapeBounds.size.x - m_tempGhost->INFO_BASE_TEXT_OFFSET * m_tempGhost->currentScale,
            shapeBounds.position.y + shapeBounds.size.y - m_tempGhost->INFO_BASE_TEXT_OFFSET * m_tempGhost->currentScale - m_tempGhost->costText.getCharacterSize() - m_tempGhost->INFO_BASE_TEXT_OFFSET
            });

    }
}

/*
 * render
 * ------
 * - Draws icons, slot backgrounds, and equipped units
 * - If a ghost is active, draws it on top
 */
void UIEquipSlider::render(sf::RenderWindow& window)
{
    for (auto& icon : m_equipIcons)
        icon->render(window);

    for (auto& slot : m_slots)
    {
        window.draw(slot.background);
        if (slot.ghost)
            slot.ghost->render(window);
    }

    if (m_tempGhost && m_tempGhost->active)
    {
        window.draw(m_tempGhost->shape);
        window.draw(m_tempGhost->sprite);
        window.draw(m_tempGhost->costText);
        window.draw(m_tempGhost->levelText);
    }
}

/*
 * handle_event
 * ------------
 * - Handles keyboard input (left/right navigation of slider)
 * - Handles mouse input:
 *      - Click and hold on current icon -> prepare for drag
 *      - Click on another icon -> switch selection
 *      - Dragging with mouse -> update ghost position
 *      - Release -> drop ghost into a slot or animate out
 * - Passes events down to slot ghosts (UIEquipSlot objects)
 */
void UIEquipSlider::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    // Keyboard: move selection left/right
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        int newIndex = m_currentIndex;
        if (keyPressed->code == sf::Keyboard::Key::Left) newIndex--;
        else if (keyPressed->code == sf::Keyboard::Key::Right) newIndex++;

        if (newIndex != m_currentIndex && newIndex >= 0 && newIndex < static_cast<int>(m_equipIcons.size()))
            move_to_index(newIndex);
    }

    // Mouse press: start hold/drag detection or select another icon
    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mousePressed->button == sf::Mouse::Button::Left)
        {
            sf::Vector2f mousePos = window.mapPixelToCoords(mousePressed->position);
            if (!m_equipIcons.empty() && m_equipIcons[m_currentIndex]->get_bounds().contains(mousePos))
            {
                // Pressed on current icon -> start hold timer
                m_mouseDown = true;
                m_mouseDownPos = mousePos;
                m_holdClock.restart();
            }
            else
            {
                // Pressed another icon -> switch selection
                for (int i = 0; i < static_cast<int>(m_equipIcons.size()); ++i)
                {
                    if (i != m_currentIndex && m_equipIcons[i]->get_bounds().contains(mousePos))
                    {
                        move_to_index(i);
                        break;
                    }
                }
            }
        }
    }

    // Mouse move: start drag if moved enough, update drag ghost position
    if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
    {
        sf::Vector2f mousePos = window.mapPixelToCoords(mouseMoved->position);

        // Start drag if the mouse has moved more than a threshold
        if (m_mouseDown && !m_dragging)
        {
            float dx = mousePos.x - m_mouseDownPos.x;
            float dy = mousePos.y - m_mouseDownPos.y;
            if (std::sqrt(dx * dx + dy * dy) > 5.f)
            {
                if (!m_equipIcons.empty())
                {
                    DragInfo dragInfo;
                    int uid = m_equipIcons[m_currentIndex]->get_uid();
                    dragInfo.uid = uid;
                    dragInfo.cost = m_dataLoader->get_unit_data(uid)->cost;

                    // Load/cached texture for ghost
                    auto it = m_unitTextures.find(uid);
                    if (it == m_unitTextures.end())
                    {
                        sf::Texture tex;
                        if (!tex.loadFromFile(m_dataLoader->get_unit_icon_texture_path(uid)))
                            tex = m_placeholderTexture;
                        auto [insertIt, _] = m_unitTextures.emplace(uid, std::move(tex));
                        dragInfo.texture = &insertIt->second;
                    }
                    else
                    {
                        dragInfo.texture = &it->second;
                    }

                    dragInfo.iconSize = m_equipIcons[m_currentIndex]->get_size();

                    start_drag(dragInfo, mousePos);
                    m_dragging = true;
                }
            }
        }

        // While dragging, move ghost with cursor
        if (m_dragging && m_tempGhost && m_tempGhost->active)
        {
            update_drag_position(mousePos);
        }
    }

    // Mouse release: finish drag or reset state
    if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (mouseReleased->button == sf::Mouse::Button::Left)
        {
            if (m_dragging && m_tempGhost && m_tempGhost->active)
            {
                sf::Vector2f mousePos = window.mapPixelToCoords(mouseReleased->position);
                end_drag(mousePos);
            }
            m_mouseDown = false;
            m_dragging = false;
        }
    }

    // Forward event handling to equipped slots (so they can handle clicks)
    for (int i = 0; i < static_cast<int>(m_slots.size()); ++i)
    {
        if (m_slots[i].ghost)
            m_slots[i].ghost->handle_event(event, window);
    }
}

/*
 * move_to_index
 * -------------
 * - Animates icons when the selection index changes
 * - New center icon is larger and shifted slightly
 * - Other icons tween to their new positions/sizes
 */
void UIEquipSlider::move_to_index(int newIndex)
{
    if (newIndex < 0 || newIndex >= static_cast<int>(m_equipIcons.size()))
        return;

    sf::Vector2f screenCenter(1920.f / 2.0f - baseIconSize.x / 2.f,
        1080.f / 2.0f - baseIconSize.y / 2.f + 300.f);

    for (int i = 0; i < static_cast<int>(m_equipIcons.size()); ++i)
    {
        auto& icon = m_equipIcons[i];

        // Calculate target position
        float targetX = screenCenter.x + (i - newIndex) * ICON_SPACING;
        if (i == newIndex)
            targetX -= CENTER_OFFSET_X;

        // Target scale (larger if center)
        sf::Vector2f targetSize = baseIconSize * ((i == newIndex) ? CENTER_SCALE : SIDE_SCALE);

        // Animate tween transitions
        icon->tweenX = tweeny::from(icon->get_position().x).to(targetX).during(static_cast<int>(TWEEN_DURATION * 60.0f)).via(tweeny::easing::quadraticOut);
        icon->tweenWidth = tweeny::from(icon->get_size().x).to(targetSize.x).during(static_cast<int>(TWEEN_DURATION * 60.0f)).via(tweeny::easing::quadraticOut);
        icon->tweenHeight = tweeny::from(icon->get_size().y).to(targetSize.y).during(static_cast<int>(TWEEN_DURATION * 60.0f)).via(tweeny::easing::quadraticOut);

        icon->select(i == newIndex);
    }

    m_currentIndex = newIndex;
}

/*
 * start_drag
 * ----------
 * - Activates the drag ghost when dragging starts
 * - Sets ghost visuals (shape, sprite, cost text)
 * - Places ghost at starting cursor position
 */
void UIEquipSlider::start_drag(const DragInfo& dragInfo, const sf::Vector2f& startPos)
{
    if (!m_tempGhost) return;

    auto& tg = *m_tempGhost;
    tg.uid = dragInfo.uid;
    tg.cost = dragInfo.cost;
    tg.level = dragInfo.level;
    tg.active = true;

    // Shape
    sf::Vector2f shapeSize = dragInfo.iconSize;
    if (shapeSize.x <= 0.f || shapeSize.y <= 0.f)
        shapeSize = { 144.f, 108.f };

    tg.shape.setSize(shapeSize);
    tg.shape.setFillColor(sf::Color(255, 255, 255, 150));
    tg.shape.setOutlineColor(sf::Color::Black);
    tg.shape.setOutlineThickness(3.f);
    tg.shape.setOrigin(shapeSize * 0.5f);
    tg.shape.setPosition(startPos);

    // Texture and Sprite
    if (dragInfo.texture)
        tg.texture = *dragInfo.texture;
    else
        tg.texture = m_placeholderTexture;

    tg.sprite.setTexture(tg.texture);
    tg.sprite.setTextureRect(sf::IntRect({ 0, 0 }, { static_cast<int>(tg.texture.getSize().x), static_cast<int>(tg.texture.getSize().y) }));
    tg.sprite.setOrigin({
        static_cast<float>(tg.texture.getSize().x) / 2.f,
        static_cast<float>(tg.texture.getSize().y) / 2.f
        });
    tg.sprite.setScale({
        shapeSize.x / tg.texture.getSize().x,
        shapeSize.y / tg.texture.getSize().y
        });
    tg.sprite.setColor(sf::Color(255, 255, 255, 200));
    tg.sprite.setPosition(startPos);

    // Cost label
    sf::FloatRect shapeBounds = tg.shape.getGlobalBounds();

    tg.costText.setString("Cost: " + std::to_string(dragInfo.cost) + "$");
    tg.costText.setCharacterSize(tg.INFO_BASE_TEXT_SIZE);

    tg.costText.setOrigin({ 0.f, 0.f });
    tg.costText.setPosition({
        shapeBounds.position.x + shapeBounds.size.x - tg.costText.getLocalBounds().size.x - tg.INFO_BASE_TEXT_OFFSET,
        shapeBounds.position.y + shapeBounds.size.y - tg.costText.getLocalBounds().size.y - tg.INFO_BASE_TEXT_OFFSET
        });

    // Level label
    tg.levelText.setString("Lvl. " + std::to_string(dragInfo.level));
    tg.levelText.setCharacterSize(tg.INFO_BASE_TEXT_SIZE);

    tg.levelText.setOrigin({ 0.f, 0.f });
    tg.levelText.setPosition({
        shapeBounds.position.x + shapeBounds.size.x - tg.levelText.getLocalBounds().size.x - tg.INFO_BASE_TEXT_OFFSET,
        shapeBounds.position.y + shapeBounds.size.y - tg.levelText.getLocalBounds().size.y - tg.INFO_BASE_TEXT_OFFSET - tg.costText.getCharacterSize() - m_tempGhost->INFO_BASE_TEXT_OFFSET
        });


    tg.currentScale = 1.f;
    tg.scaleTween = tweeny::from(1.f).to(1.f).during(60); // idle tween

    update_drag_position(startPos);
}

/*
 * update_drag_position
 * --------------------
 * - Moves drag ghost to cursor position
 * - Scales ghost if hovering over a valid slot
 * - Otherwise, smoothly returns ghost to normal scale
 */
void UIEquipSlider::update_drag_position(const sf::Vector2f& pos)
{
    if (!m_tempGhost || !m_tempGhost->active) return;
    auto& tg = *m_tempGhost;

    // Update ghost visuals to cursor position
    tg.shape.setPosition(pos);
    tg.sprite.setPosition(pos);

    // Always keep costText and levelText at bottom-right inside the shape
    sf::FloatRect shapeBounds = tg.shape.getGlobalBounds();

    tg.costText.setPosition({
        shapeBounds.position.x + shapeBounds.size.x - tg.costText.getLocalBounds().size.x - tg.INFO_BASE_TEXT_OFFSET,
        shapeBounds.position.y + shapeBounds.size.y - tg.costText.getLocalBounds().size.y - tg.INFO_BASE_TEXT_OFFSET
        });
    tg.levelText.setPosition({
        shapeBounds.position.x + shapeBounds.size.x - tg.levelText.getLocalBounds().size.x - tg.INFO_BASE_TEXT_OFFSET,
        shapeBounds.position.y + shapeBounds.size.y - tg.levelText.getLocalBounds().size.y - tg.INFO_BASE_TEXT_OFFSET - tg.costText.getCharacterSize() - m_tempGhost->INFO_BASE_TEXT_OFFSET
        });


    // Detect if cursor is over a slot
    int hoveredSlot = -1;
    for (int i = 0; i < static_cast<int>(m_slots.size()); ++i)
    {
        if (m_slots[i].bounds.contains(pos))
        {
            hoveredSlot = i;
            break;
        }
    }

    // Tween scaling depending on hover state
    if (hoveredSlot != -1)
    {
        sf::Vector2f slotSize{ m_slots[hoveredSlot].bounds.size.x, m_slots[hoveredSlot].bounds.size.y };
        float targetScaleX = slotSize.x / tg.shape.getSize().x;
        float targetScaleY = slotSize.y / tg.shape.getSize().y;
        float targetScale = std::min(targetScaleX, targetScaleY);
        tg.scaleTween = tweeny::from(tg.currentScale).to(targetScale).during(8).via(tweeny::easing::quadraticOut);
    }
    else
    {
        tg.scaleTween = tweeny::from(tg.currentScale).to(1.f).during(8).via(tweeny::easing::quadraticOut);
    }
}

/*
 * end_drag
 * --------
 * - Handles releasing the ghost
 * - If released over an empty slot, equips the unit there
 * - If invalid, ghost shrinks out with an animation
 */
void UIEquipSlider::end_drag(const sf::Vector2f& pos)
{
    if (!m_tempGhost || !m_tempGhost->active) return;

    // Detect drop slot
    int targetSlot = -1;
    for (int i = 0; i < static_cast<int>(m_slots.size()); ++i)
    {
        if (m_slots[i].bounds.contains(pos))
        {
            targetSlot = i;
            break;
        }
    }

    if (targetSlot != -1)
    {
        // Prevent duplicates: unit already equipped?
        bool alreadyEquipped = false;
        for (int j = 0; j < static_cast<int>(m_slots.size()); ++j)
        {
            if (m_slots[j].ghost)
            {
                auto s = std::dynamic_pointer_cast<UIEquipSlot>(m_slots[j].ghost);
                if (s && s->get_uid() == m_tempGhost->uid)
                {
                    alreadyEquipped = true;
                    break;
                }
            }
        }

        // If not already equipped -> assign unit to slot
        if (!alreadyEquipped)
        {
            if (auto player = m_dataLoader->get_player_data().lock())
            {
                player->equip_unit(m_tempGhost->uid, targetSlot);

                auto slotGhost = std::make_shared<UIEquipSlot>(
                    m_tempGhost->uid,
                    m_tempGhost->cost,
                    m_dataLoader->get_unit_icon_texture_path(m_tempGhost->uid),
                    targetSlot
                );

                slotGhost->set_position({ m_slots[targetSlot].bounds.position.x, m_slots[targetSlot].bounds.position.y });
                slotGhost->set_size({ m_slots[targetSlot].bounds.size.x, m_slots[targetSlot].bounds.size.y });

                // Define unequip callback
                slotGhost->set_callback([this](int slotIndex) {
                    if (auto player = m_dataLoader->get_player_data().lock()) {
                        player->unequip_unit(slotIndex);
                        m_slots[slotIndex].ghost = nullptr;
                    }
                    });

                m_slots[targetSlot].ghost = slotGhost;
            }
        }

        m_tempGhost->active = false;
        return;
    }

    // If dropped in empty space -> shrink out animations
    m_tempGhost->scaleTween = tweeny::from(m_tempGhost->currentScale).to(0.f).during(12).via(tweeny::easing::quadraticIn);
}

UIEquipSlider::TempGhost::TempGhost(const sf::Font& font, const sf::Texture& placeholder)
    : texture(placeholder)
    , sprite(texture)
    , costText(font)
    , levelText(font)
    , active(false)
    , currentScale(1.f)
    , scaleTween(tweeny::from(1.f).to(1.f).during(1))
{
    costText.setFillColor(sf::Color::Yellow);
    costText.setOutlineColor(sf::Color::Black);
    costText.setCharacterSize(INFO_BASE_TEXT_SIZE);
    costText.setOutlineThickness(2.f);

    levelText.setFillColor(sf::Color::Green);
    levelText.setOutlineColor(sf::Color::Black);
    levelText.setCharacterSize(INFO_BASE_TEXT_SIZE);
    levelText.setOutlineThickness(2.f);

    // center origins will be set when sizes are known
}

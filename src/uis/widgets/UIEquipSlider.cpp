#include "UIEquipSlider.h"
#include "UIBattleIcon.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
    // Lineup: the same slots as in battle, in a dark panel under the cards
    const float SLOT_SPACING = 12.f;
    const float LINEUP_TOP = 670.f;                      // Top of the first row of slots
    const float PANEL_PADDING = 22.f;
    const float PANEL_TITLE_BAND = 52.f;                 // "LINEUP" above the slots
    const float GHOST_SCALE = 1.5f;                      // The dragged unit is a slot this much bigger
    const float ARROW_SIZE = 96.f;
    const float ARROW_Y = 292.f;
    const float HINT_Y = 562.f;

    sf::Vector2f lineup_size()
    {
        return { 5.f * UIBattleIcon::WIDTH + 4.f * SLOT_SPACING, 2.f * UIBattleIcon::HEIGHT + SLOT_SPACING };
    }

    sf::FloatRect lineup_panel_rect()
    {
        const sf::Vector2f slots = lineup_size();
        const sf::Vector2f size{ slots.x + 2.f * PANEL_PADDING, slots.y + PANEL_PADDING + PANEL_TITLE_BAND };
        return { { (1920.f - size.x) / 2.f, LINEUP_TOP - PANEL_TITLE_BAND }, size };
    }
}

/*
 * Constructor
 * -----------
 * - Loads the placeholder texture
 * - Initializes the drag ghost, the arrows and the lineup panel
 * - Calls init_icons() to build the equip UI
 */
UIEquipSlider::UIEquipSlider(std::shared_ptr<DataLoader> dataLoader)
    : m_dataLoader(std::move(dataLoader)),
    m_previousButton(ARROW_SIZE, { 36.f, ARROW_Y }, "icon_play"),
    m_nextButton(ARROW_SIZE, { 1920.f - 36.f - ARROW_SIZE, ARROW_Y }, "icon_play"),
    m_lineupPanel(lineup_panel_rect().size, lineup_panel_rect().position, UISkin::PANEL_DARK, 1.2f),
    m_lineupTitle(UISkin::heavy_font(), "LINEUP"),
    m_hint(UISkin::heavy_font(), "Drag a fish into the lineup  -  right click a fish of the lineup to remove it"),
    m_ghostCost(UISkin::heavy_font())
{
    if (!m_placeholderTexture.loadFromFile("assets/images/textures/icons/placeholder.png")) {
        std::cerr << "UIEquipSlider: failed to load placeholder texture\n";
    }

    // Create temporary drag ghost
    m_tempGhost = std::make_unique<TempGhost>(m_placeholderTexture);

    m_previousButton.set_icon_flipped(true);
    m_previousButton.set_click_sound("");   // move_to_index plays the selection sound
    m_nextButton.set_click_sound("");
    m_previousButton.set_callback([this]() { move_to_index(m_currentIndex - 1); });
    m_nextButton.set_callback([this]() { move_to_index(m_currentIndex + 1); });

    const sf::FloatRect panel = lineup_panel_rect();
    UISkin::style_text(m_lineupTitle, 34, UISkin::TITLE);
    UISkin::set_text_anchor(m_lineupTitle, { 0.5f, 0.5f });
    m_lineupTitle.setPosition({ 960.f, panel.position.y + PANEL_TITLE_BAND / 2.f + 6.f });

    UISkin::style_text(m_hint, 26);
    UISkin::set_text_anchor(m_hint, { 0.5f, 0.5f });
    m_hint.setPosition({ 960.f, HINT_Y });

    UISkin::style_text(m_ghostCost, 32, UISkin::NUMBER);

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

    m_currentIndex = std::clamp(m_currentIndex, 0, std::max(0, static_cast<int>(m_availableEquips.size()) - 1));

    // Left edge of a side card in the middle; the cards stand on CARDS_BOTTOM
    sf::Vector2f screenCenter(1920.f / 2.0f - baseIconSize.x / 2.f, CARDS_BOTTOM);

    // Grid configuration for equip slots: the deploy slots of the battle
    const int rows = 2;
    const int cols = 5;
    const float iconWidth = UIBattleIcon::WIDTH;
    const float iconHeight = UIBattleIcon::HEIGHT;
    const float spacingX = SLOT_SPACING;
    const float spacingY = SLOT_SPACING;

    // Calculate starting position for grid
    float startX = (1920.f - lineup_size().x) / 2.f;
    float startY = LINEUP_TOP;

    // Create slot rectangles
    int index = 0;
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            sf::Vector2f pos{ startX + col * (iconWidth + spacingX), startY + row * (iconHeight + spacingY) };
            sf::Vector2f size{ iconWidth, iconHeight };

            m_slots[index].bounds = sf::FloatRect(pos, size);
            m_slots[index].ghost = nullptr;

            index++;
        }
    }

    // Load currently equipped units into compacted slots
    if (auto player = m_dataLoader->get_player_data().lock())
    {
        // Collect all equipped units without gaps
        std::vector<int> equipped;
        for (int i = 0; i < static_cast<int>(m_slots.size()); ++i) 
        {
            int uid = player->equippedUnits[i];
            if (uid >= 0) {
                equipped.push_back(uid);
            }
        }

        // Reassign them from slot 0 to avoid empty gaps
        for (int i = 0; i < static_cast<int>(equipped.size()); ++i) 
        {
            int uid = equipped[i];
            int form = player->get_unit_form(uid);
            auto unitData = m_dataLoader->get_unit_data(uid, form);

            // Create UIEquipSlot for equipped unit
            auto slotGhost = std::make_shared<UIEquipSlot>(
                unitData->UID,
                unitData->cost,
                m_dataLoader->get_unit_icon_texture_path(uid, form),
                i
            );

            slotGhost->set_position({ m_slots[i].bounds.position.x, m_slots[i].bounds.position.y });
            slotGhost->set_size({ m_slots[i].bounds.size.x, m_slots[i].bounds.size.y });

            // Define unequip callback
            slotGhost->set_callback([this](int slotIndex) 
                {
                if (auto player = m_dataLoader->get_player_data().lock()) 
                {
                    // Unequip the clicked slot
                    player->unequip_unit(slotIndex);
                    play_sound("unequip");

                    // Refresh all slots and redraw ghosts in order
                    refresh_slots_after_unequip();
                }
                });

            m_slots[i].ghost = slotGhost;
        }

        // Update PlayerData with the compacted slot order
        for (int i = 0; i < static_cast<int>(m_slots.size()); ++i) 
        {
            if (i < static_cast<int>(equipped.size()))
                player->equippedUnits[i] = equipped[i];
            else
                player->equippedUnits[i] = -1;
        }
    }

    // Create slider icons for all available units
    for (int i = 0; i < static_cast<int>(m_availableEquips.size()); ++i)
    {
        int unitUid = m_availableEquips[i];
        int unitLevel = playerData->get_unit_level(unitUid);
        int unitForm = playerData->get_unit_form(unitUid);
        auto unitData = m_dataLoader->get_unit_data(unitUid, unitForm);

        auto icon = std::make_shared<UIEquipIcon>(
            m_dataLoader->get_unit_icon_texture_path(unitUid, unitForm),
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
    // Cards: tweens, and "IN USE" on the units already in the lineup
    auto player = m_dataLoader->get_player_data().lock();
    for (auto& icon : m_equipIcons)
    {
        if (player)
            icon->set_in_lineup(std::find(player->equippedUnits.begin(), player->equippedUnits.end(), icon->get_uid()) != player->equippedUnits.end());
        icon->update(deltaTime);
    }

    m_previousButton.set_visible(m_currentIndex > 0);
    m_nextButton.set_visible(m_currentIndex + 1 < static_cast<int>(m_equipIcons.size()));
    m_previousButton.update(deltaTime);
    m_nextButton.update(deltaTime);

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
                dragInfo.cost = m_dataLoader->get_unit_data(uid, m_dataLoader->get_player_data().lock()->get_unit_form(uid))->cost;
                dragInfo.level = m_dataLoader->get_player_data().lock()->get_unit_level(uid);

                // Load texture into cache if needed
                auto it = m_unitTextures.find(uid);
                if (it == m_unitTextures.end())
                {
                    sf::Texture tex;
                    if (!tex.loadFromFile(m_dataLoader->get_unit_icon_texture_path(uid, m_dataLoader->get_player_data().lock()->get_unit_form(uid))))
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
    }
    else
    {
        // If tween shrinks to zero, deactivate ghost
        float finalVal = m_tempGhost->scaleTween.peek(1.0f);
        if (std::abs(finalVal) < 1e-6f)
            m_tempGhost->active = false;
    }
}

/*
 * render
 * ------
 * - Draws the cards (the selected one on top), the arrows and the hint
 * - Draws the lineup panel, its empty slots and the equipped units
 * - If a ghost is active, draws it on top
 */
void UIEquipSlider::render(sf::RenderWindow& window)
{
    for (int i = 0; i < static_cast<int>(m_equipIcons.size()); ++i)
        if (i != m_currentIndex) m_equipIcons[i]->render(window);
    if (!m_equipIcons.empty())
        m_equipIcons[m_currentIndex]->render(window);

    m_previousButton.render(window);
    m_nextButton.render(window);
    window.draw(m_hint);

    render_lineup(window);

    for (auto& slot : m_slots)
    {
        if (slot.ghost)
            slot.ghost->render(window);
    }

    render_ghost(window);
}

/*
 * render_lineup
 * -------------
 * The dark panel with "LINEUP", and every slot as an empty (dimmed) deploy slot
 */
void UIEquipSlider::render_lineup(sf::RenderWindow& window)
{
    m_lineupPanel.render(window);
    window.draw(m_lineupTitle);

    for (auto& slot : m_slots)
    {
        if (slot.ghost) continue;
        UISkin::draw_nine_slice(window, UISkin::SLOT, slot.bounds, 1.f, UISkin::EMPTY_SLOT_TINT);
        UISkin::draw_nine_slice(window, UISkin::SLOT_FRAME, slot.bounds, 1.f);
    }
}

/*
 * render_ghost
 * ------------
 * The dragged unit: a lineup slot following the mouse (its shape is centered on it),
 * slightly see-through, with its deploy cost
 */
void UIEquipSlider::render_ghost(sf::RenderWindow& window)
{
    if (!m_tempGhost || !m_tempGhost->active) return;

    const sf::FloatRect rect = m_tempGhost->shape.getGlobalBounds();
    if (rect.size.x < 1.f || rect.size.y < 1.f) return;
    const float scale = rect.size.x / UIBattleIcon::WIDTH;
    const sf::Color seeThrough(255, 255, 255, 225);

    UISkin::draw_nine_slice(window, UISkin::SLOT, rect, scale, seeThrough);
    UISkin::draw_cropped(window, m_tempGhost->texture, UISkin::slot_inner(rect, scale), seeThrough);
    UISkin::draw_nine_slice(window, UISkin::SLOT_FRAME, rect, scale, seeThrough);

    m_ghostCost.setString(std::to_string(m_tempGhost->cost) + "$");
    UISkin::set_text_anchor(m_ghostCost, { 1.f, 1.f });
    m_ghostCost.setScale({ scale, scale });
    m_ghostCost.setPosition(rect.position + rect.size - sf::Vector2f{ 12.f, 10.f } * scale);
    window.draw(m_ghostCost);
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
            else if (!(m_previousButton.is_visible() && m_previousButton.get_bounds().contains(mousePos))
                && !(m_nextButton.is_visible() && m_nextButton.get_bounds().contains(mousePos)))
            {
                // Pressed another icon -> switch selection (the arrows, drawn over the far cards, come first)
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
                    dragInfo.cost = m_dataLoader->get_unit_data(uid, m_dataLoader->get_player_data().lock()->get_unit_form(uid))->cost;
                    dragInfo.level = m_dataLoader->get_player_data().lock()->get_unit_level(uid);

                    // Load/cached texture for ghost
                    auto it = m_unitTextures.find(uid);
                    if (it == m_unitTextures.end())
                    {
                        sf::Texture tex;
                        if (!tex.loadFromFile(m_dataLoader->get_unit_icon_texture_path(uid, m_dataLoader->get_player_data().lock()->get_unit_form(uid))))
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

    m_previousButton.handle_event(event, window);
    m_nextButton.handle_event(event, window);
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
    if (newIndex < 0 || newIndex >= static_cast<int>(m_equipIcons.size()) || newIndex == m_currentIndex)
        return;

    play_sound("ui_select");

    sf::Vector2f screenCenter(1920.f / 2.0f - baseIconSize.x / 2.f, CARDS_BOTTOM);

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
 * - Sets ghost visuals (shape and texture; render_ghost draws it)
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

    // Shape: a lineup slot, bigger while it's not over a slot (centered on the mouse)
    const sf::Vector2f shapeSize{ UIBattleIcon::WIDTH * GHOST_SCALE, UIBattleIcon::HEIGHT * GHOST_SCALE };
    tg.shape.setSize(shapeSize);
    tg.shape.setOrigin(shapeSize * 0.5f);
    tg.shape.setScale({ 1.f, 1.f });
    tg.shape.setPosition(startPos);

    // Texture (drawn by render_ghost)
    if (dragInfo.texture)
        tg.texture = *dragInfo.texture;
    else
        tg.texture = m_placeholderTexture;
    tg.texture.setSmooth(true);

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

    // Update ghost position (render_ghost draws it from the shape)
    tg.shape.setPosition(pos);

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

    // If released on any slot area
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

        // If not already equipped -> assign unit to the first available slot
        if (!alreadyEquipped)
        {
            int finalSlot = -1;
            for (int i = 0; i < static_cast<int>(m_slots.size()); ++i)
            {
                if (!m_slots[i].ghost)
                {
                    finalSlot = i;
                    break;
                }
            }

            if (finalSlot != -1)
            {
                if (auto player = m_dataLoader->get_player_data().lock())
                {
                    player->equip_unit(m_tempGhost->uid, finalSlot);
                    play_sound("equip");

                    auto slotGhost = std::make_shared<UIEquipSlot>(
                        m_tempGhost->uid,
                        m_tempGhost->cost,
                        m_dataLoader->get_unit_icon_texture_path(m_tempGhost->uid, m_dataLoader->get_player_data().lock()->get_unit_form(m_tempGhost->uid)),
                        finalSlot
                    );

                    slotGhost->set_position({ m_slots[finalSlot].bounds.position.x, m_slots[finalSlot].bounds.position.y });
                    slotGhost->set_size({ m_slots[finalSlot].bounds.size.x, m_slots[finalSlot].bounds.size.y });

                    // Define unequip callback
                    slotGhost->set_callback([this](int slotIndex) 
                        {
                        if (auto player = m_dataLoader->get_player_data().lock()) {
                            // Unequip the clicked slot
                            player->unequip_unit(slotIndex);
                            play_sound("unequip");

                            // Refresh all slots and redraw ghosts in order
                            refresh_slots_after_unequip();
                        }
                        });

                    m_slots[finalSlot].ghost = slotGhost;
                }
            }
        }

        m_tempGhost->active = false;
        return;
    }

    // If dropped in empty space -> shrink out animations
    m_tempGhost->scaleTween = tweeny::from(m_tempGhost->currentScale).to(0.f).during(12).via(tweeny::easing::quadraticIn);
}

/*
 * refresh_slots_after_unequip
 * --------
 * - Refresh all equipped slots after an unequip action
 * - Updates equipedUnits from PlayerData
 */
void UIEquipSlider::refresh_slots_after_unequip()
{
    auto player = m_dataLoader->get_player_data().lock();
    if (!player) return;

    // Collect all equipped units in order without gaps
    std::vector<int> equipped;
    for (int uid : player->equippedUnits)
        if (uid >= 0)
            equipped.push_back(uid);

    // Clear all ghosts (visuals) first
    for (auto& slot : m_slots)
        slot.ghost = nullptr;

    // Reassign units starting from slot 0
    int newSlot = 0;
    for (int uid : equipped)
    {
        if (newSlot >= static_cast<int>(m_slots.size()))
            break;

        int form = player->get_unit_form(uid);
        auto unitData = m_dataLoader->get_unit_data(uid, form);

        // Create new ghost
        auto newGhost = std::make_shared<UIEquipSlot>(
            uid,
            unitData->cost,
            m_dataLoader->get_unit_icon_texture_path(uid, form),
            newSlot
        );

        newGhost->set_position({ m_slots[newSlot].bounds.position.x, m_slots[newSlot].bounds.position.y });
        newGhost->set_size({ m_slots[newSlot].bounds.size.x, m_slots[newSlot].bounds.size.y });

        // Set unequip callback
        newGhost->set_callback([this](int slotIndex) {
            auto player = m_dataLoader->get_player_data().lock();
            if (!player) return;

            // Unequip from PlayerData
            player->unequip_unit(slotIndex);
            play_sound("unequip");

            // Refresh all slots
            this->refresh_slots_after_unequip();
            });

        m_slots[newSlot].ghost = newGhost;
        newSlot++;
    }

    // 4. Update PlayerData equippedUnits after all ghosts are recreated
    for (int i = 0; i < static_cast<int>(m_slots.size()); ++i)
    {
        if (i < static_cast<int>(equipped.size()))
            player->equippedUnits[i] = equipped[i];
        else
            player->equippedUnits[i] = -1;
    }
}


UIEquipSlider::TempGhost::TempGhost(const sf::Texture& placeholder)
    : texture(placeholder)
    , active(false)
    , currentScale(1.f)
    , scaleTween(tweeny::from(1.f).to(1.f).during(1))
{
}
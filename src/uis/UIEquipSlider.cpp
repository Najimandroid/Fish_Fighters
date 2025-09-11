#include "UIEquipSlider.h"
#include "../core/DataLoader.h"

#include <algorithm>
#include <cmath>
#include <iostream>

UIEquipSlider::UIEquipSlider(std::shared_ptr<DataLoader> dataLoader)
    : m_dataLoader(std::move(dataLoader))
{
    if (!m_sharedFont.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf")) {
        std::cerr << "UIEquipSlider: failed to load font\n";
    }

    if (!m_placeholderTexture.loadFromFile("assets/images/textures/icons/placeholder.png")) {
        std::cerr << "UIEquipSlider: failed to load placeholder texture\n";
    }

    m_tempGhost = std::make_unique<TempGhost>(m_sharedFont, m_placeholderTexture);

    init_icons();
}

void UIEquipSlider::init_icons()
{
    m_equipIcons.clear();
    m_availableEquips.clear();

    auto playerData = m_dataLoader->get_player_data().lock();
    if (!playerData) return;

    for (auto& kv : playerData->ownedUnits)
        m_availableEquips.push_back(kv.first);

    std::sort(m_availableEquips.begin(), m_availableEquips.end());

    sf::Vector2f screenCenter(
        1920.f / 2.0f - baseIconSize.x / 2.f,
        1080.f / 2.0f - baseIconSize.y / 2.f + 300.f
    );

    // slot grid
    const int rows = 2;
    const int cols = 5;

    const float iconWidth = 144.f * 1.15f;
    const float iconHeight = 108.f * 1.15f;
    const float spacingX = 24.f * 1.15f;
    const float spacingY = 24.f * 1.15f;

    float totalWidth = cols * iconWidth + (cols - 1) * spacingX;
    float startX = (1920.f - totalWidth) / 2.f;
    float startY = 775.f;

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

    // load equipped units into the slots
    if (auto player = m_dataLoader->get_player_data().lock()) {
        for (int i = 0; i < static_cast<int>(m_slots.size()); ++i) {
            int uid = player->equippedUnits[i];
            if (uid >= 0) {
                auto unitData = m_dataLoader->get_unit_data(uid);
                // use texture path constructor of UIEquipSlot (assumes this exists)
                auto slotGhost = std::make_shared<UIEquipSlot>(
                    unitData->UID,
                    unitData->cost,
                    m_dataLoader->get_unit_icon_texture_path(uid),
                    i
                );

                slotGhost->set_position({ m_slots[i].bounds.position.x, m_slots[i].bounds.position.y });
                slotGhost->set_size({ m_slots[i].bounds.size.x, m_slots[i].bounds.size.y });

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

    // create slider icons
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

        icon->select(i == m_currentIndex);
        icon->set_unit_name(unitData->name);
        icon->set_unit_level(unitLevel);
        icon->set_deployment_cost(unitData->cost);

        float xOffset = (i - m_currentIndex) * ICON_SPACING;
        float posX = screenCenter.x + xOffset;
        if (i == m_currentIndex)
            posX -= CENTER_OFFSET_X;
        icon->set_position({ posX, screenCenter.y });

        float scale = (i == m_currentIndex) ? CENTER_SCALE : SIDE_SCALE;
        icon->set_size(baseIconSize * scale);

        icon->tweenX = tweeny::from(icon->get_position().x).to(icon->get_position().x).during(1);
        icon->tweenWidth = tweeny::from(icon->get_size().x).to(icon->get_size().x).during(1);
        icon->tweenHeight = tweeny::from(icon->get_size().y).to(icon->get_size().y).during(1);

        m_equipIcons.push_back(icon);
    }
}

void UIEquipSlider::update(float deltaTime)
{
    for (auto& icon : m_equipIcons)
        icon->update(deltaTime);

    // hold-to-drag detection
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

                // try to fetch from cache
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

    // advance tween by 1 step per frame (as requested)
    if (m_tempGhost->scaleTween.progress() < 1.0f)
    {
        m_tempGhost->currentScale = m_tempGhost->scaleTween.step(1);
        m_tempGhost->shape.setScale({ m_tempGhost->currentScale, m_tempGhost->currentScale });

        // keep sprite scaled to fit the shape
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
        float finalVal = m_tempGhost->scaleTween.peek(1.0f);
        if (std::abs(finalVal) < 1e-6f)
            m_tempGhost->active = false;
    }
}

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
    }
}

void UIEquipSlider::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        int newIndex = m_currentIndex;
        if (keyPressed->code == sf::Keyboard::Key::Left) newIndex--;
        else if (keyPressed->code == sf::Keyboard::Key::Right) newIndex++;

        if (newIndex != m_currentIndex && newIndex >= 0 && newIndex < static_cast<int>(m_equipIcons.size()))
            move_to_index(newIndex);
    }

    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mousePressed->button == sf::Mouse::Button::Left)
        {
            sf::Vector2f mousePos = window.mapPixelToCoords(mousePressed->position);
            if (!m_equipIcons.empty() && m_equipIcons[m_currentIndex]->get_bounds().contains(mousePos))
            {
                m_mouseDown = true;
                m_mouseDownPos = mousePos;
                m_holdClock.restart();
            }
            else
            {
                // click on other icon -> select
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

    if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
    {
        sf::Vector2f mousePos = window.mapPixelToCoords(mouseMoved->position);

        // start drag if we moved enough and were holding
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

        if (m_dragging && m_tempGhost && m_tempGhost->active)
        {
            update_drag_position(mousePos);
        }
    }

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

    for (int i = 0; i < static_cast<int>(m_slots.size()); ++i)
    {
        if (m_slots[i].ghost)
            m_slots[i].ghost->handle_event(event, window);
    }
}

void UIEquipSlider::move_to_index(int newIndex)
{
    if (newIndex < 0 || newIndex >= static_cast<int>(m_equipIcons.size()))
        return;

    sf::Vector2f screenCenter(1920.f / 2.0f - baseIconSize.x / 2.f,
        1080.f / 2.0f - baseIconSize.y / 2.f + 300.f);

    for (int i = 0; i < static_cast<int>(m_equipIcons.size()); ++i)
    {
        auto& icon = m_equipIcons[i];

        float targetX = screenCenter.x + (i - newIndex) * ICON_SPACING;
        if (i == newIndex)
            targetX -= CENTER_OFFSET_X;

        sf::Vector2f targetSize = baseIconSize * ((i == newIndex) ? CENTER_SCALE : SIDE_SCALE);

        icon->tweenX = tweeny::from(icon->get_position().x).to(targetX).during(static_cast<int>(TWEEN_DURATION * 60.0f)).via(tweeny::easing::quadraticOut);
        icon->tweenWidth = tweeny::from(icon->get_size().x).to(targetSize.x).during(static_cast<int>(TWEEN_DURATION * 60.0f)).via(tweeny::easing::quadraticOut);
        icon->tweenHeight = tweeny::from(icon->get_size().y).to(targetSize.y).during(static_cast<int>(TWEEN_DURATION * 60.0f)).via(tweeny::easing::quadraticOut);

        icon->select(i == newIndex);
    }

    m_currentIndex = newIndex;
}

void UIEquipSlider::start_drag(const DragInfo& dragInfo, const sf::Vector2f& startPos)
{
    if (!m_tempGhost) return;

    auto& tg = *m_tempGhost;
    tg.uid = dragInfo.uid;
    tg.cost = dragInfo.cost;
    tg.active = true;

    sf::Vector2f shapeSize = dragInfo.iconSize;
    if (shapeSize.x <= 0.f || shapeSize.y <= 0.f) {
        shapeSize = { 144.f, 108.f };
    }

    tg.shape.setSize(shapeSize);
    tg.shape.setFillColor(sf::Color(255, 255, 255, 150));
    tg.shape.setOutlineColor(sf::Color::Black);
    tg.shape.setOutlineThickness(3.f);

    tg.shape.setOrigin(sf::Vector2f{ shapeSize.x * 0.5f, shapeSize.y * 0.5f });
    tg.shape.setPosition(startPos);

    if (dragInfo.texture)
        tg.texture = *dragInfo.texture;
    else
        tg.texture = m_placeholderTexture;

    tg.sprite.setTexture(tg.texture);
    sf::FloatRect local = tg.sprite.getLocalBounds();

    if (local.size.x > 0.f && local.size.y > 0.f)
    {
        float sx = shapeSize.x / local.size.x;
        float sy = shapeSize.y / local.size.y;
        tg.sprite.setScale({ sx, sy });
        tg.sprite.setOrigin(sf::Vector2f{ local.size.x * 0.5f, local.size.y * 0.5f });
    }

    tg.sprite.setColor(sf::Color(255, 255, 255, 200));
    tg.sprite.setPosition(startPos);

    tg.costText.setString(std::to_string(dragInfo.cost) + "$");
    sf::FloatRect costLocal = tg.costText.getLocalBounds();
    tg.costText.setOrigin(sf::Vector2f{ costLocal.position.x + costLocal.size.x, costLocal.position.y + costLocal.size.y });

    tg.currentScale = 1.f;
    tg.scaleTween = tweeny::from(1.f).to(1.f).during(60); // idle by default

    update_drag_position(startPos);
}

void UIEquipSlider::update_drag_position(const sf::Vector2f& pos)
{
    if (!m_tempGhost || !m_tempGhost->active) return;
    auto& tg = *m_tempGhost;

    tg.shape.setPosition(pos);
    tg.sprite.setPosition(pos);

    sf::FloatRect shapeBounds = tg.shape.getGlobalBounds();
    tg.costText.setPosition(sf::Vector2f{ shapeBounds.position.x + shapeBounds.size.x - 8.f,
                                          shapeBounds.position.y + shapeBounds.size.y - 8.f });

    int hoveredSlot = -1;
    for (int i = 0; i < static_cast<int>(m_slots.size()); ++i)
    {
        if (m_slots[i].bounds.contains(pos))
        {
            hoveredSlot = i;
            break;
        }
    }

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

void UIEquipSlider::end_drag(const sf::Vector2f& pos)
{
    if (!m_tempGhost || !m_tempGhost->active) return;

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

    // shrink out animation
    m_tempGhost->scaleTween = tweeny::from(m_tempGhost->currentScale).to(0.f).during(12).via(tweeny::easing::quadraticIn);
}

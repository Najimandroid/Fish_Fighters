#include "UIDeploymentIcons.h"
#include "../UISkin.h"
#include "../../core/stages/Stage.h"

#include <algorithm>
#include <cstdint>
#include <iostream>

namespace
{
    const sf::Color MESSAGE_COLOR{ 255, 128, 104 };   // Warm red, outlined like every text
    const unsigned int MESSAGE_SIZE = 46;
}

/*
 * Stores references to the DataLoader and Stage, then initializes the icons grid.
 */
UIDeploymentIcons::UIDeploymentIcons(std::shared_ptr<DataLoader> dataLoader, std::shared_ptr<Stage> stage) :
    m_dataLoader(dataLoader), m_stage(stage),
    m_message(UISkin::heavy_font())
{
    UISkin::style_text(m_message, MESSAGE_SIZE, MESSAGE_COLOR);
    init_icons();
}

void UIDeploymentIcons::show_message(const std::string& text)
{
    m_message.setString(text);
    m_messageTime = MESSAGE_SECONDS;
}

/*
 * Creates a grid of icons for unit deployment.
 * - Arranges icons in 2 rows and 5 columns
 * - Reads equipped units from player data
 * - Sets textures, cooldowns, costs, and click callbacks for each icon
 */
void UIDeploymentIcons::init_icons()
{
    std::array<int, 10> equippedUnits;
    if (auto player = m_dataLoader->get_player_data().lock())
        equippedUnits = player->equippedUnits; // retrieve equipped unit IDs
    else
        equippedUnits.fill(-1); // no units equipped

    // The second row only when it holds a fish: otherwise the first row sits at the bottom, giving the
    // battlefield more room
    const bool hasSecondRow = std::any_of(equippedUnits.begin() + 5, equippedUnits.end(), [](int uid) { return uid >= 0; });
    const int rows = hasSecondRow ? 2 : 1;
    const int cols = 5;

    const float iconWidth = UIBattleIcon::WIDTH;
    const float iconHeight = UIBattleIcon::HEIGHT;
    const float spacingX = 12.f;
    const float spacingY = 12.f;
    const float bottomMargin = 18.f;

    float totalWidth = cols * iconWidth + (cols - 1) * spacingX;
    float totalHeight = rows * iconHeight + (rows - 1) * spacingY;
    float startX = (1920.f - totalWidth) / 2.f; // center horizontally
    float startY = 1080.f - bottomMargin - totalHeight; // at the bottom of the screen

    int index = 0;

    // Keys 1 to 0 deploy the slots in order
    const sf::Keyboard::Key keys[10] = { sf::Keyboard::Key::Num1, sf::Keyboard::Key::Num2, sf::Keyboard::Key::Num3,
        sf::Keyboard::Key::Num4, sf::Keyboard::Key::Num5, sf::Keyboard::Key::Num6, sf::Keyboard::Key::Num7,
        sf::Keyboard::Key::Num8, sf::Keyboard::Key::Num9, sf::Keyboard::Key::Num0 };

    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            sf::Vector2f pos{
                startX + col * (iconWidth + spacingX),
                startY + row * (iconHeight + spacingY)
            };

            auto icon = std::make_shared<UIBattleIcon>(pos, "assets/images/textures/icons/empty.png");

            int uid = equippedUnits[index];
            int form = m_dataLoader->get_player_data().lock()->get_unit_form(uid);

            if (uid >= 0)
            {
                // Load the unit's texture and data
                std::string texturePath = m_dataLoader->get_unit_icon_texture_path(uid, form);
                std::shared_ptr<EntityData> unitData = m_dataLoader->get_unit_data(uid, form);

                icon->set_texture(texturePath);
                icon->set_uid(uid);
                icon->set_cost(unitData->cost);
                icon->set_max_cooldown(unitData->cooldown);
                icon->set_hotkey(keys[index]);
                icon->set_key_label(std::to_string((index + 1) % 10));

                std::weak_ptr<EntityData> weakData = unitData;

                // The callback is stored inside the icon, so it must not hold a shared_ptr
                // to that same icon (it would keep itself alive forever)
                std::weak_ptr<UIBattleIcon> weakIcon = icon;

                // Set callback to spawn unit when icon is clicked
                icon->set_callback([this, weakIcon, weakData]()
                    {
                        auto icon = weakIcon.lock();
                        if (!icon) return;
                        if (icon->get_if_on_cooldown()) { icon->shake(); UIElement::play_sound("ui_error"); return; }

                        if (auto data = weakData.lock())
                        {
                            auto stage = m_stage.lock();
                            if (!stage) return;

                            // The stage's units limit: nothing spawns, the slot doesn't start its cooldown
                            if (stage->is_unit_limit_reached())
                            {
                                show_message("UNIT LIMIT REACHED");
                                icon->shake();
                                UIElement::play_sound("ui_error");
                                return;
                            }
                            if (stage->get_cash() < data->cost) { icon->shake(); UIElement::play_sound("ui_error"); return; }

                            // The stage plays the deploy sound
                            if (stage->spawn_unit(data)) icon->start_cooldown();
                            else UIElement::play_sound("ui_error");
                        }
                    });
            }
            else
            {
                // No unit equipped in this slot
                icon->set_uid(-1);
                icon->set_cost(-1);
                icon->set_max_cooldown(-1);
            }

            m_icons[index] = icon;
            index++;
        }
    }
}

/*
 * Updates each icon's state:
 * - Darkens the slots the wallet can't pay yet, and how close it is (the slots show their cooldown themselves)
 * - Updates cooldown timers
 */
void UIDeploymentIcons::update(float deltaTime)
{
    auto stage = m_stage.lock();
    for (auto& icon : m_icons)
    {
        if (!icon) continue;
        // Not enough cash (the slot shows its cooldown itself); how full the wallet is toward the price
        const int cash = stage ? stage->get_cash() : 0;
        icon->set_darkened(icon->get_uid() < 0 || cash < icon->get_cost());
        icon->set_affordability(icon->get_cost() > 0 ? static_cast<float>(cash) / icon->get_cost() : 1.f);
        icon->update(deltaTime);
    }

    m_messageTime = std::max(0.f, m_messageTime - deltaTime);
}

/*
 * Draws all icons to the window if visible, and the message over them while it lasts.
 */
void UIDeploymentIcons::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    for (auto& icon : m_icons)
    {
        if (icon) icon->render(window);
    }

    if (m_messageTime > 0.f)
    {
        // Pops in bigger, settles, fades out over its last 0.4 seconds
        const float age = MESSAGE_SECONDS - m_messageTime;
        const float scale = 1.f + 0.25f * std::max(0.f, 1.f - age / 0.15f);
        const std::uint8_t alpha = static_cast<std::uint8_t>(255.f * std::min(1.f, m_messageTime / 0.4f));

        const sf::FloatRect slots = get_bounds();
        UISkin::set_text_anchor(m_message, { 0.5f, 1.f });
        m_message.setScale({ scale, scale });
        m_message.setPosition({ slots.position.x + slots.size.x / 2.f, slots.position.y - 14.f });
        m_message.setFillColor(sf::Color(MESSAGE_COLOR.r, MESSAGE_COLOR.g, MESSAGE_COLOR.b, alpha));
        m_message.setOutlineColor(sf::Color(UISkin::OUTLINE.r, UISkin::OUTLINE.g, UISkin::OUTLINE.b, alpha));
        window.draw(m_message);
    }
}

/*
 * Delegates input events to each icon for interaction handling.
 */
void UIDeploymentIcons::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    for (auto& icon : m_icons)
    {
        if (icon) icon->handle_event(event, window);
    }
}

/*
 * Returns the bounding rectangle that encompasses all icons.
 * Useful for detecting if the mouse is over the deployment area.
 */
sf::FloatRect UIDeploymentIcons::get_bounds() const
{
    bool first = true;
    float minX = 0.f, minY = 0.f;
    float maxX = 0.f, maxY = 0.f;

    for (const auto& icon : m_icons)
    {
        if (!icon) continue;
        sf::FloatRect b = icon->get_bounds();

        float bx = b.position.x;
        float by = b.position.y;
        float bw = b.size.x;
        float bh = b.size.y;

        if (first)
        {
            minX = bx;
            minY = by;
            maxX = bx + bw;
            maxY = by + bh;
            first = false;
        }
        else
        {
            minX = std::min(minX, bx);
            minY = std::min(minY, by);
            maxX = std::max(maxX, bx + bw);
            maxY = std::max(maxY, by + bh);
        }
    }

    if (first) // No icons
        return sf::FloatRect({ 0.f, 0.f }, { 0.f, 0.f });

    return sf::FloatRect(
        { minX, minY },
        { maxX - minX, maxY - minY }
    );
}

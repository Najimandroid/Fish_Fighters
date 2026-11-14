#include "UIClamStorage.h"
#include "UIConfirmDialog.h"
#include "UIFlyingIcons.h"
#include "UIPlayerShellsInfo.h"
#include "UIToast.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"
#include "../../data/PlayerData.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>

namespace
{
    const float PI = 3.14159265f;

    // Like a storage room: the fish swim around the whole clam; a tap on one brings up its card (centered, over the
    // dimmed clam); USE ALL / SELL ALL and how many fish wait along the bottom
    const sf::FloatRect TANK{ { 40.f, 120.f }, { 1840.f, 810.f } };     // Where the fish swim
    const sf::FloatRect PANEL{ { 960.f - 265.f, 150.f }, { 530.f, 750.f } };   // The card of the kind picked
    const sf::Vector2f BULK_SIZE{ 300.f, 92.f };
    const float BULK_Y = 960.f;
    const float PANEL_X = PANEL.position.x + 30.f, PANEL_CENTER = PANEL.position.x + PANEL.size.x / 2.f;
    const float TEXT_WIDTH = PANEL.size.x - 60.f;
    const sf::FloatRect CHECKBOX{ { PANEL_X, 488.f }, { 38.f, 38.f } };

    const sf::Vector2f FISH_SIZE{ 140.f, 90.f };   // A floater's box (its sprite fits inside)
    const float WALK_FRAME_SECONDS = 0.12f;

    const sf::Color INSIDE_PINK{ 70, 22, 52 };        // What the dive into the clam ends on (UIClamRoom): the fade in starts there
    const sf::Color NACRE{ 255, 170, 220 };
    const float FADE_IN = 0.4f;
    const sf::Color PICKED{ 255, 222, 48 };         // The ring around a picked fish (UISkin::NUMBER)
    const sf::Color CORAL{ 255, 128, 104 };
    // The clam's inner face, and the pearl card's mother-of-pearl and ink
    const sf::Color SHELL_CENTER{ 178, 96, 150 }, SHELL_EDGE{ 46, 12, 38 };
    // Deep pearl (light text on it): a pale card looked washed out in the dark clam
    const sf::Color CARD_TOP{ 146, 62, 116 }, CARD_BOTTOM{ 92, 30, 76 };
    const sf::Color INK{ 255, 238, 248 }, INK_SOFT{ 238, 190, 222 }, EXTRA_INK{ 214, 166, 255 }, WARNING_INK{ 255, 136, 116 };
    const float CARD_CORNER = 44.f;
}

UIClamStorage::UIClamStorage(std::shared_ptr<DataLoader> dataLoader) :
    UIElement({ 1920.f, 1080.f }, { 0.f, 0.f }),
    m_dataLoader(std::move(dataLoader)),
    m_use({ PANEL.size.x - 60.f, 90.f }, { PANEL_X, 640.f }, "USE"),
    m_sellShells({ 225.f, 80.f }, { PANEL_X, 772.f }, ""),
    m_sellScales({ 225.f, 80.f }, { PANEL_X + 245.f, 772.f }, ""),
    m_closeCard(84.f, PANEL.position + sf::Vector2f{ PANEL.size.x - 54.f, -30.f }, "icon_close"),
    m_useAll(BULK_SIZE, { 960.f - BULK_SIZE.x - 15.f, BULK_Y }, "USE ALL"),
    m_sellAll(BULK_SIZE, { 960.f + 15.f, BULK_Y }, "SELL ALL"),
    m_text(UISkin::heavy_font())
{
    m_use.set_text_size(44);
    m_use.set_click_sound("");   // Its own sounds
    m_use.set_callback([this]() { use_selected(); });
    for (UIButtonElement* sell : { &m_sellShells, &m_sellScales })
    {
        sell->set_style(UIButtonElement::Style::Coral);   // No metal inside the clam
        sell->set_text_size(34);
    }
    m_sellShells.set_callback([this]() { ask_sell(false); });
    m_closeCard.set_click_sound("ui_back");
    m_closeCard.set_callback([this]() { clear_selection(); });
    m_useAll.set_text_size(40);
    m_useAll.set_click_sound("");   // Its own sounds
    m_useAll.set_callback([this]() { use_all(); });
    m_sellAll.set_text_size(40);
    m_sellAll.set_style(UIButtonElement::Style::Coral);
    m_sellAll.set_callback([this]() { ask_sell_all(); });
    m_sellScales.set_callback([this]() { ask_sell(true); });

    for (int i = 0; i < 18; ++i)
        m_bubbles.push_back({ { random(TANK.position.x + 20.f, TANK.position.x + TANK.size.x - 20.f), random(TANK.position.y, TANK.position.y + TANK.size.y) },
            random(25.f, 70.f), random(2.f, 6.f) });
    sync_floaters();
}

void UIClamStorage::set_counters(std::shared_ptr<UIPlayerShellsInfo> shells, std::shared_ptr<UIPlayerShellsInfo> fishScales)
{
    m_shellsPill = shells;
    m_fishScalesPill = fishScales;
}

void UIClamStorage::set_flying_icons(std::shared_ptr<UIFlyingIcons> flying)
{
    m_flying = flying;
}

void UIClamStorage::set_confirm_dialog(std::shared_ptr<UIConfirmDialog> dialog)
{
    m_dialog = dialog;
}

float UIClamStorage::random(float from, float to)
{
    return std::uniform_real_distribution<float>(from, std::max(from, to))(m_random);
}

int UIClamStorage::shells_each(int uid) const
{
    auto data = m_dataLoader->get_unit_data(uid, 0);
    return data ? data->baseUpgradeCost * PlayerData::SPARE_FISH_SHELLS_FACTOR : 0;
}

/*
 * As many floaters of each kind as stored fish: new ones appear somewhere in the water, extra ones go
 */
void UIClamStorage::sync_floaters()
{
    auto player = m_dataLoader->get_player_data().lock();
    if (!player) return;

    std::map<int, int> shown;
    for (const Floater& floater : m_floaters) shown[floater.uid] += 1;
    for (const auto& [uid, count] : player->clamStorage)
        for (int i = shown[uid]; i < count; ++i)
        {
            Floater floater;
            floater.uid = uid;
            floater.position = { random(TANK.position.x + 90.f, TANK.position.x + TANK.size.x - 90.f),
                random(TANK.position.y + 70.f, TANK.position.y + TANK.size.y - 70.f) };
            const float angle = random(0.f, 2.f * PI);
            floater.velocity = sf::Vector2f{ std::cos(angle), std::sin(angle) * 0.4f } * random(25.f, 55.f);
            floater.turnIn = random(1.5f, 4.f);
            floater.phase = random(0.f, 10.f);
            m_floaters.push_back(floater);
        }
    for (auto& [uid, count] : shown)
    {
        int extra = count - player->get_stored_count(uid);
        for (auto it = m_floaters.end(); extra > 0 && it != m_floaters.begin(); )
        {
            --it;
            if (it->uid == uid) { it = m_floaters.erase(it); --extra; }
        }
    }
    if (m_selectedUid >= 0 && player->get_stored_count(m_selectedUid) == 0) clear_selection();
}

UIClamStorage::Floater* UIClamStorage::floater_at(sf::Vector2f point)
{
    // The one drawn on top first
    for (auto it = m_floaters.rbegin(); it != m_floaters.rend(); ++it)
    {
        const sf::FloatRect box{ it->position - FISH_SIZE / 2.f, FISH_SIZE };
        if (box.contains(point)) return &*it;
    }
    return nullptr;
}

void UIClamStorage::clear_selection()
{
    for (Floater& floater : m_floaters) floater.isSelected = false;
    m_selectedUid = -1;
}

/*
 * A click on a fish: a new kind starts the selection again (all of it with "Select all"), the same kind adds or
 * removes this one
 */
void UIClamStorage::click_fish(Floater& floater)
{
    if (floater.uid != m_selectedUid)
    {
        clear_selection();
        m_selectedUid = floater.uid;
        floater.isSelected = true;
        if (m_selectAll) set_select_all(true);
    }
    else
    {
        floater.isSelected = !floater.isSelected;
        if (!floater.isSelected) m_selectAll = false;
        if (selected_count() == 0) m_selectedUid = -1;
    }
    play_sound("ui_select");
}

void UIClamStorage::set_select_all(bool selectAll)
{
    m_selectAll = selectAll;
    if (m_selectedUid < 0) return;
    bool isFirst = true;
    for (Floater& floater : m_floaters)
    {
        if (floater.uid != m_selectedUid) continue;
        // Unticked: only the first one picked stays
        floater.isSelected = selectAll || (floater.isSelected && isFirst);
        if (floater.isSelected) isFirst = false;
    }
}

int UIClamStorage::selected_count() const
{
    return static_cast<int>(std::count_if(m_floaters.begin(), m_floaters.end(), [](const Floater& floater) { return floater.isSelected; }));
}

/*
 * USE: unlocks the fish or adds extra levels (as many of the picked ones as can be used), then the School Rank's
 * milestones
 */
void UIClamStorage::use_selected()
{
    auto player = m_dataLoader->get_player_data().lock();
    if (!player || m_selectedUid < 0) return;
    const PlayerData::StoredUse use = player->preview_use(m_selectedUid, selected_count());
    if (use.usable <= 0) { play_sound("ui_error"); return; }

    // The fish used rise as bubbles
    int left = use.usable;
    for (Floater& floater : m_floaters)
        if (floater.isSelected && left-- > 0)
            for (int i = 0; i < 6; ++i) m_bubbles.push_back({ floater.position + sf::Vector2f{ random(-40.f, 40.f), random(-20.f, 20.f) }, random(90.f, 160.f), random(4.f, 9.f), true });

    player->use_stored_fish(m_selectedUid, use.usable);
    play_sound(use.unlocks ? "unlock" : "purchase");
    if (use.unlocks)
    {
        auto data = m_dataLoader->get_unit_data(m_selectedUid, 0);
        UIToast::show("NEW FISH!", (data ? data->name : std::string("A fish")) + " joined your tank", "");
    }
    UIToast::announce_school_rank_rewards(*player);
    m_dataLoader->save_progress();

    // The used ones leave the water (the picked ones first)
    int toRemove = use.usable;
    std::erase_if(m_floaters, [&](const Floater& floater) { return floater.isSelected && toRemove-- > 0; });
    sync_floaters();
}

/*
 * SELL asks first: how much it brings, and (for Fish Scales) what Fish Scales are for
 */
void UIClamStorage::ask_sell(bool forFishScales)
{
    auto player = m_dataLoader->get_player_data().lock();
    auto dialog = m_dialog.lock();
    const int count = selected_count();
    if (!player || !dialog || m_selectedUid < 0 || count <= 0) return;

    auto data = m_dataLoader->get_unit_data(m_selectedUid, 0);
    const std::string name = data ? data->name : "fish";
    const std::string question = "Sell " + std::to_string(count) + " " + name + "?";
    std::string explanation = forFishScales
        ? "For " + std::to_string(count) + " Fish Scale" + (count > 1 ? "s" : "") + ". 10 Fish Scales craft a Shiny Scale: the + next to your Shiny Scales."
        : "For " + std::to_string(count * shells_each(m_selectedUid)) + " shells.";
    if (!player->is_unit_owned(m_selectedUid) && count >= player->get_stored_count(m_selectedUid))
        explanation += " You don't have this fish in your tank yet!";
    dialog->set_texts(question, explanation, "SELL");
    dialog->open([this, forFishScales]() { sell_selected(forFishScales); });
}

void UIClamStorage::sell_selected(bool forFishScales)
{
    auto player = m_dataLoader->get_player_data().lock();
    if (!player || m_selectedUid < 0) return;
    const int count = selected_count();
    const int each = shells_each(m_selectedUid);
    const int sold = forFishScales ? player->sell_stored_fish_for_scales(m_selectedUid, count)
        : player->sell_stored_fish_for_shells(m_selectedUid, count, each);
    if (sold <= 0) return;

    // What they bring flies out of each fish into its pill (the pill counts it as it lands)
    auto flying = m_flying.lock();
    int index = 0;
    for (const Floater& floater : m_floaters)
    {
        if (!floater.isSelected || index >= sold) continue;
        const float delay = 0.05f * static_cast<float>(index);
        const bool isLast = index == sold - 1;
        if (flying && forFishScales)
            flying->launch("icon_fish_scale", floater.position, m_fishScalesPill.lock(), 1, 1, delay, isLast ? "purchase" : "");
        else if (flying)
            flying->launch("icon_shell", floater.position, m_shellsPill.lock(), each, 4, delay, isLast ? "shell_collect" : "");
        ++index;
    }
    m_dataLoader->save_progress();

    std::erase_if(m_floaters, [](const Floater& floater) { return floater.isSelected; });
    sync_floaters();
}

void UIClamStorage::update(float deltaTime)
{
    m_time += deltaTime;

    // The fish wander, turning now and then, bouncing off the glass
    const sf::FloatRect area{ TANK.position + sf::Vector2f{ 80.f, 60.f }, TANK.size - sf::Vector2f{ 160.f, 120.f } };
    for (Floater& floater : m_floaters)
    {
        floater.turnIn -= deltaTime;
        if (floater.turnIn <= 0.f)
        {
            const float angle = random(0.f, 2.f * PI);
            floater.velocity = sf::Vector2f{ std::cos(angle), std::sin(angle) * 0.4f } * random(25.f, 55.f);
            floater.turnIn = random(1.5f, 4.f);
        }
        floater.position += floater.velocity * deltaTime;
        if (floater.position.x < area.position.x || floater.position.x > area.position.x + area.size.x) floater.velocity.x = -floater.velocity.x;
        if (floater.position.y < area.position.y || floater.position.y > area.position.y + area.size.y) floater.velocity.y = -floater.velocity.y;
        floater.position.x = std::clamp(floater.position.x, area.position.x, area.position.x + area.size.x);
        floater.position.y = std::clamp(floater.position.y, area.position.y, area.position.y + area.size.y);
    }

    // Bubbles rise, and come back from the bottom
    for (Bubble& bubble : m_bubbles)
    {
        bubble.position.y -= bubble.speed * deltaTime;
        bubble.position.x += std::sin(m_time * 2.f + bubble.radius) * 10.f * deltaTime;
    }
    std::erase_if(m_bubbles, [](const Bubble& bubble) { return bubble.isBurst && bubble.position.y < TANK.position.y + 10.f; });
    for (Bubble& bubble : m_bubbles)
        if (bubble.position.y < TANK.position.y + 10.f) bubble.position = { random(TANK.position.x + 20.f, TANK.position.x + TANK.size.x - 20.f), TANK.position.y + TANK.size.y - 10.f };

    // The buttons follow the selection
    auto player = m_dataLoader->get_player_data().lock();
    const int count = selected_count();
    const bool hasPick = player && m_selectedUid >= 0 && count > 0;
    const PlayerData::StoredUse use = hasPick ? player->preview_use(m_selectedUid, count) : PlayerData::StoredUse{};
    m_use.set_enabled(use.usable > 0);
    m_use.set_text(use.usable > 1 ? "USE " + std::to_string(use.usable) : "USE");
    m_sellShells.set_enabled(hasPick);
    m_sellScales.set_enabled(hasPick);
    // Centered right of their icon (the leading spaces)
    m_sellShells.set_text(hasPick ? "    +" + std::to_string(count * shells_each(m_selectedUid)) : "");
    m_sellScales.set_text(hasPick ? "    +" + std::to_string(count) : "");
    m_use.update(deltaTime);
    m_sellShells.update(deltaTime);
    m_sellScales.update(deltaTime);
    m_closeCard.update(deltaTime);

    // USE ALL: when at least one stored fish can be used; SELL ALL: when any wait
    bool canUse = false;
    if (player) for (const auto& [uid, stored] : player->clamStorage) canUse = canUse || player->preview_use(uid, stored).usable > 0;
    m_useAll.set_enabled(canUse);
    m_sellAll.set_enabled(stored_total() > 0);
    m_useAll.update(deltaTime);
    m_sellAll.update(deltaTime);
}

/*
 * A fish of the tank: its swimming frames cropped to the fish, facing the way it swims, bobbing; a ring when picked
 */
void UIClamStorage::render_floater(sf::RenderWindow& window, const Floater& floater, float scale) const
{
    auto data = m_dataLoader->get_unit_data(floater.uid, 0);
    if (!data || data->texture.empty()) return;
    const UISkin::SwimFrames& swim = UISkin::swim_frames(data->texture, data->frameCount, data->knockbackFrameIndex);
    if (swim.box.size.x <= 0 || swim.box.size.y <= 0) return;

    const sf::Vector2f position = floater.position + sf::Vector2f{ 0.f, std::sin(m_time * 2.f + floater.phase) * 5.f };
    if (floater.isSelected)
    {
        const float radius = 62.f * scale;
        sf::CircleShape ring(radius);
        ring.setOrigin({ radius, radius });
        ring.setPosition(position);
        ring.setFillColor(sf::Color(PICKED.r, PICKED.g, PICKED.b, 50));
        ring.setOutlineThickness(5.f);
        ring.setOutlineColor(PICKED);
        window.draw(ring);
    }

    const int frame = static_cast<int>((m_time + floater.phase) / WALK_FRAME_SECONDS) % std::max(1, swim.count);
    sf::Sprite fish(UISkin::texture_file(data->texture), { { frame * swim.frameWidth + swim.box.position.x, swim.box.position.y }, swim.box.size });
    const sf::Vector2f boxSize(swim.box.size);
    const float fit = std::min(FISH_SIZE.x / boxSize.x, FISH_SIZE.y / boxSize.y) * scale;
    fish.setOrigin(boxSize / 2.f);
    fish.setScale({ floater.velocity.x > 0.f ? -fit : fit, fit });   // The sheets face left
    fish.setPosition(position);
    window.draw(fish);
}

void UIClamStorage::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    const sf::FloatRect screen = UISkin::screen_rect();   // A phone's extra width too
    const float top = screen.position.y, bottom = top + screen.size.y;

    // The clam's inner face: deep plum at its edges, pearly pink where the light falls (on the fish)
    UISkin::fill_screen(window, SHELL_EDGE);
    const sf::Vector2f glowCenter = TANK.position + TANK.size / 2.f;
    const int segments = 64;
    sf::VertexArray face(sf::PrimitiveType::TriangleFan, segments + 2);
    face[0] = sf::Vertex{ glowCenter, SHELL_CENTER };
    for (int i = 0; i <= segments; ++i)
    {
        const float angle = static_cast<float>(i) / segments * 2.f * PI;
        face[i + 1] = sf::Vertex{ glowCenter + sf::Vector2f{ std::cos(angle) * screen.size.x * 0.62f, std::sin(angle) * screen.size.y * 0.8f }, SHELL_EDGE };
    }
    window.draw(face);

    // Its ribs: faint ridges fanning out from the hinge below the screen, like the inside of a scallop
    const sf::Vector2f hinge{ 960.f, bottom + 480.f };
    for (int rib = -10; rib <= 10; ++rib)
    {
        const float angle = static_cast<float>(rib) * 7.f * PI / 180.f;
        const sf::Vector2f direction{ std::sin(angle), -std::cos(angle) };
        const sf::Vector2f side{ -direction.y, direction.x };
        const float length = 2300.f;
        const sf::Color color = rib % 2 == 0 ? sf::Color(255, 214, 238, 16) : sf::Color(30, 0, 24, 30);
        sf::ConvexShape ridge(4);
        ridge.setPoint(0, hinge + side * 3.f);
        ridge.setPoint(1, hinge + direction * length + side * 70.f);
        ridge.setPoint(2, hinge + direction * length - side * 70.f);
        ridge.setPoint(3, hinge - side * 3.f);
        ridge.setFillColor(color);
        window.draw(ridge);
    }

    // Pearly glows drifting slowly
    for (int glow = 0; glow < 4; ++glow)
    {
        const float phase = static_cast<float>(glow) * 1.7f;
        const sf::Vector2f center{ 960.f + std::sin(m_time * 0.11f + phase) * 700.f, 560.f + std::cos(m_time * 0.08f + phase * 1.3f) * 300.f };
        for (int ring = 6; ring >= 1; --ring)
        {
            const float radius = 70.f * static_cast<float>(ring);
            sf::CircleShape circle(radius);
            circle.setOrigin({ radius, radius });
            circle.setPosition(center);
            circle.setFillColor(sf::Color(NACRE.r, NACRE.g, NACRE.b, 6));
            window.draw(circle);
        }
    }

    // Bubbles drifting up through the light, then the fish, free in it (no box: they're in the clam)
    for (const Bubble& bubble : m_bubbles)
    {
        sf::CircleShape circle(bubble.radius);
        circle.setOrigin({ bubble.radius, bubble.radius });
        circle.setPosition(bubble.position);
        circle.setFillColor(sf::Color(255, 235, 248, 40));
        circle.setOutlineThickness(1.5f);
        circle.setOutlineColor(sf::Color(255, 240, 250, 130));
        window.draw(circle);
    }
    for (const Floater& floater : m_floaters) render_floater(window, floater, 1.f);
    if (m_floaters.empty())
    {
        UISkin::style_text(m_text, 36, sf::Color(255, 220, 240));
        m_text.setString("The fish out of the clam wait here");
        UISkin::set_text_anchor(m_text, { 0.5f, 0.5f });
        m_text.setPosition(TANK.position + TANK.size / 2.f);
        window.draw(m_text);
    }

    // The shell's white lips: scallops under the title bar and along the bottom (the clam's rim around us)
    for (const auto& [y, radius, spacing] : { std::tuple{ 92.f, 54.f, 104.f }, std::tuple{ bottom + 34.f, 74.f, 132.f } })
        for (float x = screen.position.x - 20.f; x < screen.position.x + screen.size.x + 40.f; x += spacing)
        {
            sf::CircleShape scallop(radius);
            scallop.setOrigin({ radius, radius });
            scallop.setPosition({ x, y });
            scallop.setFillColor(sf::Color(226, 186, 214));   // The shell's rim: pearly, not glaring white
            scallop.setOutlineThickness(6.f);
            scallop.setOutlineColor(UISkin::OUTLINE);
            window.draw(scallop);
        }

    // Along the bottom: USE ALL, SELL ALL, how many fish wait; under the title, what a tap does
    if (!m_floaters.empty())
    {
        m_useAll.render(window);
        m_sellAll.render(window);
        const int total = stored_total();
        UISkin::style_text(m_text, 34, sf::Color(255, 220, 240));
        m_text.setString(std::to_string(total) + " fish");
        UISkin::set_text_anchor(m_text, { 1.f, 0.5f });
        m_text.setPosition({ 1872.f, BULK_Y + BULK_SIZE.y / 2.f });
        window.draw(m_text);
        UISkin::style_text(m_text, 26, sf::Color(255, 214, 238));
        m_text.setString("Tap a fish to use or sell it");
        UISkin::set_text_anchor(m_text, { 0.5f, 0.5f });
        m_text.setPosition({ 960.f, 182.f });   // Under the shell's rim
        window.draw(m_text);
    }

    // The card of the kind picked, over the dimmed clam
    if (is_card_open())
    {
        UISkin::fill_screen(window, sf::Color(30, 0, 24, 150));
        render_panel(window);
        m_closeCard.render(window);
    }

    // Coming in from the dive into the clam: out of its dark pink
    if (m_time < FADE_IN)
    {
        UISkin::fill_screen(window, sf::Color(INSIDE_PINK.r, INSIDE_PINK.g, INSIDE_PINK.b, static_cast<std::uint8_t>(255.f * (1.f - m_time / FADE_IN))));
    }
}

/*
 * The pearl card (mother-of-pearl, dark ink): the kind picked (swimming), its name and level, how many are picked,
 * "Select all", what USE does, USE, SELL for shells / for Fish Scales
 */
void UIClamStorage::render_panel(sf::RenderWindow& window)
{
    // The card: a soft pink pearl, lighter at the top, a dark outline and its shadow down-right
    sf::ConvexShape card = UISkin::rounded_rect({ PANEL.position + sf::Vector2f{ UISkin::SHADOW_SHIFT, UISkin::SHADOW_DROP } * 1.5f, PANEL.size }, CARD_CORNER);
    card.setFillColor(sf::Color(30, 0, 24, 110));
    window.draw(card);
    card = UISkin::rounded_rect(PANEL, CARD_CORNER);
    card.setFillColor(CARD_BOTTOM);
    card.setOutlineThickness(6.f);
    card.setOutlineColor(UISkin::OUTLINE);
    window.draw(card);
    // Lighter towards the top: a few soft layers make the gradient (no hard edge)
    for (const float reach : { 0.62f, 0.46f, 0.3f })
    {
        sf::ConvexShape light = UISkin::rounded_rect({ PANEL.position + sf::Vector2f{ 8.f, 8.f }, { PANEL.size.x - 16.f, PANEL.size.y * reach } }, CARD_CORNER - 8.f);
        light.setFillColor(sf::Color(CARD_TOP.r, CARD_TOP.g, CARD_TOP.b, 90));
        window.draw(light);
    }
    // A sheen across it (mother-of-pearl catches the light)
    sf::ConvexShape sheen(4);
    sheen.setPoint(0, PANEL.position + sf::Vector2f{ PANEL.size.x * 0.55f, 10.f });
    sheen.setPoint(1, PANEL.position + sf::Vector2f{ PANEL.size.x * 0.75f, 10.f });
    sheen.setPoint(2, PANEL.position + sf::Vector2f{ PANEL.size.x * 0.35f, PANEL.size.y * 0.5f });
    sheen.setPoint(3, PANEL.position + sf::Vector2f{ PANEL.size.x * 0.15f, PANEL.size.y * 0.5f });
    sheen.setFillColor(sf::Color(255, 220, 245, 30));
    window.draw(sheen);

    auto player = m_dataLoader->get_player_data().lock();
    // Ink on the card (no outline), or the outlined game text
    auto text = [&](const std::string& string, unsigned int size, sf::Color color, sf::Vector2f position, sf::Vector2f anchor, bool isBody = false, bool isOutlined = false)
        {
            if (isBody) UISkin::style_body_text(m_text, size, color);
            else UISkin::style_text(m_text, size, color);
            if (!isOutlined && isBody) m_text.setOutlineThickness(0.f);   // Titles keep the game's outline
            m_text.setString(string);
            UISkin::set_text_anchor(m_text, anchor);
            m_text.setPosition(position);
            window.draw(m_text);
            return m_text.getGlobalBounds();
        };

    const int count = selected_count();
    if (!player || m_selectedUid < 0 || count == 0) return;   // The card only comes up for a pick

    // The kind picked, swimming
    Floater portrait;
    portrait.uid = m_selectedUid;
    portrait.position = { PANEL_CENTER, 255.f };
    portrait.velocity = { -1.f, 0.f };
    render_floater(window, portrait, 1.6f);

    auto data = m_dataLoader->get_unit_data(m_selectedUid, player->get_unit_form(m_selectedUid));
    text(data ? data->name : "?", 42, INK, { PANEL_CENTER, 360.f }, { 0.5f, 0.5f });
    if (player->is_unit_owned(m_selectedUid))
    {
        const std::string level = "LEVEL " + std::to_string(player->get_unit_level(m_selectedUid));
        const std::string extra = "+" + std::to_string(player->get_unit_extra_levels(m_selectedUid));
        sf::Text measure(UISkin::heavy_font(), level + " " + extra);
        UISkin::style_text(measure, 32, INK);
        const float left = PANEL_CENTER - measure.getLocalBounds().size.x / 2.f;
        const sf::FloatRect levelBounds = text(level, 32, INK_SOFT, { left, 405.f }, { 0.f, 0.5f });
        text(extra, 32, EXTRA_INK, { levelBounds.position.x + levelBounds.size.x + 10.f, 405.f }, { 0.f, 0.5f });
    }
    else text("NOT IN YOUR TANK YET", 30, WARNING_INK, { PANEL_CENTER, 405.f }, { 0.5f, 0.5f });

    const int ofKind = player->get_stored_count(m_selectedUid);
    text("Picked: " + std::to_string(count) + " of " + std::to_string(ofKind), 30, INK_SOFT, { PANEL_CENTER, 450.f }, { 0.5f, 0.5f });

    // "Select all": a pearl box, coral with a white tick when on
    sf::ConvexShape box = UISkin::rounded_rect(CHECKBOX, 9.f);
    box.setFillColor(m_selectAll ? sf::Color(242, 118, 158) : sf::Color(70, 22, 58));
    box.setOutlineThickness(4.f);
    box.setOutlineColor(UISkin::OUTLINE);
    window.draw(box);
    if (m_selectAll)
    {
        const sf::Vector2f c = CHECKBOX.position + CHECKBOX.size / 2.f;
        for (const auto& [from, to] : { std::pair{ sf::Vector2f{ -11.f, 1.f }, sf::Vector2f{ -3.f, 9.f } }, std::pair{ sf::Vector2f{ -3.f, 9.f }, sf::Vector2f{ 12.f, -8.f } } })
        {
            const sf::Vector2f delta = to - from;
            sf::RectangleShape stroke({ std::hypot(delta.x, delta.y) + 6.f, 6.f });
            stroke.setOrigin({ 3.f, 3.f });
            stroke.setPosition(c + from);
            stroke.setRotation(sf::radians(std::atan2(delta.y, delta.x)));
            stroke.setFillColor(sf::Color::White);
            window.draw(stroke);
        }
    }
    text("Select all " + std::to_string(ofKind), 30, INK, { CHECKBOX.position.x + CHECKBOX.size.x + 16.f, CHECKBOX.position.y + CHECKBOX.size.y / 2.f }, { 0.f, 0.5f });

    // What USE would do
    const PlayerData::StoredUse use = player->preview_use(m_selectedUid, count);
    const int extraNow = player->get_unit_extra_levels(m_selectedUid);
    std::string what;
    if (use.usable <= 0)
        what = "Its extra levels are maxed (+" + std::to_string(PlayerData::MAX_EXTRA_LEVELS) + "): these can only be sold.";
    else if (use.unlocks)
        what = "USE: it joins your tank!" + (use.extraLevels > 0 ? " The others: +" + std::to_string(use.extraLevels) + " extra levels." : std::string());
    else
        what = "USE: +" + std::to_string(use.extraLevels) + " extra level" + (use.extraLevels > 1 ? "s" : "") + " (+" + std::to_string(extraNow)
            + " to +" + std::to_string(extraNow + use.extraLevels) + ").";
    if (use.usable > 0 && use.usable < count)
        what += " Only " + std::to_string(use.usable) + " can be used (+" + std::to_string(PlayerData::MAX_EXTRA_LEVELS) + " is the max): sell the others.";
    text(UISkin::wrap_text(what, 26, TEXT_WIDTH, UISkin::regular_font()), 26, use.usable > 0 ? INK : WARNING_INK,
        { PANEL_CENTER, 548.f }, { 0.5f, 0.f }, true);

    m_use.render(window);

    // SELL FOR: shells, Fish Scales (their icons on the coral buttons)
    text("SELL FOR", 26, INK_SOFT, { PANEL_CENTER, 750.f }, { 0.5f, 0.5f });
    m_sellShells.render(window);
    m_sellScales.render(window);
    for (const auto& [button, icon] : { std::pair<UIButtonElement*, const char*>{ &m_sellShells, "icon_shell" }, { &m_sellScales, "icon_fish_scale" } })
    {
        const sf::FloatRect bounds = button->get_bounds();
        UISkin::draw_fitted(window, UISkin::texture(icon), { { bounds.position.x + 10.f, bounds.position.y + bounds.size.y / 2.f - 24.f }, { 48.f, 48.f } });
    }
    text("10 Fish Scales craft a Shiny Scale (+)", 22, INK_SOFT, { PANEL_CENTER, 872.f }, { 0.5f, 0.5f }, true);
}

bool UIClamStorage::is_card_open() const
{
    return m_selectedUid >= 0 && selected_count() > 0;
}

int UIClamStorage::stored_total() const
{
    int total = 0;
    if (auto player = m_dataLoader->get_player_data().lock())
        for (const auto& [uid, count] : player->clamStorage) total += count;
    return total;
}

/*
 * USE ALL: every kind's usable fish, at once (new fish join the tank, the others give extra levels up to the max)
 */
void UIClamStorage::use_all()
{
    auto player = m_dataLoader->get_player_data().lock();
    if (!player) return;
    int used = 0;
    bool hasUnlocked = false;
    const std::map<int, int> stored = player->clamStorage;   // A copy: using them changes it
    for (const auto& [uid, count] : stored)
    {
        const PlayerData::StoredUse use = player->preview_use(uid, count);
        if (use.usable <= 0) continue;
        int left = use.usable;   // The fish used rise as bubbles
        for (const Floater& floater : m_floaters)
            if (floater.uid == uid && left-- > 0)
                for (int i = 0; i < 4; ++i) m_bubbles.push_back({ floater.position + sf::Vector2f{ random(-40.f, 40.f), random(-20.f, 20.f) }, random(90.f, 160.f), random(4.f, 9.f), true });
        player->use_stored_fish(uid, use.usable);
        used += use.usable;
        if (use.unlocks)
        {
            hasUnlocked = true;
            auto data = m_dataLoader->get_unit_data(uid, 0);
            UIToast::show("NEW FISH!", (data ? data->name : std::string("A fish")) + " joined your tank", "");
        }
    }
    if (used == 0) { play_sound("ui_error"); return; }
    play_sound(hasUnlocked ? "unlock" : "purchase");
    UIToast::announce_school_rank_rewards(*player);
    m_dataLoader->save_progress();
    clear_selection();
    sync_floaters();
}

// SELL ALL asks first: how many shells, and whether some fish aren't in the tank yet
void UIClamStorage::ask_sell_all()
{
    auto player = m_dataLoader->get_player_data().lock();
    auto dialog = m_dialog.lock();
    const int total = stored_total();
    if (!player || !dialog || total <= 0) return;
    int shells = 0;
    bool hasNewFish = false;
    for (const auto& [uid, count] : player->clamStorage)
    {
        shells += count * shells_each(uid);
        hasNewFish = hasNewFish || !player->is_unit_owned(uid);
    }
    std::string explanation = "For " + std::to_string(shells) + " shells.";
    if (hasNewFish) explanation += " Some of them aren't in your tank yet!";
    dialog->set_texts("Sell all " + std::to_string(total) + " fish?", explanation, "SELL ALL");
    dialog->open([this]() { sell_all(); });
}

void UIClamStorage::sell_all()
{
    auto player = m_dataLoader->get_player_data().lock();
    if (!player) return;
    auto flying = m_flying.lock();
    const std::map<int, int> stored = player->clamStorage;
    int index = 0;
    for (const auto& [uid, count] : stored)
    {
        const int each = shells_each(uid);
        int left = player->sell_stored_fish_for_shells(uid, count, each);
        // Their shells fly out of each fish into the pill (it counts them as they land)
        for (const Floater& floater : m_floaters)
            if (floater.uid == uid && left-- > 0 && flying)
                flying->launch("icon_shell", floater.position, m_shellsPill.lock(), each, 2, 0.04f * static_cast<float>(index++), "");
    }
    play_sound("shell_collect");
    m_dataLoader->save_progress();
    clear_selection();
    m_floaters.clear();
    sync_floaters();
}

void UIClamStorage::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    // The card is up: it takes the taps (its buttons, its X, "Select all"); a tap outside it closes it
    if (is_card_open())
    {
        m_use.handle_event(event, window);
        m_sellShells.handle_event(event, window);
        m_sellScales.handle_event(event, window);
        m_closeCard.handle_event(event, window);
        const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>();
        if (!pressed || pressed->button != sf::Mouse::Button::Left || !is_card_open()) return;
        const sf::Vector2f mouse = window.mapPixelToCoords(pressed->position);
        const sf::FloatRect checkRow{ CHECKBOX.position, { TEXT_WIDTH, CHECKBOX.size.y } };
        if (checkRow.contains(mouse))
        {
            set_select_all(!m_selectAll);
            play_sound("ui_click");
        }
        else if (!PANEL.contains(mouse) && !m_closeCard.get_bounds().contains(mouse))
        {
            clear_selection();
            play_sound("ui_back");
        }
        return;
    }

    // Else: USE ALL / SELL ALL, or a fish (its card comes up)
    m_useAll.handle_event(event, window);
    m_sellAll.handle_event(event, window);
    const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>();
    if (!pressed || pressed->button != sf::Mouse::Button::Left) return;
    if (Floater* floater = floater_at(window.mapPixelToCoords(pressed->position))) click_fish(*floater);
}

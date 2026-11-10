#include "UIEncyclopedia.h"
#include "../UISkin.h"
#include "../EntityPortraits.h"
#include "../../core/DataLoader.h"
#include "../../core/combat/AbilityGlossary.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>

namespace
{
    // Layout
    // The grid stops above the back button (bottom left, 120 px + 24 px margin): 5 rows, scrolling
    const sf::FloatRect GRID_PANEL{ { 40.f, 232.f }, { 840.f, 656.f } };
    const sf::FloatRect DETAIL_PANEL{ { 910.f, 232.f }, { 970.f, 818.f } };
    const sf::Vector2f SLOT_SIZE{ 150.f, 112.f };
    const float SLOT_GAP = 12.f;
    const int COLUMNS = 5;
    const sf::Vector2f GRID_ORIGIN{ GRID_PANEL.position.x + 21.f, GRID_PANEL.position.y + 24.f };
    const float SLOT_FRAME_SCALE = SLOT_SIZE.y / 124.f;   // Slots look like the deploy slots (124 px tall)

    const sf::FloatRect PORTRAIT{ { DETAIL_PANEL.position.x + 40.f, DETAIL_PANEL.position.y + 40.f }, { 360.f, 270.f } };
    const float INFO_X = PORTRAIT.position.x + PORTRAIT.size.x + 30.f;
    const float TEXT_X = DETAIL_PANEL.position.x + 40.f;
    const float TEXT_WIDTH = DETAIL_PANEL.size.x - 80.f;
    const float STAT_COLUMN_WIDTH = 420.f;
    const float STAT_ROW_HEIGHT = 42.f;

    const sf::Color DIMMED_BUTTON{ 140, 150, 160 };

    const char* SORT_NAMES[] = { "No.", "Name", "Health", "Attack", "Range", "Speed" };

    std::string format_number(float value, int decimals)
    {
        char text[32];
        std::snprintf(text, sizeof(text), "%.*f", decimals, value);
        return text;
    }
}

UIEncyclopedia::UIEncyclopedia(std::shared_ptr<DataLoader> dataLoader, Book book) :
    UIElement({ 1920.f, 1080.f }, { 0.f, 0.f }),
    m_dataLoader(dataLoader),
    m_fishTab({ 200.f, 84.f }, { 40.f, 128.f }, "FISH"),
    m_enemiesTab({ 260.f, 84.f }, { 254.f, 128.f }, "ENEMIES"),
    m_sortLabel(UISkin::heavy_font(), "SORT"),
    m_gridPanel(GRID_PANEL.size, GRID_PANEL.position, UISkin::PANEL_DARK, 1.2f),
    m_detailPanel(DETAIL_PANEL.size, DETAIL_PANEL.position, UISkin::PANEL_DARK, 1.2f),
    m_text(UISkin::heavy_font())
{
    m_fishTab.set_text_size(40);
    m_enemiesTab.set_text_size(40);
    m_fishTab.set_callback([this]() { open_book(Book::FISH); });
    m_enemiesTab.set_callback([this]() { open_book(Book::ENEMIES); });

    UISkin::style_text(m_sortLabel, 30, UISkin::TITLE);
    UISkin::set_text_anchor(m_sortLabel, { 0.f, 0.5f });
    m_sortLabel.setPosition({ 560.f, 170.f });

    // Sort buttons: pick a key, click it again to reverse the order
    for (int key = 0; key < static_cast<int>(SortKey::COUNT); ++key)
    {
        auto button = std::make_unique<UIButtonElement>(sf::Vector2f{ 196.f, 72.f }, sf::Vector2f{ 660.f + key * 204.f, 134.f }, SORT_NAMES[key]);
        button->set_text_size(28);
        button->set_callback([this, key]()
            {
                const SortKey sortKey = static_cast<SortKey>(key);
                if (sortKey == m_sortKey) m_isDescending = !m_isDescending;
                else { m_sortKey = sortKey; m_isDescending = sortKey != SortKey::NUMBER && sortKey != SortKey::NAME; }
                sort_entries();
                refresh_buttons();
            });
        m_sortButtons.push_back(std::move(button));
    }

    UISkin::style_text(m_text, 28);
    m_text.setLineSpacing(1.1f);

    open_book(book);
}

void UIEncyclopedia::open_book(Book book)
{
    m_book = book;
    m_scrollRow = 0;
    build_entries();
    sort_entries();
    select(m_entries.empty() ? -1 : 0);
    refresh_buttons();
}

/*
 * Every unit (its current form) or every enemy, known or not
 */
void UIEncyclopedia::build_entries()
{
    m_entries.clear();
    auto player = m_dataLoader->get_player_data().lock();

    if (m_book == Book::FISH)
    {
        std::set<int> uids;
        for (const auto& [key, data] : m_dataLoader->get_units_database()) uids.insert(key.first);

        for (int uid : uids)
        {
            Entry entry;
            entry.uid = uid;
            entry.isKnown = player && player->is_unit_owned(uid);
            const int form = entry.isKnown ? player->get_unit_form(uid) : 0;
            entry.level = entry.isKnown ? player->get_unit_level(uid) : 0;
            entry.data = m_dataLoader->get_unit_data(uid, form);
            entry.iconPath = m_dataLoader->get_unit_icon_texture_path(uid, form);
            if (entry.data) m_entries.push_back(entry);
        }
    }
    else
    {
        for (const auto& [uid, data] : m_dataLoader->get_enemies_database())
        {
            Entry entry;
            entry.uid = uid;
            entry.data = data;
            entry.isKnown = player && player->has_seen_enemy(uid);
            m_entries.push_back(entry);
        }
    }
}

float UIEncyclopedia::health_of(const Entry& entry) const
{
    return static_cast<float>(entry.data->health) * (m_book == Book::FISH ? PlayerData::stat_multiplier(entry.level) : 1.f);
}

float UIEncyclopedia::attack_of(const Entry& entry) const
{
    return static_cast<float>(entry.data->attackPower) * (m_book == Book::FISH ? PlayerData::stat_multiplier(entry.level) : 1.f);
}

/*
 * Sorts by the key (unknown ones always last, except by number), keeping the selection
 */
void UIEncyclopedia::sort_entries()
{
    const int selectedUid = (m_selected >= 0 && m_selected < static_cast<int>(m_entries.size())) ? m_entries[m_selected].uid : -1;

    std::stable_sort(m_entries.begin(), m_entries.end(), [this](const Entry& a, const Entry& b)
        {
            if (m_sortKey != SortKey::NUMBER && a.isKnown != b.isKnown) return a.isKnown;

            auto compare = [this](auto valueA, auto valueB) { return m_isDescending ? valueA > valueB : valueA < valueB; };
            switch (m_sortKey)
            {
            case SortKey::NAME:
                if (a.data->name != b.data->name) return compare(a.data->name, b.data->name);
                break;
            case SortKey::HEALTH:
                if (health_of(a) != health_of(b)) return compare(health_of(a), health_of(b));
                break;
            case SortKey::ATTACK:
                if (attack_of(a) != attack_of(b)) return compare(attack_of(a), attack_of(b));
                break;
            case SortKey::RANGE:
                if (a.data->attackRange != b.data->attackRange) return compare(a.data->attackRange, b.data->attackRange);
                break;
            case SortKey::SPEED:
                if (a.data->movementSpeed != b.data->movementSpeed) return compare(a.data->movementSpeed, b.data->movementSpeed);
                break;
            default:
                return compare(a.uid, b.uid);
            }
            return a.uid < b.uid;
        });

    m_selected = -1;
    for (size_t i = 0; i < m_entries.size(); ++i)
        if (m_entries[i].uid == selectedUid) m_selected = static_cast<int>(i);
}

void UIEncyclopedia::select(int index)
{
    m_selected = index;
    if (index >= 0 && index < static_cast<int>(m_entries.size()) && m_entries[index].isKnown)
        m_abilities.set_abilities(m_entries[index].data->abilities);
    else
        m_abilities.set_abilities({});

    // Keep the selection in view
    if (index >= 0)
    {
        const int row = index / COLUMNS;
        if (row < m_scrollRow) m_scrollRow = row;
        if (row >= m_scrollRow + visible_rows()) m_scrollRow = row - visible_rows() + 1;
    }
}

/*
 * Tabs and sort buttons: the active ones yellow, the others dimmed; the active sort shows its direction
 */
void UIEncyclopedia::refresh_buttons()
{
    m_fishTab.set_fill_color(m_book == Book::FISH ? sf::Color::White : DIMMED_BUTTON);
    m_enemiesTab.set_fill_color(m_book == Book::ENEMIES ? sf::Color::White : DIMMED_BUTTON);

    for (int key = 0; key < static_cast<int>(m_sortButtons.size()); ++key)
    {
        const bool isActive = key == static_cast<int>(m_sortKey);
        m_sortButtons[key]->set_fill_color(isActive ? sf::Color::White : DIMMED_BUTTON);
        sf::String label = SORT_NAMES[key];
        if (isActive) label += m_isDescending ? sf::String(U" \u25BC") : sf::String(U" \u25B2");
        m_sortButtons[key]->set_label(label);
    }
}

int UIEncyclopedia::visible_rows() const
{
    return static_cast<int>((GRID_PANEL.size.y - 48.f + SLOT_GAP) / (SLOT_SIZE.y + SLOT_GAP));
}

int UIEncyclopedia::row_count() const
{
    return (static_cast<int>(m_entries.size()) + COLUMNS - 1) / COLUMNS;
}

sf::FloatRect UIEncyclopedia::slot_rect(int index) const
{
    const int column = index % COLUMNS;
    const int row = index / COLUMNS - m_scrollRow;
    return { { GRID_ORIGIN.x + column * (SLOT_SIZE.x + SLOT_GAP), GRID_ORIGIN.y + row * (SLOT_SIZE.y + SLOT_GAP) }, SLOT_SIZE };
}

void UIEncyclopedia::update(float deltaTime)
{
    m_fishTab.update(deltaTime);
    m_enemiesTab.update(deltaTime);
    for (auto& button : m_sortButtons) button->update(deltaTime);
}

void UIEncyclopedia::render_portrait(sf::RenderWindow& window, const Entry& entry, const sf::FloatRect& area)
{
    if (m_book == Book::FISH)
    {
        EntityPortraits::draw_unit(window, entry.iconPath, area, !entry.isKnown);
    }
    else
    {
        const float padding = area.size.y * 0.06f;
        EntityPortraits::draw_enemy(window, *entry.data,
            { area.position + sf::Vector2f{ padding, padding }, area.size - sf::Vector2f{ padding, padding } * 2.f }, !entry.isKnown);
    }
}

void UIEncyclopedia::render_grid(sf::RenderWindow& window)
{
    m_gridPanel.render(window);

    const int first = m_scrollRow * COLUMNS;
    const int last = std::min(static_cast<int>(m_entries.size()), first + visible_rows() * COLUMNS);
    for (int i = first; i < last; ++i)
    {
        const sf::FloatRect slot = slot_rect(i);
        if (i == m_selected)
        {
            const float border = 7.f;
            UISkin::draw_nine_slice(window, UISkin::BUTTON_YELLOW,
                { slot.position - sf::Vector2f{ border, border }, slot.size + sf::Vector2f{ border, border } * 2.f }, 0.5f);
        }
        UISkin::draw_nine_slice(window, UISkin::SLOT, slot, SLOT_FRAME_SCALE);
        render_portrait(window, m_entries[i], UISkin::slot_inner(slot, SLOT_FRAME_SCALE));
        UISkin::draw_nine_slice(window, UISkin::SLOT_FRAME, slot, SLOT_FRAME_SCALE);
    }

    // Scroll position (when everything doesn't fit)
    if (row_count() > visible_rows())
    {
        const float trackTop = GRID_PANEL.position.y + 24.f;
        const float trackHeight = GRID_PANEL.size.y - 48.f;
        const float barHeight = trackHeight * visible_rows() / row_count();
        const float barTop = trackTop + (trackHeight - barHeight) * m_scrollRow / std::max(1, row_count() - visible_rows());
        sf::RectangleShape bar({ 8.f, barHeight });
        bar.setPosition({ GRID_PANEL.position.x + GRID_PANEL.size.x - 14.f, barTop });
        bar.setFillColor(UISkin::NUMBER);
        window.draw(bar);
    }
}

void UIEncyclopedia::render_details(sf::RenderWindow& window)
{
    m_detailPanel.render(window);
    if (m_selected < 0 || m_selected >= static_cast<int>(m_entries.size())) return;

    const Entry& entry = m_entries[m_selected];
    const EntityData& data = *entry.data;

    // Portrait
    UISkin::draw_nine_slice(window, UISkin::SLOT, PORTRAIT, 2.f);
    render_portrait(window, entry, UISkin::slot_inner(PORTRAIT, 2.f));
    UISkin::draw_nine_slice(window, UISkin::SLOT_FRAME, PORTRAIT, 2.f);

    auto drawText = [&](const std::string& text, sf::Vector2f position, unsigned int size, sf::Color color, sf::Vector2f anchor = { 0.f, 0.5f })
        {
            UISkin::style_text(m_text, size, color);
            m_text.setString(text);
            UISkin::set_text_anchor(m_text, anchor);
            m_text.setPosition(position);
            window.draw(m_text);
            return m_text.getGlobalBounds();
        };

    const float top = PORTRAIT.position.y;
    drawText(entry.isKnown ? data.name : "???", { INFO_X, top + 36.f }, 44, UISkin::NUMBER);
    drawText("No. " + std::to_string(entry.uid), { INFO_X, top + 88.f }, 28, UISkin::TITLE);

    if (!entry.isKnown)
    {
        const std::string hint = m_book == Book::FISH ? "Not unlocked yet." : "Not met yet: meet it in battle to learn about it.";
        drawText(UISkin::wrap_text(hint, 28, TEXT_WIDTH), { TEXT_X, PORTRAIT.position.y + PORTRAIT.size.y + 40.f }, 28, UISkin::TEXT, { 0.f, 0.f });
        return;
    }

    // Level (fish) or types (enemies)
    if (m_book == Book::FISH)
    {
        drawText("LEVEL " + std::to_string(entry.level), { INFO_X, top + 140.f }, 32, UISkin::NUMBER);
    }
    else
    {
        float x = INFO_X;
        const sf::Texture& typeSheet = UISkin::texture_file(AbilityGlossary::TYPE_ICONS_TEXTURE);
        for (EntityType type : data.types)
        {
            UISkin::draw_fitted(window, typeSheet, UISkin::icon_cell(typeSheet, AbilityGlossary::type_icon_index(type), AbilityGlossary::ICON_CELL_SIZE),
                { { x, top + 140.f - 20.f }, { 40.f, 40.f } });
            const sf::FloatRect name = drawText(AbilityGlossary::type_name(type), { x + 48.f, top + 140.f }, 30, UIAbilityList::type_color(type));
            x = name.position.x + name.size.x + 24.f;
        }
    }

    // Description
    const sf::FloatRect description = drawText(UISkin::wrap_text(data.description, 28, TEXT_WIDTH),
        { TEXT_X, PORTRAIT.position.y + PORTRAIT.size.y + 30.f }, 28, UISkin::TEXT, { 0.f, 0.f });

    // Stats in two columns
    std::vector<std::pair<std::string, std::string>> stats = {
        { "Health", format_number(health_of(entry), 0) },
        { "Attack", format_number(attack_of(entry), 0) },
        { "Attack rate", format_number(data.attackFrequency, 1) + " s" },
        { "Range", format_number(data.attackRange, 0) },
        { "Speed", format_number(data.movementSpeed, 1) },
        { "Knockbacks", std::to_string(data.knockbackCount) },
        { "Targets", static_cast<int>(data.attackType) == 1 ? "Single" : "Area" },
    };
    if (m_book == Book::FISH)
    {
        stats.push_back({ "Cost", std::to_string(data.cost) + "$" });
        stats.push_back({ "Cooldown", format_number(data.cooldown, 1) + " s" });
    }

    const float statsTop = std::max(description.position.y + description.size.y + 30.f, PORTRAIT.position.y + PORTRAIT.size.y + 110.f);
    for (size_t i = 0; i < stats.size(); ++i)
    {
        const float x = TEXT_X + static_cast<float>(i % 2) * (STAT_COLUMN_WIDTH + 50.f);
        const float y = statsTop + static_cast<float>(i / 2) * STAT_ROW_HEIGHT + STAT_ROW_HEIGHT / 2.f;
        drawText(stats[i].first, { x, y }, 28, UISkin::TITLE);
        drawText(stats[i].second, { x + STAT_COLUMN_WIDTH, y }, 28, UISkin::NUMBER, { 1.f, 0.5f });
    }

    // Abilities under the stats
    const float statsBottom = statsTop + static_cast<float>((stats.size() + 1) / 2) * STAT_ROW_HEIGHT;
    m_abilities.set_position({ TEXT_X, statsBottom + 16.f });
}

void UIEncyclopedia::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    m_fishTab.render(window);
    m_enemiesTab.render(window);
    window.draw(m_sortLabel);
    for (auto& button : m_sortButtons) button->render(window);

    render_grid(window);
    render_details(window);
    m_abilities.render(window);   // Last: its bubble goes over everything
}

void UIEncyclopedia::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    m_fishTab.handle_event(event, window);
    m_enemiesTab.handle_event(event, window);
    for (auto& button : m_sortButtons) button->handle_event(event, window);
    m_abilities.handle_event(event, window);

    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mousePressed->button != sf::Mouse::Button::Left) return;
        const sf::Vector2f mouse = window.mapPixelToCoords(mousePressed->position);
        const int first = m_scrollRow * COLUMNS;
        const int last = std::min(static_cast<int>(m_entries.size()), first + visible_rows() * COLUMNS);
        for (int i = first; i < last; ++i)
            if (slot_rect(i).contains(mouse))
            {
                if (i != m_selected) play_sound("ui_select");
                select(i);
                break;
            }
    }
    else if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>())
    {
        if (GRID_PANEL.contains(window.mapPixelToCoords(wheel->position)))
        {
            const int maxRow = std::max(0, row_count() - visible_rows());
            m_scrollRow = std::clamp(m_scrollRow - static_cast<int>(std::round(wheel->delta)), 0, maxRow);
        }
    }
    else if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        // Arrow keys move the selection in the grid
        int next = m_selected;
        if (keyPressed->code == sf::Keyboard::Key::Left) next -= 1;
        else if (keyPressed->code == sf::Keyboard::Key::Right) next += 1;
        else if (keyPressed->code == sf::Keyboard::Key::Up) next -= COLUMNS;
        else if (keyPressed->code == sf::Keyboard::Key::Down) next += COLUMNS;
        if (next != m_selected && next >= 0 && next < static_cast<int>(m_entries.size()))
        {
            play_sound("ui_select");
            select(next);
        }
    }
}

#include "UIEncyclopedia.h"
#include "../UISkin.h"
#include "../EntityPortraits.h"
#include "../../core/DataLoader.h"
#include "../../core/combat/AbilityGlossary.h"
#include "../../data/ChapterData.h"
#include "../../data/EnemyStageData.h"
#include "../../data/RewardData.h"
#include "../../data/StageData.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>

namespace
{
    // Layout
    // Both panels down to the bottom bar (the back button sits on the grid panel's corner): 5 rows centered, scrolling
    const sf::FloatRect GRID_PANEL{ { 40.f, 232.f }, { 840.f, UISkin::CONTENT_BOTTOM - 16.f - 232.f } };
    const sf::FloatRect DETAIL_PANEL{ { 910.f, 232.f }, { 970.f, UISkin::CONTENT_BOTTOM - 16.f - 232.f } };   // Down to the bottom bar
    const sf::Vector2f SLOT_SIZE{ 150.f, 112.f };
    const float SLOT_GAP = 12.f;
    const int COLUMNS = 5;
    const float GRID_TOP_PADDING = std::floor((GRID_PANEL.size.y - (5.f * SLOT_SIZE.y + 4.f * SLOT_GAP)) / 2.f);
    const sf::Vector2f GRID_ORIGIN{ GRID_PANEL.position.x + 21.f, GRID_PANEL.position.y + GRID_TOP_PADDING };
    const float SLOT_FRAME_SCALE = SLOT_SIZE.y / 124.f;   // Slots look like the deploy slots (124 px tall)

    const sf::FloatRect PORTRAIT{ { DETAIL_PANEL.position.x + 40.f, DETAIL_PANEL.position.y + 40.f }, { 360.f, 270.f } };
    const float INFO_X = PORTRAIT.position.x + PORTRAIT.size.x + 30.f;
    const float TEXT_X = DETAIL_PANEL.position.x + 40.f;
    const float TEXT_WIDTH = DETAIL_PANEL.size.x - 80.f;
    const float STAT_COLUMN_WIDTH = 420.f;
    const float STAT_ROW_HEIGHT = 42.f;


    const char* SORT_NAMES[] = { "No.", "Name", "Health", "Attack", "Range", "Speed" };
    // The sort pill and its direction button on the tabs' row, right of them; the list opens under the pill
    const sf::FloatRect SORT_PILL{ { 540.f, 159.f }, { 264.f, 64.f } };
    const sf::FloatRect SORT_DIRECTION{ { 816.f, 159.f }, { 64.f, 64.f } };
    const float SORT_ROW = 58.f;
    const sf::Color RARE_GOLD{ 255, 204, 80 }, SUPER_RARE_PINK{ 255, 128, 214 };
    const int FORM_LEVELS[] = { 1, 10, 30 };   // The level each form comes at (as on the Upgrade screen)
    const sf::Vector2f FORM_BUTTON{ 150.f, 52.f };

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
    m_fishTab({ 200.f, 86.f }, { 40.f, 148.f }, "FISH"),          // Folder tabs on the grid panel (drawn by render_tab)
    m_enemiesTab({ 260.f, 86.f }, { 250.f, 148.f }, "ENEMIES"),
    m_sortPill(SORT_PILL.size, SORT_PILL.position, "Sort: No."),
    m_sortDirection(SORT_DIRECTION.size, SORT_DIRECTION.position, "^"),
    m_gridPanel(GRID_PANEL.size, GRID_PANEL.position, UISkin::PANEL_DARK, 1.2f),
    m_detailPanel(DETAIL_PANEL.size, DETAIL_PANEL.position, UISkin::PANEL_DARK, 1.2f),
    m_text(UISkin::heavy_font())
{
    m_fishTab.set_text_size(40);
    m_enemiesTab.set_text_size(40);
    m_fishTab.set_callback([this]() { open_book(Book::FISH); });
    m_enemiesTab.set_callback([this]() { open_book(Book::ENEMIES); });

    // The sort pill opens its list, the arrow next to it reverses the order
    m_sortPill.set_text_size(30);
    m_sortPill.set_style(UIButtonElement::Style::Secondary);
    m_sortPill.set_callback([this]() { m_isSortOpen = !m_isSortOpen; });
    m_sortDirection.set_text_size(30);
    m_sortDirection.set_style(UIButtonElement::Style::Secondary);
    m_sortDirection.set_callback([this]()
        {
            m_isDescending = !m_isDescending;
            sort_entries();
            refresh_buttons();
        });

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
    const std::map<int, int> numbers = encounter_numbers();
    const auto number_of = [&numbers](int uid) { const auto it = numbers.find(uid); return it != numbers.end() ? it->second : 9999; };

    if (m_book == Book::FISH)
    {
        std::set<int> uids;
        for (const auto& [key, data] : m_dataLoader->get_units_database()) uids.insert(key.first);

        for (int uid : uids)
        {
            Entry entry;
            entry.uid = uid;
            entry.number = number_of(uid);
            entry.isKnown = player && player->is_unit_owned(uid);
            const int form = entry.isKnown ? player->get_unit_form(uid) : 0;
            entry.level = entry.isKnown ? player->get_unit_level(uid) : 0;
            entry.extraLevels = entry.isKnown ? player->get_unit_extra_levels(uid) : 0;
            entry.data = m_dataLoader->get_unit_data(uid, form);
            entry.iconPath = m_dataLoader->get_unit_icon_texture_path(uid, form);
            entry.form = entry.currentForm = form;
            entry.formCount = 0;
            for (const auto& [key, data] : m_dataLoader->get_units_database()) if (key.first == uid) entry.formCount++;
            if (entry.data) m_entries.push_back(entry);
        }
    }
    else
    {
        for (const auto& [uid, data] : m_dataLoader->get_enemies_database())
        {
            Entry entry;
            entry.uid = uid;
            entry.number = number_of(uid);
            entry.data = data;
            entry.isKnown = player && player->has_seen_enemy(uid);
            entry.iconPath = m_dataLoader->get_enemy_icon_texture_path(uid);
            m_entries.push_back(entry);
        }
    }
}

/*
 * The No. of each fish or enemy: the order the game brings them. Fish: the first fish, then the ones the stages give
 * (chapter after chapter, stage after stage), the other Normal fish, then the Rare and Super Rare ones (the clam's).
 * Enemies: as the stages send them (each stage's in the order they start coming), then any other
 */
std::map<int, int> UIEncyclopedia::encounter_numbers() const
{
    std::map<int, int> numbers;
    const auto add = [&numbers](int uid) { if (uid >= 0 && !numbers.count(uid)) numbers.emplace(uid, static_cast<int>(numbers.size()) + 1); };

    std::vector<std::shared_ptr<StageData>> stages;
    for (int chapterUid : m_dataLoader->get_chapter_uids())
        if (auto chapter = m_dataLoader->get_chapter_data(chapterUid))
            for (const auto& stageOnMap : chapter->stages)
                if (auto stage = m_dataLoader->get_stage_data(stageOnMap.UID)) stages.push_back(stage);

    if (m_book == Book::FISH)
    {
        add(1);   // The first fish (every new game starts with it)
        for (const auto& stage : stages)
            for (const auto& reward : stage->rewards)
                if (reward && reward->type == "unit") add(reward->unitUID);
        std::map<int, std::vector<int>> byRarity;   // The others: Normal, then Rare, then Super Rare, by UID
        for (const auto& [key, data] : m_dataLoader->get_units_database())
            if (key.second == 0 && data) byRarity[static_cast<int>(data->rarity)].push_back(key.first);
        for (auto& [rarity, uids] : byRarity)
        {
            std::sort(uids.begin(), uids.end());
            for (int uid : uids) add(uid);
        }
    }
    else
    {
        for (const auto& stage : stages)
        {
            std::vector<std::shared_ptr<EnemyStageData>> spawns = stage->enemies;
            std::stable_sort(spawns.begin(), spawns.end(), [](const auto& a, const auto& b) { return a->spawnStart < b->spawnStart; });
            for (const auto& spawn : spawns) add(spawn->UID);
        }
        std::vector<int> others;
        for (const auto& [uid, data] : m_dataLoader->get_enemies_database()) others.push_back(uid);
        std::sort(others.begin(), others.end());
        for (int uid : others) add(uid);
    }
    return numbers;
}
float UIEncyclopedia::health_of(const Entry& entry) const
{
    return static_cast<float>(entry.data->health) * (m_book == Book::FISH ? PlayerData::stat_multiplier(entry.level + entry.extraLevels) : 1.f);
}

float UIEncyclopedia::attack_of(const Entry& entry) const
{
    return static_cast<float>(entry.data->attackPower) * (m_book == Book::FISH ? PlayerData::stat_multiplier(entry.level + entry.extraLevels) : 1.f);
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
                return compare(a.number, b.number);
            }
            return a.number < b.number;
        });

    m_selected = -1;
    for (size_t i = 0; i < m_entries.size(); ++i)
        if (m_entries[i].uid == selectedUid) m_selected = static_cast<int>(i);
}

void UIEncyclopedia::select(int index)
{
    m_selected = index;
    m_detailScroll = 0.f;
    discover_selected();
    if (index >= 0 && index < static_cast<int>(m_entries.size()) && m_entries[index].isKnown)
        m_abilities.set_abilities(m_entries[index].data->abilities);
    else
        m_abilities.set_abilities({});
    build_form_buttons();

    // Keep the selection in view
    if (index >= 0)
    {
        const int row = index / COLUMNS;
        if (row < m_scrollRow) m_scrollRow = row;
        if (row >= m_scrollRow + visible_rows()) m_scrollRow = row - visible_rows() + 1;
    }
}

/*
 * An owned fish looked at for the first time: its Pearls (PlayerData::PEARLS_PER_NEW_FISH), saved, shown rising over
 * the portrait
 */
void UIEncyclopedia::discover_selected()
{
    if (m_book != Book::FISH || m_selected < 0 || m_selected >= static_cast<int>(m_entries.size()) || !m_entries[m_selected].isKnown) return;
    auto player = m_dataLoader->get_player_data().lock();
    if (!player || !player->discover_fish(m_entries[m_selected].uid)) return;
    m_dataLoader->save_progress();
    m_rewardAmount = PlayerData::PEARLS_PER_NEW_FISH;
    m_rewardTime = 2.f;
    play_sound("purchase");
}

void UIEncyclopedia::show_form(int form)
{
    if (m_selected < 0 || m_selected >= static_cast<int>(m_entries.size())) return;
    Entry& entry = m_entries[m_selected];
    auto data = m_dataLoader->get_unit_data(entry.uid, form);
    if (!data) return;
    entry.form = form;
    entry.data = data;
    m_detailScroll = 0.f;
    entry.iconPath = m_dataLoader->get_unit_icon_texture_path(entry.uid, form);
    m_abilities.set_abilities(data->abilities);
    build_form_buttons();
}

/*
 * One button per form of the selected fish (known fish with more than one form), the shown one yellow
 */
void UIEncyclopedia::build_form_buttons()
{
    m_formButtons.clear();
    if (m_book != Book::FISH || m_selected < 0 || m_selected >= static_cast<int>(m_entries.size())) return;
    const Entry& entry = m_entries[m_selected];
    if (!entry.isKnown || entry.formCount < 2) return;

    for (int form = 0; form < entry.formCount; ++form)
    {
        // A form not reached yet stays a secret: "???", it can't be shown
        const bool isReached = form <= entry.currentForm;
        auto button = std::make_unique<UIButtonElement>(FORM_BUTTON,
            sf::Vector2f{ INFO_X + form * (FORM_BUTTON.x + 12.f), PORTRAIT.position.y + 178.f }, isReached ? "FORM " + std::to_string(form + 1) : "???");
        button->set_text_size(26);
        button->set_style(form == entry.form ? UIButtonElement::Style::Primary : UIButtonElement::Style::Secondary);
        button->set_enabled(isReached);
        button->set_callback([this, form]() { m_pendingForm = form; });
        m_formButtons.push_back(std::move(button));
    }
}

/*
 * Tabs and sort buttons: the active ones yellow, the others dimmed; the active sort shows its direction
 */
void UIEncyclopedia::refresh_buttons()
{
    m_fishTab.set_style(m_book == Book::FISH ? UIButtonElement::Style::Primary : UIButtonElement::Style::Secondary);
    m_enemiesTab.set_style(m_book == Book::ENEMIES ? UIButtonElement::Style::Primary : UIButtonElement::Style::Secondary);

    m_sortPill.set_text(std::string("Sort: ") + SORT_NAMES[static_cast<int>(m_sortKey)]);
    m_sortDirection.set_label(m_isDescending ? sf::String(U"\u25BC") : sf::String(U"\u25B2"));
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
    m_sortPill.update(deltaTime);
    m_sortDirection.update(deltaTime);
    m_rewardTime = std::max(0.f, m_rewardTime - deltaTime);
    for (auto& button : m_formButtons) button->update(deltaTime);
    if (m_pendingForm >= 0) { show_form(m_pendingForm); m_pendingForm = -1; }   // Not from inside a button's callback
}

void UIEncyclopedia::render_portrait(sf::RenderWindow& window, const Entry& entry, const sf::FloatRect& area)
{
    if (m_book == Book::FISH)
    {
        EntityPortraits::draw_unit(window, entry.iconPath, area, !entry.isKnown);
    }
    else
    {
        EntityPortraits::draw_enemy(window, *entry.data, entry.iconPath, area, !entry.isKnown);
    }
}

void UIEncyclopedia::render_tab(sf::RenderWindow& window, const UIButtonElement& tab, const std::string& label, bool isOpen)
{
    const sf::FloatRect bounds = tab.get_bounds();
    sf::FloatRect rect = bounds;
    if (isOpen) rect.size.y = GRID_PANEL.position.y + 16.f - rect.position.y;   // Down over the panel's top edge
    UISkin::draw_nine_slice(window, isOpen ? UISkin::PANEL_DARK : UISkin::BUTTON_STEEL, rect, 0.6f,
        isOpen ? sf::Color::White : sf::Color(140, 140, 142));

    UISkin::style_text(m_text, 40, isOpen ? UISkin::NUMBER : UISkin::TITLE);
    m_text.setString(label);
    UISkin::set_text_anchor(m_text, { 0.5f, 0.5f });
    m_text.setPosition(bounds.position + bounds.size / 2.f);
    window.draw(m_text);
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

        // NEW: an owned fish not looked at yet (its Pearls wait for the first look)
        if (m_book == Book::FISH && m_entries[i].isKnown)
            if (auto player = m_dataLoader->get_player_data().lock(); player && !player->is_fish_discovered(m_entries[i].uid))
            {
                const sf::FloatRect tag{ { slot.position.x + slot.size.x - 66.f, slot.position.y - 10.f }, { 72.f, 32.f } };
                UISkin::draw_nine_slice(window, UISkin::SLOT, tag, 0.3f, sf::Color(255, 96, 120));
                UISkin::style_text(m_text, 22, UISkin::TEXT);
                m_text.setString("NEW");
                UISkin::set_text_anchor(m_text, { 0.5f, 0.5f });
                m_text.setPosition(tag.position + tag.size / 2.f);
                window.draw(m_text);
            }
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

    auto drawText = [&](const std::string& text, sf::Vector2f position, unsigned int size, sf::Color color, sf::Vector2f anchor = { 0.f, 0.5f },
        const sf::Font& font = UISkin::heavy_font())
        {
            UISkin::style_text(m_text, size, color, font);
            m_text.setString(text);
            UISkin::set_text_anchor(m_text, anchor);
            m_text.setPosition(position);
            window.draw(m_text);
            return m_text.getGlobalBounds();
        };

    const float top = PORTRAIT.position.y;
    drawText(entry.isKnown ? data.name : "???", { INFO_X, top + 36.f }, 44, UISkin::NUMBER);
    const sf::FloatRect number = drawText("No. " + std::to_string(entry.number), { INFO_X, top + 88.f }, 28, UISkin::TITLE);
    if (m_book == Book::FISH && entry.isKnown)   // Its rarity after the number, in its colour
    {
        const Rarity rarity = data.rarity;
        drawText(rarity_name(rarity), { number.position.x + number.size.x + 24.f, top + 88.f }, 28,
            rarity == Rarity::SUPER_RARE ? SUPER_RARE_PINK : rarity == Rarity::RARE ? RARE_GOLD : UISkin::TEXT);
    }

    if (!entry.isKnown)
    {
        const std::string hint = m_book == Book::FISH ? "Not unlocked yet." : "Not met yet: meet it in battle to learn about it.";
        drawText(UISkin::wrap_text(hint, 28, TEXT_WIDTH, UISkin::regular_font()), { TEXT_X, PORTRAIT.position.y + PORTRAIT.size.y + 40.f }, 28, UISkin::TEXT,
            { 0.f, 0.f }, UISkin::regular_font());
        m_detailContentHeight = 0.f;
        return;
    }

    // Level (fish) or types (enemies)
    if (m_book == Book::FISH)
    {
        sf::FloatRect levelBounds = drawText("LEVEL " + std::to_string(entry.level), { INFO_X, top + 140.f }, 32, UISkin::NUMBER);
        if (entry.extraLevels > 0)   // The Clam Room's extra levels, purple
            levelBounds = drawText("+" + std::to_string(entry.extraLevels), { levelBounds.position.x + levelBounds.size.x + 8.f, top + 140.f }, 32, UISkin::EXTRA);
        for (auto& button : m_formButtons) button->render(window);
        // The next form's level under the buttons (what it looks like stays a secret)
        if (!m_formButtons.empty() && entry.currentForm + 1 < entry.formCount && entry.currentForm + 1 < static_cast<int>(std::size(FORM_LEVELS)))
            drawText("Evolves at level " + std::to_string(FORM_LEVELS[entry.currentForm + 1]), { INFO_X, top + 254.f }, 24, sf::Color(255, 128, 104));
        (void)levelBounds;
    }
    else
    {
        float x = INFO_X;
        const sf::Texture& typeSheet = UISkin::texture_file(AbilityGlossary::TYPE_ICONS_TEXTURE);
        for (EntityType type : data.types)
        {
            if (type == EntityType::ALL_ENEMIES) continue;   // Hidden: every enemy has it
            UISkin::draw_fitted(window, typeSheet, UISkin::icon_cell(typeSheet, AbilityGlossary::type_icon_index(type), AbilityGlossary::ICON_CELL_SIZE),
                { { x, top + 140.f - 20.f }, { 40.f, 40.f } });
            const sf::FloatRect name = drawText(AbilityGlossary::type_name(type), { x + 48.f, top + 140.f }, 30, UIAbilityList::type_color(type));
            x = name.position.x + name.size.x + 24.f;
        }
    }

    // Under the portrait: the description, the stats and the abilities, clipped to the panel (too long, they scroll)
    const sf::FloatRect area = detail_scroll_rect();
    const sf::View view = window.getView();
    {
        const sf::Vector2f windowSize(window.getSize());
        const sf::Vector2i topLeft = window.mapCoordsToPixel(area.position, view);
        const sf::Vector2i bottomRight = window.mapCoordsToPixel(area.position + area.size, view);
        sf::View clipped = view;
        clipped.setScissor({ { std::clamp(topLeft.x / windowSize.x, 0.f, 1.f), std::clamp(topLeft.y / windowSize.y, 0.f, 1.f) },
            { std::clamp((bottomRight.x - topLeft.x) / windowSize.x, 0.f, 1.f), std::clamp((bottomRight.y - topLeft.y) / windowSize.y, 0.f, 1.f) } });
        window.setView(clipped);
    }
    const float scrolled = std::round(m_detailScroll);

    // Description
    const sf::FloatRect description = drawText(UISkin::wrap_text(data.description, 28, TEXT_WIDTH, UISkin::regular_font()),
        { TEXT_X, area.position.y + 12.f - scrolled }, 28, UISkin::TEXT, { 0.f, 0.f }, UISkin::regular_font());

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

    const float statsTop = description.position.y + description.size.y + 24.f;
    for (size_t i = 0; i < stats.size(); ++i)
    {
        const float x = TEXT_X + static_cast<float>(i % 2) * (STAT_COLUMN_WIDTH + 50.f);
        const float y = statsTop + static_cast<float>(i / 2) * STAT_ROW_HEIGHT + STAT_ROW_HEIGHT / 2.f;
        drawText(stats[i].first, { x, y }, 28, UISkin::TITLE);
        drawText(stats[i].second, { x + STAT_COLUMN_WIDTH, y }, 28, UISkin::NUMBER, { 1.f, 0.5f });
    }

    // Abilities under the stats (their bubble is drawn later, over everything)
    const float statsBottom = statsTop + static_cast<float>((stats.size() + 1) / 2) * STAT_ROW_HEIGHT;
    m_abilities.set_position({ TEXT_X, statsBottom + 12.f });
    m_abilities.render_lines(window);
    window.setView(view);

    // How tall it all is: what doesn't fit scrolls, a bar on the right shows where
    const float contentBottom = statsBottom + 12.f + m_abilities.get_height() + 12.f;
    m_detailContentHeight = contentBottom + scrolled - area.position.y;
    const float hidden = m_detailContentHeight - area.size.y;
    if (hidden > 1.f)
    {
        const float thumbHeight = std::max(50.f, area.size.y * area.size.y / m_detailContentHeight);
        sf::RectangleShape track({ 8.f, area.size.y });
        track.setPosition({ DETAIL_PANEL.position.x + DETAIL_PANEL.size.x - 22.f, area.position.y });
        track.setFillColor(sf::Color(255, 255, 255, 40));
        window.draw(track);
        sf::RectangleShape thumb({ 8.f, thumbHeight });
        thumb.setPosition({ track.getPosition().x, area.position.y + (area.size.y - thumbHeight) * std::clamp(m_detailScroll / hidden, 0.f, 1.f) });
        thumb.setFillColor(UISkin::NUMBER);
        window.draw(thumb);
    }
}

// The part of the detail panel under the portrait (the description, stats and abilities scroll in it)
sf::FloatRect UIEncyclopedia::detail_scroll_rect() const
{
    const float top = PORTRAIT.position.y + PORTRAIT.size.y + 12.f;
    return { { DETAIL_PANEL.position.x + 12.f, top }, { DETAIL_PANEL.size.x - 24.f, DETAIL_PANEL.position.y + DETAIL_PANEL.size.y - 18.f - top } };
}

void UIEncyclopedia::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    m_sortPill.render(window);
    m_sortDirection.render(window);

    // The closed tab behind the panel, the open one over its top edge: it's part of it
    if (m_book == Book::FISH) render_tab(window, m_enemiesTab, "ENEMIES", false);
    else render_tab(window, m_fishTab, "FISH", false);
    render_grid(window);
    if (m_book == Book::FISH) render_tab(window, m_fishTab, "FISH", true);
    else render_tab(window, m_enemiesTab, "ENEMIES", true);
    render_details(window);

    // A first look's Pearls rising over the portrait
    if (m_rewardTime > 0.f)
    {
        const float t = 1.f - m_rewardTime / 2.f;
        const std::uint8_t alpha = static_cast<std::uint8_t>(255 * std::min(1.f, m_rewardTime / 0.5f));
        const sf::Vector2f center{ PORTRAIT.position.x + PORTRAIT.size.x / 2.f, PORTRAIT.position.y + PORTRAIT.size.y * 0.55f - 60.f * t };
        UISkin::draw_fitted(window, UISkin::texture("icon_pearl"), { center - sf::Vector2f{ 120.f, 32.f }, { 64.f, 64.f } }, sf::Color(255, 255, 255, alpha));
        UISkin::style_text(m_text, 52, sf::Color(UISkin::NUMBER.r, UISkin::NUMBER.g, UISkin::NUMBER.b, alpha));
        m_text.setString("+" + std::to_string(m_rewardAmount));
        UISkin::set_text_anchor(m_text, { 0.f, 0.5f });
        m_text.setPosition(center + sf::Vector2f{ -46.f, 0.f });
        window.draw(m_text);
    }
    m_abilities.render_hover_bubble(window);   // Its bubble goes over everything...
    if (m_isSortOpen) render_sort_menu(window);   // ...but the open sort list
}

sf::FloatRect UIEncyclopedia::sort_option_rect(int key) const
{
    return { { SORT_PILL.position.x + 10.f, SORT_PILL.position.y + SORT_PILL.size.y + 22.f + key * SORT_ROW }, { SORT_PILL.size.x - 20.f, SORT_ROW - 6.f } };
}

/*
 * The list of orders under the pill: the current one yellow with its direction
 */
void UIEncyclopedia::render_sort_menu(sf::RenderWindow& window)
{
    const sf::FloatRect first = sort_option_rect(0), last = sort_option_rect(static_cast<int>(SortKey::COUNT) - 1);
    const sf::FloatRect panel{ { SORT_PILL.position.x, first.position.y - 12.f }, { SORT_PILL.size.x, last.position.y + last.size.y + 12.f - (first.position.y - 12.f) } };
    sf::RectangleShape shadow(panel.size);
    shadow.setPosition(panel.position + sf::Vector2f{ 6.f, 10.f });
    shadow.setFillColor(sf::Color(0, 0, 0, 110));
    window.draw(shadow);
    UISkin::draw_nine_slice(window, UISkin::PANEL_DARK, panel, 0.8f);
    for (int key = 0; key < static_cast<int>(SortKey::COUNT); ++key)
    {
        const sf::FloatRect row = sort_option_rect(key);
        const bool isCurrent = key == static_cast<int>(m_sortKey);
        if (isCurrent) UISkin::draw_nine_slice(window, UISkin::BUTTON_YELLOW, row, 0.45f);
        sf::String label = SORT_NAMES[key];
        if (isCurrent) label += m_isDescending ? sf::String(U"  \u25BC") : sf::String(U"  \u25B2");
        UISkin::style_text(m_text, 30, isCurrent ? UISkin::TEXT : UISkin::TITLE);
        m_text.setString(label);
        UISkin::set_text_anchor(m_text, { 0.5f, 0.5f });
        m_text.setPosition(row.position + row.size / 2.f);
        window.draw(m_text);
    }
}

// Picks an order from the list (the current one again: reversed); numbers and names go up, the stats down at first
void UIEncyclopedia::choose_sort(SortKey key)
{
    if (key == m_sortKey) m_isDescending = !m_isDescending;
    else { m_sortKey = key; m_isDescending = key != SortKey::NUMBER && key != SortKey::NAME; }
    m_isSortOpen = false;
    play_sound("ui_select");
    sort_entries();
    refresh_buttons();
}

void UIEncyclopedia::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    // The sort list open: it takes the input (a press elsewhere closes it, the pill too)
    if (m_isSortOpen)
    {
        m_sortPill.handle_event(event, window);
        if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
        {
            const sf::Vector2f mouse = window.mapPixelToCoords(pressed->position);
            bool isOnList = m_sortPill.get_bounds().contains(mouse);
            for (int key = 0; key < static_cast<int>(SortKey::COUNT); ++key) isOnList = isOnList || sort_option_rect(key).contains(mouse);
            if (!isOnList) m_isSortOpen = false;
        }
        else if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>())
        {
            const sf::Vector2f mouse = window.mapPixelToCoords(released->position);
            for (int key = 0; key < static_cast<int>(SortKey::COUNT); ++key)
                if (sort_option_rect(key).contains(mouse)) { choose_sort(static_cast<SortKey>(key)); break; }
        }
        return;
    }

    m_fishTab.handle_event(event, window);
    m_enemiesTab.handle_event(event, window);
    m_sortPill.handle_event(event, window);
    m_sortDirection.handle_event(event, window);
    for (auto& button : m_formButtons) button->handle_event(event, window);
    m_abilities.handle_event(event, window);

    // The details under the portrait: dragged up and down (or the wheel) when they don't fit
    const float detailHidden = std::max(0.f, m_detailContentHeight - detail_scroll_rect().size.y);
    if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>(); pressed && detailHidden > 0.f
        && detail_scroll_rect().contains(window.mapPixelToCoords(pressed->position)))
    {
        m_isDetailDragging = true;
        m_detailPressY = window.mapPixelToCoords(pressed->position).y;
        m_detailPressScroll = m_detailScroll;
    }
    else if (const auto* moved = event.getIf<sf::Event::MouseMoved>(); moved && m_isDetailDragging)
        m_detailScroll = std::clamp(m_detailPressScroll - (window.mapPixelToCoords(moved->position).y - m_detailPressY), 0.f, detailHidden);
    else if (event.is<sf::Event::MouseButtonReleased>())
        m_isDetailDragging = false;
    else if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>(); wheel && detail_scroll_rect().contains(window.mapPixelToCoords(wheel->position)))
        m_detailScroll = std::clamp(m_detailScroll - wheel->delta * 60.f, 0.f, detailHidden);

    const int maxRow = std::max(0, row_count() - visible_rows());
    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mousePressed->button != sf::Mouse::Button::Left) return;
        const sf::Vector2f mouse = window.mapPixelToCoords(mousePressed->position);
        if (GRID_PANEL.contains(mouse) && UISkin::is_slide_area(mouse))
        {
            m_isGridPressed = true;
            m_isGridDragging = false;
            m_gridPress = mouse;
            m_gridPressRow = m_scrollRow;
        }
    }
    else if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>(); mouseMoved && m_isGridPressed)
    {
        // Dragged up and down: the grid follows, a row at a time
        const float dragY = window.mapPixelToCoords(mouseMoved->position).y - m_gridPress.y;
        if (std::abs(dragY) > 18.f) m_isGridDragging = true;
        if (m_isGridDragging)
            m_scrollRow = std::clamp(m_gridPressRow - static_cast<int>(std::lround(dragY / (SLOT_SIZE.y + SLOT_GAP))), 0, maxRow);
    }
    else if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>(); mouseReleased && m_isGridPressed)
    {
        m_isGridPressed = false;
        if (m_isGridDragging) return;
        // A tap: the slot under it
        const int first = m_scrollRow * COLUMNS;
        const int last = std::min(static_cast<int>(m_entries.size()), first + visible_rows() * COLUMNS);
        for (int i = first; i < last; ++i)
            if (slot_rect(i).contains(m_gridPress))
            {
                if (i != m_selected) play_sound("ui_select");
                select(i);
                break;
            }
    }
    else if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>())
    {
        if (GRID_PANEL.contains(window.mapPixelToCoords(wheel->position)))
            m_scrollRow = std::clamp(m_scrollRow - static_cast<int>(std::round(wheel->delta)), 0, maxRow);
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
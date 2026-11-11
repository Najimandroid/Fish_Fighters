#pragma once

#include "UIButtonElement.h"
#include "UIPanel.h"
#include "UIAbilityList.h"

#include <memory>
#include <vector>

class DataLoader;
struct EntityData;

/*
 * UIEncyclopedia
 * --------------
 * The fish and enemy encyclopedia, like The Battle Cats' Cat Guide and Enemy Guide:
 * two tabs (FISH / ENEMIES), sort buttons (number, name, health, attack, range, speed; click the
 * active one again to reverse it), a scrollable grid of portraits, and a panel describing the
 * selected one: name, types, description, stats and abilities (UIAbilityList).
 * Fish the player doesn't own and enemies not met in battle yet are black, with their details hidden.
 * Fish stats are the ones of their current level.
 */

class UIEncyclopedia : public UIElement
{
public:

    enum class Book { FISH, ENEMIES };
    enum class SortKey { NUMBER, NAME, HEALTH, ATTACK, RANGE, SPEED, COUNT };

    UIEncyclopedia(std::shared_ptr<DataLoader> dataLoader, Book book = Book::FISH);

    void open_book(Book book);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

private:

    struct Entry
    {
        int uid = -1;
        std::shared_ptr<EntityData> data;  // Current form for fish
        bool isKnown = false;              // Owned fish, enemies met
        int level = 0;                     // Fish only
        std::string iconPath;              // Fish only
        int form = 0;                      // Fish: the form shown (starts on the current one)
        int currentForm = 0;               // Fish: the form reached
        int formCount = 1;                 // Fish: how many forms it has
    };

    void build_entries();
    void sort_entries();
    void select(int index);

    // Fish: shows another form of the selected one, and the buttons that switch between them
    void show_form(int form);
    void build_form_buttons();
    void refresh_buttons();

    sf::FloatRect slot_rect(int index) const;   // Screen rect of a grid slot (scroll applied)
    int visible_rows() const;
    int row_count() const;

    void render_grid(sf::RenderWindow& window);
    // A folder tab of the grid panel: the open one is part of the panel (yellow label), the other darker steel
    void render_tab(sf::RenderWindow& window, const UIButtonElement& tab, const std::string& label, bool isOpen);
    void render_details(sf::RenderWindow& window);
    void render_portrait(sf::RenderWindow& window, const Entry& entry, const sf::FloatRect& area);

    float health_of(const Entry& entry) const;
    float attack_of(const Entry& entry) const;

    std::shared_ptr<DataLoader> m_dataLoader;
    Book m_book = Book::FISH;
    SortKey m_sortKey = SortKey::NUMBER;
    bool m_isDescending = false;

    std::vector<Entry> m_entries;
    int m_selected = -1;
    int m_scrollRow = 0;

    UIButtonElement m_fishTab;
    UIButtonElement m_enemiesTab;
    std::vector<std::unique_ptr<UIButtonElement>> m_sortButtons;   // One per SortKey
    std::vector<std::unique_ptr<UIButtonElement>> m_formButtons;   // FORM 1, FORM 2... of the selected fish
    int m_pendingForm = -1;                                        // Clicked, shown at the next update
    sf::Text m_sortLabel;

    UIPanel m_gridPanel;
    UIPanel m_detailPanel;
    sf::Text m_text;            // Reused for the details
    UIAbilityList m_abilities;
};

#pragma once

#include "UIButtonElement.h"
#include "UIPanel.h"
#include "UIAbilityList.h"

#include <map>
#include <memory>
#include <vector>

class DataLoader;
struct EntityData;

/*
 * UIEncyclopedia
 * --------------
 * The fish and enemy encyclopedia, like The Battle Cats' Cat Guide and Enemy Guide:
 * two tabs (FISH / ENEMIES), a sort pill (its list: number, name, health, attack, range, speed) with a button
 * reversing the order, a grid of portraits (a finger drags it up and down, the mouse wheel scrolls it), and a panel
 * describing the selected one: name, number, rarity or types, description, stats and abilities (UIAbilityList).
 * Numbers are the order they're met in the game: the first fish, then the fish the stages give, chapter by chapter
 * (then the clam's rarer ones); the enemies in the order the stages send them.
 * Fish the player doesn't own and enemies not met in battle yet are black, with their details hidden; the forms a
 * fish hasn't reached are "???" too. Fish stats are the ones of their current level.
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
        int number = 0;                    // Its No.: the order it's met in the game
        std::shared_ptr<EntityData> data;  // Current form for fish
        bool isKnown = false;              // Owned fish, enemies met
        int level = 0;                     // Fish only
        int extraLevels = 0;               // Fish only: the Clam Room's "+3" (it counts for the stats)
        std::string iconPath;              // Fish only
        int form = 0;                      // Fish: the form shown (starts on the current one)
        int currentForm = 0;               // Fish: the form reached
        int formCount = 1;                 // Fish: how many forms it has
    };

    void build_entries();
    std::map<int, int> encounter_numbers() const;   // UID -> No. of the book open
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
    sf::FloatRect detail_scroll_rect() const;   // Under the portrait: the description, stats and abilities (they scroll)
    float m_detailScroll = 0.f;                 // How far they're scrolled up
    float m_detailContentHeight = 0.f;          // How tall they are (set when drawn)
    bool m_isDetailDragging = false;
    float m_detailPressY = 0.f, m_detailPressScroll = 0.f;
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
    UIButtonElement m_sortPill;          // "Sort: Name": opens the list of orders under it
    UIButtonElement m_sortDirection;     // Up or down: reverses the order
    bool m_isSortOpen = false;           // The list is open (it takes the input until closed)
    sf::FloatRect sort_option_rect(int key) const;
    void choose_sort(SortKey key);
    void render_sort_menu(sf::RenderWindow& window);
    std::vector<std::unique_ptr<UIButtonElement>> m_formButtons;   // FORM 1, FORM 2... of the selected fish (??? not reached)
    int m_pendingForm = -1;                                        // Clicked, shown at the next update

    // A finger dragging the grid up and down scrolls it; a tap (no drag) selects
    bool m_isGridPressed = false;
    bool m_isGridDragging = false;
    sf::Vector2f m_gridPress;
    int m_gridPressRow = 0;

    UIPanel m_gridPanel;
    UIPanel m_detailPanel;
    sf::Text m_text;            // Reused for the details
    UIAbilityList m_abilities;

    // A fish looked at for the first time gives its Pearls: "+10 Pearls!" rises over its portrait
    void discover_selected();
    float m_rewardTime = 0.f;   // Counts down while it shows
    int m_rewardAmount = 0;
};

#pragma once

#include "UIUpgradeIcon.h"
#include "UIButtonElement.h"
#include "UIOfferButton.h"
#include "UISwipe.h"
#include "UIRoundButton.h"
#include "UIPanel.h"
#include "UIAbilityList.h"

class DataLoader;

/*
 * UIUpgradeSlider
 * ----------------
 * The Upgrade screen's carousel, like The Battle Cats' upgrade screen: unit cards in a row
 * (the selected one big in the middle), arrows on both sides, a big UPGRADE! button, and a panel
 * with the selected unit's name, description and abilities (UIAbilityList: hover one to see what it does).
 * Category cards at its left end, like The Battle Cats': BASE (the base upgrades: Base Health, Wallet Up), and RARE
 * and SUPER RARE once the player has a fish that rare. Tapping one (or OPEN) shows its cards, a NORMAL card at their
 * left end goes back to the Normal fish.
 * Left / Right keys, the arrows or a click on a side card move it; a finger sliding sideways moves the cards with
 * it (the one passing the middle grows), and they settle on the nearest when let go.
 * Works with DataLoader to fetch player data and unit info.
 */

class UIUpgradeSlider : public UIElement
{
public:

    // Constructor
    // dataLoader: shared pointer to DataLoader for player/unit data
    UIUpgradeSlider(std::shared_ptr<DataLoader> dataLoader);

    // Initialize icons based on available upgrades and player data
    void init_icons();

    // Moves slider to display unit at new index
    // force: glide back to it even if it's the current one (after a drag)
    void move_to_index(int newIndex, bool force = false);
    void follow_drag(float dragX);       // The cards follow a finger sliding them
    // Where they settle when let go slowly (allowsFlick: a short flick from rest still moves one card)
    int snap_index(float dragX, bool allowsFlick = true) const;
    void release_slide(float dragX, float speed);   // Let go: glides on when flung, else settles on the nearest
    float drag_offset() const;           // Where the cards are now, as a drag from the selected one (0 at rest)

    // The selected fish (-1: none, or a base upgrade or category card), and selecting one at once (coming from the
    // Equip screen's shortcut; a Rare fish opens the RARE cards)
    int get_selected_uid() const;
    void select_unit(int uid);

    // Calculate the upgrade cost of a unit given its UID and current level
    int calculate_upgrade_cost(int uid, int level);
    static int calculate_upgrade_cost(const DataLoader& dataLoader, int uid, int level);

    // UI update/render/event methods
    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

private:

    // Upgrades (or unlocks) the selected unit if the player has enough shells
    void upgrade_selected();

    // What a card is: a fish, a base upgrade, or a category (the cards of the fish of a rarity, or the base upgrades)
    enum class Category { NORMAL, BASE, RARE, SUPER_RARE };
    struct Entry
    {
        enum class Kind { FISH, BASE, CATEGORY } kind = Kind::FISH;
        int id = -1;   // FISH: the unit's UID; BASE: a PlayerData::BaseUpgrade; CATEGORY: a Category
    };
    Category m_category = Category::NORMAL;   // The cards shown: the Normal fish (and the category cards), or a category's
    void build_entries(const PlayerData& player);
    void layout_cards(float slideIn = 0.f);   // The cards of m_entries round m_currentIndex (slideIn: they slide in from that far)
    std::shared_ptr<UIUpgradeIcon> make_card(const Entry& entry, const PlayerData& player) const;
    void open_category(Category category);
    Category category_of(int uid) const;      // The cards a fish is among (its rarity)
    int index_of(Entry::Kind kind, int id) const;

    int m_currentIndex = 0; // Current selected upgrade index
    std::vector<Entry> m_entries;   // What each card is
    std::vector<std::shared_ptr<UIUpgradeIcon>> m_upgradeIcons; // Icon objects for the slider

    UIOfferButton m_upgradeButton;    // UPGRADE! (UNLOCK!) with its price on it, in shells
    UISwipe m_swipe;                  // Swiping the cards sideways
    bool m_isFollowing = false;       // The cards follow the finger
    UIGlide m_glide;                  // Flung: the cards glide on to the selected one (counted from it)
    float m_dragBase = 0.f;           // Where the cards were when pressed (0 at rest; mid-glide when caught)
    bool m_caughtGlide = false;       // The press stopped a glide: its release settles, it isn't a tap
    UIRoundButton m_previousButton;   // Arrows on both sides of the cards
    UIRoundButton m_nextButton;

    std::shared_ptr<DataLoader> m_dataLoader; // Reference to data loader

    // Lower panel: name and description of the selected unit
    UIPanel m_descriptionPanel;
    sf::Text m_unitName;
    sf::Text m_description;
    UIAbilityList m_abilities;  // Under the description: "Strong against Jelly"... (hover: what it does)
    // The description and the abilities are clipped to the panel's left column; too long, they scroll (drag, wheel)
    sf::FloatRect description_area() const;
    float m_abilitiesTop = 0.f;          // Where the abilities start, not scrolled
    float m_descriptionScroll = 0.f;
    bool m_isDescriptionDragging = false;
    float m_descriptionPressY = 0.f, m_descriptionPressScroll = 0.f;
    float description_hidden() const;    // How much doesn't fit
    int m_describedKey = -1;   // The description is wrapped again only when the selection changes (kind and id)
    int m_describedLevel = -1;

    // What the upgrade gives: { name, now, next } rows, the evolution line; what it costs.
    // isExtra: the Clam Room's extra levels ("+3", in UISkin::EXTRA), one value: the upgrade doesn't change them
    struct StatRow { std::string name, now, next; bool isBetter = false; bool isExtra = false; };
    std::vector<StatRow> m_statRows;
    std::string m_evolutionLine;
    std::string m_infoTitle;    // Over the rows: NEXT LEVEL, MAX LEVEL, INSIDE
    int m_cost = 0;
    bool m_isMaxLevel = false;  // Also: one value per row (a category's contents)
    void describe_upgrade(int uid, int level);
    void describe_base(PlayerData::BaseUpgrade upgrade, int level);
    void describe_category(Category category);
    void render_upgrade_info(sf::RenderWindow& window);

    // Card layout: bottom-aligned on CARDS_BOTTOM, the selected card centered
    const sf::Vector2f baseIconSize = { 300.0f, 300.0f };
    const float CARDS_BOTTOM = 640.f;
    const float ICON_SPACING = 440.0f;
    const float CENTER_SCALE = 1.3f;
    const float SIDE_SCALE = 1.0f;
    const float CENTER_OFFSET_X = (CENTER_SCALE - SIDE_SCALE) * 300.f / 2.f; // Keeps the bigger card centered
    const float TWEEN_DURATION = 0.3f; // Animation duration in seconds
};

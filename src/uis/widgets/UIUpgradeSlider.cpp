#include "UIUpgradeSlider.h"
#include "UIToast.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"
#include "../../core/stages/Stage.h"

#include <algorithm>
#include <cmath>

namespace
{
    const sf::FloatRect UPGRADE_BUTTON{ { 60.f, 700.f }, { 470.f, 140.f } };
    const sf::FloatRect DESCRIPTION_PANEL{ { 570.f, 668.f }, { 1290.f, UISkin::CONTENT_BOTTOM - 16.f - 668.f } };   // Down to the bottom bar: room for the abilities
    const float SHELF_TOP = 116.f;     // The cards' shelf, under the title bar
    const float DOTS_Y = 146.f;        // The page dots, at its top
    const float TEXT_PADDING = 50.f;
    const float ARROW_SIZE = 96.f;
    const float ARROW_Y = 412.f;
    const unsigned int NAME_SIZE = 44;
    const unsigned int DESCRIPTION_SIZE = 28;
    const float STATS_X = DESCRIPTION_PANEL.position.x + 790.f;     // The right column of the panel: the upgrade
    const float DESCRIPTION_WIDTH = 690.f;                            // The left column: description and abilities
    const unsigned int STAT_SIZE = 30;
    const sf::Color BETTER{ 110, 220, 120 };                          // Green: what the upgrade adds
    const sf::Color CORAL{ 255, 128, 104 };

    std::string format_stat(float value)
    {
        const int rounded = static_cast<int>(std::lround(value));
        return std::to_string(rounded);
    }

    // The base upgrades' and the categories' cards
    const char* BASE_TEXTURE = "assets/images/textures/bases/fishBaseTEST.png";   // The fish bowl (Stage::spawn_bases)
    const char* WALLET_TEXTURE = "assets/images/textures/icons/uis/cash_up.png";  // The battle's WALLET button
    const sf::Color CATEGORY_SLOT{ 255, 214, 120 };                              // A category card's slot: gold
    const int FISH_UID = 1;                                                       // The Normal fish' card shows the Fish

    const char* base_upgrade_name(PlayerData::BaseUpgrade upgrade)
    {
        return upgrade == PlayerData::BaseUpgrade::HEALTH ? "Base Health" : "Wallet Up";
    }

    std::string percent(float factor)
    {
        return "+" + std::to_string(static_cast<int>(std::lround((factor - 1.f) * 100.f))) + "%";
    }
}

/*
 * Initializes the upgrade slider: upgrade button, arrows, description panel and the cards.
 */
UIUpgradeSlider::UIUpgradeSlider(std::shared_ptr<DataLoader> dataLoader) :
	m_dataLoader(dataLoader),
	m_upgradeButton(UPGRADE_BUTTON.size, UPGRADE_BUTTON.position, "UPGRADE!", UIOfferButton::Kind::CURRENCY, "icon_shell", 0),
	m_previousButton(ARROW_SIZE, { 36.f, ARROW_Y }, "icon_play"),
	m_nextButton(ARROW_SIZE, { 1920.f - 36.f - ARROW_SIZE, ARROW_Y }, "icon_play"),
	m_descriptionPanel(DESCRIPTION_PANEL.size, DESCRIPTION_PANEL.position, UISkin::PANEL_DARK, 1.2f),
	m_unitName(UISkin::heavy_font()),
	m_description(UISkin::regular_font())
{
	m_upgradeButton.set_text_size(54);
	m_upgradeButton.set_attention(true);   // Breathes while there are enough shells (greyed: still)
	m_upgradeButton.set_click_sound("");   // upgrade_selected plays "purchase" or "unlock"
	m_upgradeButton.set_callback([this]() { upgrade_selected(); });

	m_previousButton.set_icon_flipped(true);
	m_previousButton.set_click_sound("");   // move_to_index plays the selection sound
	m_nextButton.set_click_sound("");
	m_previousButton.set_callback([this]() { move_to_index(m_currentIndex - 1); });
	m_nextButton.set_callback([this]() { move_to_index(m_currentIndex + 1); });

	UISkin::style_text(m_unitName, NAME_SIZE, UISkin::NUMBER);
	UISkin::style_body_text(m_description, DESCRIPTION_SIZE);
	m_description.setLineSpacing(1.1f);

	init_icons();
}

/*
 * Upgrades the selected unit (or unlocks it at level 0) if the player has enough shells, then saves.
 */
void UIUpgradeSlider::upgrade_selected()
{
	if (m_upgradeIcons.empty()) return;
	auto player = m_dataLoader->get_player_data().lock();
	if (!player) return;
	const Entry entry = m_entries[m_currentIndex];

	// A category card: OPEN
	if (entry.kind == Entry::Kind::CATEGORY)
	{
		open_category(static_cast<Category>(entry.id));
		return;
	}

	// A base upgrade: a level for the shells
	if (entry.kind == Entry::Kind::BASE)
	{
		const auto upgrade = static_cast<PlayerData::BaseUpgrade>(entry.id);
		const int level = player->get_base_upgrade_level(upgrade);
		if (level >= PlayerData::BASE_UPGRADE_MAX_LEVEL || !player->spend_shells(PlayerData::base_upgrade_cost(level))) return;
		player->upgrade_base(upgrade);
		play_sound("purchase");
		auto icon = m_upgradeIcons[m_currentIndex];
		icon->set_unit_level(player->get_base_upgrade_level(upgrade));
		icon->set_upgrade_cost(PlayerData::base_upgrade_cost(player->get_base_upgrade_level(upgrade)));
		m_describedKey = -1;
		m_dataLoader->save_progress();
		return;
	}

	int unitUid = entry.id;                                // Currently selected unit
	int level = player->get_unit_level(unitUid);           // Current level
	int upgradeCost = calculate_upgrade_cost(unitUid, level); // Calculate cost

	if (level >= UIUpgradeIcon::MAX_LEVEL) return;

	if (player->spend_shells(upgradeCost))
	{
		// Shells clinking for a level, a fanfare for a new fish
		if (player->is_unit_owned(unitUid))
		{
			player->upgrade_unit(unitUid, m_dataLoader->get_unit_form_count(unitUid)); // Upgrade if owned
			play_sound("purchase");
		}
		else
		{
			player->unlock_unit(unitUid); // Unlock if not owned
			play_sound("unlock");
		}
		UIToast::announce_school_rank_rewards(*player);   // A School Rank milestone: Shiny Scales

		// The form (and so the name and icon) can change with the level
		auto icon = m_upgradeIcons[m_currentIndex];
		auto unitLevel = player->get_unit_level(unitUid);
		auto unitForm = player->get_unit_form(unitUid);
		auto unitData = m_dataLoader->get_unit_data(unitUid, unitForm);

		icon->set_unit_level(unitLevel);
		icon->set_upgrade_cost(calculate_upgrade_cost(unitUid, unitLevel));
		icon->set_unit_name(unitData->name);
		icon->set_texture(m_dataLoader->get_unit_icon_texture_path(unitUid, unitForm));
		m_describedKey = -1; // Name and description again

		m_dataLoader->save_progress();
	}
}
/*
 * Initializes the array of UIUpgradeIcon objects based on player data.
 * Includes owned units and units waiting to be unlocked.
 */
void UIUpgradeSlider::init_icons()
{
	m_upgradeIcons.clear();
	m_entries.clear();
	auto playerData = m_dataLoader->get_player_data().lock();
	if (!playerData) return;
	build_entries(*playerData);

	// At first on the first fish, not on the category cards before them
	if (m_currentIndex <= 0 && m_category == Category::NORMAL)
		for (int i = 0; i < static_cast<int>(m_entries.size()); ++i)
			if (m_entries[i].kind != Entry::Kind::CATEGORY) { m_currentIndex = i; break; }
	layout_cards();
}

/*
 * The cards of the category shown. The Normal fish: the category cards first (BASE; RARE and SUPER RARE once the
 * player has a fish that rare), then the fish, owned ones first. A category: NORMAL (back) first, then its cards
 */
void UIUpgradeSlider::build_entries(const PlayerData& player)
{
	m_entries.clear();
	std::vector<int> fish;
	for (auto& [uid, _] : player.ownedUnits) fish.push_back(uid);
	for (int uid : player.unitsWaitingToBeUnlocked)
		if (!player.is_unit_owned(uid)) fish.push_back(uid);
	std::sort(fish.begin(), fish.end(), [&](int a, int b) {
		const bool aOwned = player.is_unit_owned(a), bOwned = player.is_unit_owned(b);
		if (aOwned != bOwned) return aOwned > bOwned;   // unlocked first
		return a < b;                                   // then by UID
		});

	const auto add_fish = [&](Category category) {
		for (int uid : fish)
			if (category_of(uid) == category) m_entries.push_back({ Entry::Kind::FISH, uid });
		};
	const auto has_fish = [&](Category category) {
		return std::any_of(fish.begin(), fish.end(), [&](int uid) { return category_of(uid) == category; });
		};

	switch (m_category)
	{
	case Category::NORMAL:
		m_entries.push_back({ Entry::Kind::CATEGORY, static_cast<int>(Category::BASE) });
		for (Category rarity : { Category::RARE, Category::SUPER_RARE })
			if (has_fish(rarity)) m_entries.push_back({ Entry::Kind::CATEGORY, static_cast<int>(rarity) });
		add_fish(Category::NORMAL);
		break;
	case Category::BASE:
		m_entries.push_back({ Entry::Kind::CATEGORY, static_cast<int>(Category::NORMAL) });
		for (int i = 0; i < static_cast<int>(PlayerData::BaseUpgrade::COUNT); ++i)
			m_entries.push_back({ Entry::Kind::BASE, i });
		break;
	default:
		m_entries.push_back({ Entry::Kind::CATEGORY, static_cast<int>(Category::NORMAL) });
		add_fish(m_category);
		break;
	}
}

UIUpgradeSlider::Category UIUpgradeSlider::category_of(int uid) const
{
	auto data = m_dataLoader->get_unit_data(uid, 0);
	const Rarity rarity = data ? data->rarity : Rarity::NORMAL;
	return rarity == Rarity::SUPER_RARE ? Category::SUPER_RARE : rarity == Rarity::RARE ? Category::RARE : Category::NORMAL;
}

int UIUpgradeSlider::index_of(Entry::Kind kind, int id) const
{
	for (int i = 0; i < static_cast<int>(m_entries.size()); ++i)
		if (m_entries[i].kind == kind && m_entries[i].id == id) return i;
	return -1;
}

/*
 * A card for an entry: a fish (its level, the next level's price), a base upgrade (the bowl or the wallet, fitted
 * whole in the slot), or a category (a gold slot, how many cards it holds)
 */
std::shared_ptr<UIUpgradeIcon> UIUpgradeSlider::make_card(const Entry& entry, const PlayerData& player) const
{
	std::shared_ptr<UIUpgradeIcon> icon;
	if (entry.kind == Entry::Kind::FISH)
	{
		const int unitForm = player.get_unit_form(entry.id);
		auto unitData = m_dataLoader->get_unit_data(entry.id, unitForm);
		const int unitLevel = player.get_unit_level(entry.id);
		icon = std::make_shared<UIUpgradeIcon>(m_dataLoader->get_unit_icon_texture_path(entry.id, unitForm));
		icon->set_unit_name(unitData ? unitData->name : "?");
		icon->set_unit_level(unitLevel);
		icon->set_extra_levels(player.get_unit_extra_levels(entry.id));
		icon->set_upgrade_cost(calculate_upgrade_cost(*m_dataLoader, entry.id, unitLevel));
		return icon;
	}
	if (entry.kind == Entry::Kind::BASE)
	{
		const auto upgrade = static_cast<PlayerData::BaseUpgrade>(entry.id);
		const int level = player.get_base_upgrade_level(upgrade);
		icon = std::make_shared<UIUpgradeIcon>(upgrade == PlayerData::BaseUpgrade::HEALTH ? BASE_TEXTURE : WALLET_TEXTURE);
		icon->set_icon_fitted(true);
		icon->set_unit_name(base_upgrade_name(upgrade));
		icon->set_max_level(PlayerData::BASE_UPGRADE_MAX_LEVEL);
		icon->set_unit_level(level);
		icon->set_upgrade_cost(PlayerData::base_upgrade_cost(level));
		return icon;
	}

	// A category: its picture (the bowl, or its first fish), its name, what it holds
	const auto category = static_cast<Category>(entry.id);
	std::string picture = BASE_TEXTURE, name = "BASE", caption;
	if (category == Category::BASE)
		caption = std::to_string(static_cast<int>(PlayerData::BaseUpgrade::COUNT)) + " UPGRADES";
	else
	{
		int count = 0, first = -1;
		for (auto& [uid, _] : player.ownedUnits)
			if (category_of(uid) == category) { ++count; if (first < 0 || uid < first) first = uid; }
		if (category == Category::NORMAL && player.is_unit_owned(FISH_UID)) first = FISH_UID;
		if (first >= 0) picture = m_dataLoader->get_unit_icon_texture_path(first, player.get_unit_form(first));
		name = category == Category::NORMAL ? "NORMAL" : category == Category::RARE ? "RARE" : "SUPER RARE";
		caption = std::to_string(count) + " FISH";
	}
	icon = std::make_shared<UIUpgradeIcon>(picture);
	icon->set_icon_fitted(category == Category::BASE);
	icon->set_slot_tint(CATEGORY_SLOT);
	icon->set_unit_name(name);
	icon->set_caption(caption);
	return icon;
}

/*
 * The cards round the selected one (it big in the middle); slideIn: they start that far to the side and glide in
 * (a category opening or closing)
 */
void UIUpgradeSlider::layout_cards(float slideIn)
{
	m_upgradeIcons.clear();
	auto playerData = m_dataLoader->get_player_data().lock();
	if (!playerData) return;
	m_currentIndex = std::clamp(m_currentIndex, 0, std::max(0, static_cast<int>(m_entries.size()) - 1));
	const float centerX = 1920.f / 2.0f - baseIconSize.x / 2.f;

	for (int i = 0; i < static_cast<int>(m_entries.size()); ++i)
	{
		auto icon = make_card(m_entries[i], *playerData);
		icon->select(i == m_currentIndex);

		float posX = centerX + (i - m_currentIndex) * ICON_SPACING + slideIn;
		if (i == m_currentIndex)
			posX -= CENTER_OFFSET_X;

		icon->set_position({ posX, CARDS_BOTTOM });
		float scale = (i == m_currentIndex) ? CENTER_SCALE : SIDE_SCALE;
		icon->set_size(baseIconSize * scale);

		icon->tweenX = tweeny::from(icon->get_position().x).to(icon->get_position().x).during(1.0f);
		icon->tweenWidth = tweeny::from(icon->get_size().x).to(icon->get_size().x).during(1.0f);
		icon->tweenHeight = tweeny::from(icon->get_size().y).to(icon->get_size().y).during(1.0f);

		m_upgradeIcons.push_back(icon);
	}
	m_describedKey = -1;
	if (slideIn != 0.f) move_to_index(m_currentIndex, true);   // Glide into place
}

/*
 * Shows a category's cards (NORMAL: back to the Normal fish). Inside one, its first card is selected; back, the
 * category card it came from. They slide in from the side they're on
 */
void UIUpgradeSlider::open_category(Category category)
{
	auto player = m_dataLoader->get_player_data().lock();
	if (!player || category == m_category) return;
	const Category from = m_category;
	m_category = category;
	m_glide.isActive = false;
	build_entries(*player);
	if (category == Category::NORMAL)
		m_currentIndex = std::max(0, index_of(Entry::Kind::CATEGORY, static_cast<int>(from)));
	else
		m_currentIndex = std::min(1, static_cast<int>(m_entries.size()) - 1);
	play_sound("ui_transition");
	layout_cards(category == Category::NORMAL ? -ICON_SPACING : ICON_SPACING);
}
/*
 * Computes the cost to upgrade a unit based on its level and evolutions.
 */
int UIUpgradeSlider::calculate_upgrade_cost(int uid, int level)
{
	return calculate_upgrade_cost(*m_dataLoader, uid, level);
}

int UIUpgradeSlider::calculate_upgrade_cost(const DataLoader& dataLoader, int uid, int level)
{
	auto data = dataLoader.get_unit_data(uid, 0);
	return data ? PlayerData::upgrade_cost(data->baseUpgradeCost, level) : 0;   // The formula is PlayerData's
}

/*
 * Updates the cards (tweens, affordability, lineup), the button and the description of the selected unit.
 */
void UIUpgradeSlider::update(float deltaTime)
{
	auto player = m_dataLoader->get_player_data().lock();

	// Flung: the cards glide on and slow down, then land (move_to_index puts everything in place)
	if (m_glide.isActive)
	{
		const bool isGliding = m_glide.step(deltaTime);
		follow_drag(m_glide.position);
		if (!isGliding) move_to_index(m_currentIndex, true);
	}

	for (int i = 0; i < static_cast<int>(m_upgradeIcons.size()); ++i)
	{
		auto& icon = m_upgradeIcons[i];
		const Entry& entry = m_entries[i];
		if (player && entry.kind == Entry::Kind::FISH)
		{
			const int level = player->get_unit_level(entry.id);
			icon->set_affordable(player->shells >= calculate_upgrade_cost(entry.id, level));
			icon->set_in_lineup(std::find(player->equippedUnits.begin(), player->equippedUnits.end(), entry.id) != player->equippedUnits.end());
		}
		else if (player && entry.kind == Entry::Kind::BASE)
			icon->set_affordable(player->shells >= PlayerData::base_upgrade_cost(player->get_base_upgrade_level(static_cast<PlayerData::BaseUpgrade>(entry.id))));
		icon->update(deltaTime); // Tweens and info lines
	}

	m_previousButton.set_visible(m_currentIndex > 0);
	m_nextButton.set_visible(m_currentIndex + 1 < static_cast<int>(m_upgradeIcons.size()));
	m_previousButton.update(deltaTime);
	m_nextButton.update(deltaTime);

	if (m_upgradeIcons.empty() || !player)
	{
		m_upgradeButton.set_enabled(false);
		m_upgradeButton.update(deltaTime);
		return;
	}

	// The button: OPEN for a category, UNLOCK! for a new unit, greyed out when maxed or too expensive
	const Entry entry = m_entries[m_currentIndex];
	int level = 0, cost = 0, maxLevel = UIUpgradeIcon::MAX_LEVEL;
	if (entry.kind == Entry::Kind::FISH)
	{
		level = player->get_unit_level(entry.id);
		cost = calculate_upgrade_cost(entry.id, level);
	}
	else if (entry.kind == Entry::Kind::BASE)
	{
		level = player->get_base_upgrade_level(static_cast<PlayerData::BaseUpgrade>(entry.id));
		cost = PlayerData::base_upgrade_cost(level);
		maxLevel = PlayerData::BASE_UPGRADE_MAX_LEVEL;
	}
	if (entry.kind == Entry::Kind::CATEGORY)
	{
		m_upgradeButton.set_text("OPEN");
		m_upgradeButton.set_enabled(true);
		m_upgradeButton.set_cost(-1);
	}
	else
	{
		m_upgradeButton.set_text(level <= 0 ? "UNLOCK!" : "UPGRADE!");
		m_upgradeButton.set_enabled(level < maxLevel && player->shells >= cost);
		m_upgradeButton.set_cost(level < maxLevel ? cost : -1);   // The price on the button
		if (level >= maxLevel) m_upgradeButton.set_text("MAX LEVEL");
	}
	m_upgradeButton.update(deltaTime);

	// Name and description of the selected card (wrapped to the panel)
	const int key = static_cast<int>(entry.kind) * 100000 + entry.id;
	if (key != m_describedKey)
	{
		std::string name, description;
		std::vector<AbilityData> abilities;
		if (entry.kind == Entry::Kind::FISH)
		{
			auto unitData = m_dataLoader->get_unit_data(entry.id, player->get_unit_form(entry.id));
			name = unitData->name;
			description = unitData->description;
			abilities = unitData->abilities;
		}
		else if (entry.kind == Entry::Kind::BASE)
		{
			const bool isHealth = static_cast<PlayerData::BaseUpgrade>(entry.id) == PlayerData::BaseUpgrade::HEALTH;
			name = base_upgrade_name(static_cast<PlayerData::BaseUpgrade>(entry.id));
			description = isHealth
				? "Your fish bowl gets 15% more health in every stage, for each level."
				: "Your wallet starts every stage 10% bigger and fills up 10% faster, for each level. The WALLET upgrades of the battle add to it.";
		}
		else
		{
			switch (static_cast<Category>(entry.id))
			{
			case Category::BASE: name = "Base"; description = "Upgrades for your fish bowl and your wallet. They work in every stage."; break;
			case Category::RARE: name = "Rare fish"; description = "Rare fish from the giant clam (Clam Room)."; break;
			case Category::SUPER_RARE: name = "Super Rare fish"; description = "The rarest fish of the giant clam (Clam Room)."; break;
			default: name = "Normal fish"; description = "The fish you meet in the stages. Back to them."; break;
			}
		}
		m_unitName.setString(name);
		m_description.setString(UISkin::wrap_text(description, DESCRIPTION_SIZE, DESCRIPTION_WIDTH, UISkin::regular_font()));
		m_abilities.set_abilities(abilities);
		m_describedKey = key;
		m_describedLevel = -1;   // The numbers too

		// Name centered, description and abilities left-aligned under it
		UISkin::set_text_anchor(m_unitName, { 0.5f, 0.5f });
		m_unitName.setPosition({ DESCRIPTION_PANEL.position.x + TEXT_PADDING + DESCRIPTION_WIDTH / 2.f, DESCRIPTION_PANEL.position.y + 44.f });
		UISkin::set_text_anchor(m_description, { 0.f, 0.f });
		m_description.setPosition({ DESCRIPTION_PANEL.position.x + TEXT_PADDING, DESCRIPTION_PANEL.position.y + 80.f });

		const float descriptionBottom = m_description.getGlobalBounds().position.y + m_description.getGlobalBounds().size.y;
		m_abilitiesTop = descriptionBottom + 14.f;
		m_descriptionScroll = 0.f;
	}
	if (level != m_describedLevel || m_statRows.empty())
	{
		if (entry.kind == Entry::Kind::FISH) describe_upgrade(entry.id, level);
		else if (entry.kind == Entry::Kind::BASE) describe_base(static_cast<PlayerData::BaseUpgrade>(entry.id), level);
		else describe_category(static_cast<Category>(entry.id));
	}
}

/*
 * A base upgrade's numbers: its level, what it gives now and at the next level, the price
 */
void UIUpgradeSlider::describe_base(PlayerData::BaseUpgrade upgrade, int level)
{
	m_describedLevel = level;
	m_statRows.clear();
	m_evolutionLine.clear();
	m_isMaxLevel = level >= PlayerData::BASE_UPGRADE_MAX_LEVEL;
	m_infoTitle = m_isMaxLevel ? "MAX LEVEL" : "NEXT LEVEL";
	m_cost = PlayerData::base_upgrade_cost(level);
	const int next = m_isMaxLevel ? level : level + 1;
	m_statRows.push_back({ "Level", std::to_string(level), std::to_string(next), next > level });
	if (upgrade == PlayerData::BaseUpgrade::HEALTH)
		m_statRows.push_back({ "Health", percent(PlayerData::base_health_factor(level)), percent(PlayerData::base_health_factor(next)), !m_isMaxLevel });
	else
	{
		const float now = PlayerData::wallet_factor(level), then = PlayerData::wallet_factor(next);
		m_statRows.push_back({ "Wallet", format_stat(Stage::BASE_MAX_CASH * now), format_stat(Stage::BASE_MAX_CASH * then), !m_isMaxLevel });
		m_statRows.push_back({ "Per second", format_stat(Stage::BASE_CASH_PER_SECOND * now), format_stat(Stage::BASE_CASH_PER_SECOND * then), !m_isMaxLevel });
	}
}

/*
 * A category card: what it holds (one value per row)
 */
void UIUpgradeSlider::describe_category(Category category)
{
	m_describedLevel = 0;
	m_statRows.clear();
	m_evolutionLine.clear();
	m_isMaxLevel = true;   // One value per row
	m_infoTitle = "INSIDE";
	m_cost = 0;
	auto player = m_dataLoader->get_player_data().lock();
	if (!player) return;
	if (category == Category::BASE)
	{
		for (int i = 0; i < static_cast<int>(PlayerData::BaseUpgrade::COUNT); ++i)
		{
			const auto upgrade = static_cast<PlayerData::BaseUpgrade>(i);
			m_statRows.push_back({ base_upgrade_name(upgrade), "Lv " + std::to_string(player->get_base_upgrade_level(upgrade)), "" });
		}
		return;
	}
	int owned = 0;
	for (auto& [uid, _] : player->ownedUnits)
		if (category_of(uid) == category) ++owned;
	m_statRows.push_back({ "Fish", std::to_string(owned), "" });
}
/*
 * The upgrade's numbers: level, health and attack now and after it (the level multiplies both), the evolution
 * coming, the price
 */
void UIUpgradeSlider::describe_upgrade(int uid, int level)
{
	m_describedLevel = level;
	m_statRows.clear();
	m_evolutionLine.clear();
	m_isMaxLevel = level >= UIUpgradeIcon::MAX_LEVEL;
	m_infoTitle = m_isMaxLevel ? "MAX LEVEL" : "NEXT LEVEL";
	m_cost = calculate_upgrade_cost(uid, level);

	auto player = m_dataLoader->get_player_data().lock();
	auto data = m_dataLoader->get_unit_data(uid, player ? player->get_unit_form(uid) : 0);
	if (!data) return;

	// The Clam Room's extra levels add to the stats, not to the price (their own row, under the level)
	const int extra = player ? player->get_unit_extra_levels(uid) : 0;
	const int now = std::max(1, level), next = m_isMaxLevel ? level : level <= 0 ? 1 : level + 1;
	const float nowFactor = level <= 0 ? 0.f : PlayerData::stat_multiplier(now + extra), nextFactor = PlayerData::stat_multiplier(next + extra);
	m_statRows.push_back({ "Level", level <= 0 ? "-" : std::to_string(level), std::to_string(next), next > level });
	if (extra > 0) m_statRows.push_back({ "Extra levels", "", "+" + std::to_string(extra), false, true });
	m_statRows.push_back({ "Health", level <= 0 ? "-" : format_stat(data->health * nowFactor), format_stat(data->health * nextFactor), !m_isMaxLevel });
	m_statRows.push_back({ "Attack", level <= 0 ? "-" : format_stat(data->attackPower * nowFactor), format_stat(data->attackPower * nextFactor), !m_isMaxLevel });

	// Evolutions at levels 10 and 30 (when the fish has that form)
	for (const auto& [atLevel, form] : { std::pair{ 10, 1 }, std::pair{ 30, 2 } })
	{
		if (level >= atLevel) continue;
		// A form that doesn't exist comes back as the last one: no evolution then
		auto evolved = m_dataLoader->get_unit_data(uid, form);
		if (!evolved || evolved == data || evolved == m_dataLoader->get_unit_data(uid, form - 1)) break;
		m_evolutionLine = next == atLevel ? "Evolves into " + evolved->name + "!" : "Evolves at level " + std::to_string(atLevel);
		break;
	}
}

/*
 * The right column of the panel (now -> next) and the price under the button
 */
void UIUpgradeSlider::render_upgrade_info(sf::RenderWindow& window)
{
	auto drawText = [&window](const std::string& string, unsigned int size, sf::Color color, sf::Vector2f position, sf::Vector2f anchor)
		{
			sf::Text text(UISkin::heavy_font(), string);
			UISkin::style_text(text, size, color);
			UISkin::set_text_anchor(text, anchor);
			text.setPosition(position);
			window.draw(text);
			return text.getGlobalBounds();
		};

	float y = DESCRIPTION_PANEL.position.y + 42.f;
	drawText(m_infoTitle, 34, UISkin::TITLE, { STATS_X + 190.f, y }, { 0.5f, 0.5f });
	y += 50.f;
	for (const StatRow& row : m_statRows)
	{
		if (row.isExtra)
		{
			// The extra levels: purple, one value at the end of the row (no arrow: upgrading doesn't change them)
			drawText(row.name, STAT_SIZE, UISkin::EXTRA, { STATS_X, y }, { 0.f, 0.5f });
			drawText(row.next, STAT_SIZE, UISkin::EXTRA, { STATS_X + (m_isMaxLevel ? 250.f : 385.f), y }, { 1.f, 0.5f });
			y += 43.f;
			continue;
		}
		drawText(row.name, STAT_SIZE, UISkin::TEXT, { STATS_X, y }, { 0.f, 0.5f });
		if (row.next.empty() && m_isMaxLevel)   // A category's contents: the value at the column's end
		{
			drawText(row.now, STAT_SIZE, UISkin::NUMBER, { STATS_X + 385.f, y }, { 1.f, 0.5f });
			y += 43.f;
			continue;
		}
		drawText(row.now, STAT_SIZE, UISkin::TEXT, { STATS_X + 250.f, y }, { 1.f, 0.5f });
		if (!m_isMaxLevel)
		{
			drawText(">", STAT_SIZE, UISkin::TITLE, { STATS_X + 285.f, y }, { 0.5f, 0.5f });
			drawText(row.next, STAT_SIZE, row.isBetter ? BETTER : UISkin::TEXT, { STATS_X + 385.f, y }, { 1.f, 0.5f });
		}
		y += 43.f;
	}
	if (!m_evolutionLine.empty())
		drawText(UISkin::wrap_text(m_evolutionLine, 28, 420.f), 28, UISkin::NUMBER, { STATS_X, y + 10.f }, { 0.f, 0.f });

	// The price is on the button; under it, what is missing
	auto player = m_dataLoader->get_player_data().lock();
	if (!player || m_upgradeIcons.empty() || m_isMaxLevel || m_entries[m_currentIndex].kind == Entry::Kind::CATEGORY) return;
	const float centerX = UPGRADE_BUTTON.position.x + UPGRADE_BUTTON.size.x / 2.f;
	if (player->shells < m_cost)
		drawText("Need " + std::to_string(m_cost - player->shells) + " more shells", 28, CORAL, { centerX, UPGRADE_BUTTON.position.y + UPGRADE_BUTTON.size.y + 34.f }, { 0.5f, 0.5f });
}

/*
 * Draws the side cards, then the selected one on top, the arrows, the button and the description panel.
 */
void UIUpgradeSlider::render(sf::RenderWindow& window)
{
	// The shelf the cards stand on, the page dots (they follow a slide), then the cards: the biggest (the one in the
	// middle, or passing it while sliding) on top
	UIUnitCard::draw_shelf(window, SHELF_TOP, CARDS_BOTTOM + 24.f, CARDS_BOTTOM);
	UIUnitCard::draw_page_dots(window, { 960.f, DOTS_Y }, static_cast<int>(m_upgradeIcons.size()),
		static_cast<float>(m_currentIndex) - drag_offset() / ICON_SPACING);
	const int front = UIUnitCard::biggest(m_upgradeIcons);
	for (int i = 0; i < static_cast<int>(m_upgradeIcons.size()); ++i)
		if (i != front) m_upgradeIcons[i]->render(window);
	if (front >= 0)
		m_upgradeIcons[front]->render(window);

	m_previousButton.render(window);
	m_nextButton.render(window);
	m_upgradeButton.render(window);

	m_descriptionPanel.render(window);
	window.draw(m_unitName);
	// The description and the abilities, clipped to the left column (scrolled when too long)
	{
		const sf::FloatRect area = description_area();
		const sf::View view = window.getView();
		const sf::Vector2f windowSize(window.getSize());
		const sf::Vector2i topLeft = window.mapCoordsToPixel(area.position, view);
		const sf::Vector2i bottomRight = window.mapCoordsToPixel(area.position + area.size, view);
		sf::View clipped = view;
		clipped.setScissor({ { std::clamp(topLeft.x / windowSize.x, 0.f, 1.f), std::clamp(topLeft.y / windowSize.y, 0.f, 1.f) },
			{ std::clamp((bottomRight.x - topLeft.x) / windowSize.x, 0.f, 1.f), std::clamp((bottomRight.y - topLeft.y) / windowSize.y, 0.f, 1.f) } });
		window.setView(clipped);
		const float scrolled = std::round(m_descriptionScroll);
		sf::RenderStates states;
		states.transform.translate({ 0.f, -scrolled });
		window.draw(m_description, states);
		m_abilities.set_position({ DESCRIPTION_PANEL.position.x + TEXT_PADDING, m_abilitiesTop - scrolled });
		m_abilities.render_lines(window);
		window.setView(view);

		const float hidden = description_hidden();
		if (hidden > 1.f)
		{
			const float contentHeight = area.size.y + hidden;
			const float thumbHeight = std::max(40.f, area.size.y * area.size.y / contentHeight);
			sf::RectangleShape track({ 8.f, area.size.y });
			track.setPosition({ area.position.x + area.size.x + 6.f, area.position.y });
			track.setFillColor(sf::Color(255, 255, 255, 40));
			window.draw(track);
			sf::RectangleShape thumb({ 8.f, thumbHeight });
			thumb.setPosition({ track.getPosition().x, area.position.y + (area.size.y - thumbHeight) * std::clamp(m_descriptionScroll / hidden, 0.f, 1.f) });
			thumb.setFillColor(UISkin::NUMBER);
			window.draw(thumb);
		}
	}
	render_upgrade_info(window);
	m_abilities.render_hover_bubble(window);   // Last: its bubble goes over everything
}

/*
 * Handles keyboard and mouse input.
 * Left/Right keys navigate the slider.
 * Mouse clicks select an icon if clicked.
 */
void UIUpgradeSlider::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
	if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
	{
		if (keyPressed->code == sf::Keyboard::Key::Left)
			move_to_index(m_currentIndex - 1);
		else if (keyPressed->code == sf::Keyboard::Key::Right)
			move_to_index(m_currentIndex + 1);
	}

	// The cards: sliding sideways anywhere on the screen moves them with the finger, not only on them (flung, they
	// glide on; a tap catches them); a tap on a side card brings it to the middle (on release, so a swipe that starts
	// on a card doesn't pick it). The arrows (drawn over the far cards), the UPGRADE button and the back button keep
	// their presses
	const auto is_on_arrow = [this](sf::Vector2f point)
		{
			return (m_previousButton.is_visible() && m_previousButton.get_bounds().contains(point))
				|| (m_nextButton.is_visible() && m_nextButton.get_bounds().contains(point));
		};
	const auto is_slide_area = [&](sf::Vector2f point)
		{
			return UISkin::is_slide_area(point) && !is_on_arrow(point) && !UPGRADE_BUTTON.contains(point);
		};
	if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>(); mousePressed && mousePressed->button == sf::Mouse::Button::Left)
	{
		const sf::Vector2f mousePos = window.mapPixelToCoords(mousePressed->position);
		const bool wasGliding = m_glide.isActive;
		m_glide.isActive = false;
		if (is_slide_area(mousePos))
		{
			// A glide stops under the finger, which can slide the cards on from there
			m_caughtGlide = wasGliding;
			m_dragBase = drag_offset();
			if (wasGliding) follow_drag(m_dragBase);
			m_swipe.press(mousePos);
		}
		else if (wasGliding)
			move_to_index(m_currentIndex, true);   // Elsewhere: they land on the selected card at once
	}
	else if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
	{
		// Sliding sideways: the cards follow the finger
		m_swipe.move(window.mapPixelToCoords(mouseMoved->position));
		const sf::Vector2f drag = m_swipe.last - m_swipe.start;
		if (m_swipe.isDown && (m_isFollowing || (std::abs(drag.x) > 12.f && std::abs(drag.x) > 1.4f * std::abs(drag.y))))
		{
			m_isFollowing = true;
			follow_drag(m_dragBase + drag.x);
		}
	}
	else if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>(); mouseReleased && m_swipe.isDown)
	{
		const sf::Vector2f mousePos = window.mapPixelToCoords(mouseReleased->position);
		const float dragX = m_dragBase + mousePos.x - m_swipe.start.x;
		m_swipe.release(mousePos);
		if (m_isFollowing || m_caughtGlide)
		{
			// Let go (or a glide caught by a tap): flung, they glide on; else they settle on the nearest card
			m_isFollowing = false;
			release_slide(dragX, m_swipe.velocity.x);
			m_caughtGlide = false;
		}
		else
			for (int i = 0; i < static_cast<int>(m_upgradeIcons.size()); ++i)
				if (m_upgradeIcons[i]->get_bounds().contains(mousePos))
				{
					// A side card comes to the middle; the category card in the middle opens
					if (i != m_currentIndex) move_to_index(i);
					else if (m_entries[i].kind == Entry::Kind::CATEGORY) open_category(static_cast<Category>(m_entries[i].id));
					break;
				}
	}

	m_previousButton.handle_event(event, window);
	m_nextButton.handle_event(event, window);
	m_upgradeButton.handle_event(event, window);
	m_abilities.handle_event(event, window);

	// The description column: dragged up and down (or the wheel) when it doesn't fit
	const float hidden = description_hidden();
	if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>(); pressed && hidden > 0.f
		&& description_area().contains(window.mapPixelToCoords(pressed->position)))
	{
		m_isDescriptionDragging = true;
		m_descriptionPressY = window.mapPixelToCoords(pressed->position).y;
		m_descriptionPressScroll = m_descriptionScroll;
	}
	else if (const auto* moved = event.getIf<sf::Event::MouseMoved>(); moved && m_isDescriptionDragging)
		m_descriptionScroll = std::clamp(m_descriptionPressScroll - (window.mapPixelToCoords(moved->position).y - m_descriptionPressY), 0.f, hidden);
	else if (event.is<sf::Event::MouseButtonReleased>())
		m_isDescriptionDragging = false;
	else if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>(); wheel && description_area().contains(window.mapPixelToCoords(wheel->position)))
		m_descriptionScroll = std::clamp(m_descriptionScroll - wheel->delta * 50.f, 0.f, hidden);
}

sf::FloatRect UIUpgradeSlider::description_area() const
{
	const float top = DESCRIPTION_PANEL.position.y + 72.f;
	return { { DESCRIPTION_PANEL.position.x + TEXT_PADDING - 8.f, top },
		{ DESCRIPTION_WIDTH + 16.f, DESCRIPTION_PANEL.position.y + DESCRIPTION_PANEL.size.y - 16.f - top } };
}

float UIUpgradeSlider::description_hidden() const
{
	const float bottom = m_abilitiesTop + m_abilities.get_height();
	const sf::FloatRect area = description_area();
	return std::max(0.f, bottom + 6.f - (area.position.y + area.size.y));
}

/*
 * The cards follow a finger sliding them: each at its place plus how far it has slid (no glide while held), the
 * one passing the middle growing to the selected size and the one leaving it shrinking; the nearest is the
 * highlighted one
 */
void UIUpgradeSlider::follow_drag(float dragX)
{
	for (int i = 0; i < static_cast<int>(m_upgradeIcons.size()); ++i)
	{
		const float slot = static_cast<float>(i - m_currentIndex) + dragX / ICON_SPACING;   // Cards from the middle
		m_upgradeIcons[i]->place_at(1920.f / 2.f + slot * ICON_SPACING, baseIconSize.x * UIUnitCard::carousel_scale(slot, CENTER_SCALE, SIDE_SCALE));
		m_upgradeIcons[i]->select(std::abs(slot) < 0.5f);
	}
}

/*
 * Animates the slider to focus on a new icon index.
 * Updates icon positions, sizes, and selection states.
 */

// Where the cards settle when let go slowly: the nearest one (a short flick from rest still moves one card)
int UIUpgradeSlider::snap_index(float dragX, bool allowsFlick) const
{
	int target = static_cast<int>(std::lround(static_cast<float>(m_currentIndex) - dragX / ICON_SPACING));
	if (allowsFlick && target == m_currentIndex && std::abs(dragX) > UISwipe::DISTANCE) target += dragX < 0.f ? 1 : -1;
	return std::clamp(target, 0, std::max(0, static_cast<int>(m_upgradeIcons.size()) - 1));
}

int UIUpgradeSlider::get_selected_uid() const
{
	if (m_entries.empty() || m_entries[m_currentIndex].kind != Entry::Kind::FISH) return -1;
	return m_entries[m_currentIndex].id;
}
void UIUpgradeSlider::select_unit(int uid)
{
	auto player = m_dataLoader->get_player_data().lock();
	if (!player) return;
	m_category = category_of(uid);   // Among its rarity's cards
	build_entries(*player);
	const int index = index_of(Entry::Kind::FISH, uid);
	if (index < 0) return;
	m_currentIndex = index;
	layout_cards();   // Laid out round it at once
}
float UIUpgradeSlider::drag_offset() const
{
	if (m_upgradeIcons.empty()) return 0.f;
	const auto& card = m_upgradeIcons[m_currentIndex];
	return card->get_position().x + card->get_size().x / 2.f - 1920.f / 2.f;
}

/*
 * Let go: flung (faster than UISwipe::GLIDE_SPEED), the cards glide on and slow down, landing on the card that speed
 * carries them to; it's selected at once, so its name and price show while they glide. Slower, they settle on the
 * nearest card
 */
void UIUpgradeSlider::release_slide(float dragX, float speed)
{
	const int last = static_cast<int>(m_upgradeIcons.size()) - 1;
	if (std::abs(speed) <= UISwipe::GLIDE_SPEED || last < 0)
	{
		move_to_index(snap_index(dragX, !m_caughtGlide && std::abs(m_dragBase) < 20.f), true);
		return;
	}
	const int target = std::clamp(UIGlide::landing(static_cast<float>(m_currentIndex), dragX, speed, ICON_SPACING), 0, last);
	if (target != m_currentIndex) play_sound("ui_select");
	m_glide.start(dragX + static_cast<float>(target - m_currentIndex) * ICON_SPACING, 0.f);   // Counted from the target
	m_currentIndex = target;
}

void UIUpgradeSlider::move_to_index(int newIndex, bool force)
{
	if (newIndex < 0 || newIndex >= static_cast<int>(m_upgradeIcons.size()) || (newIndex == m_currentIndex && !force))
		return;
	m_glide.isActive = false;   // The arrows, the keys or a tap take over from a glide

	if (newIndex != m_currentIndex) play_sound("ui_select");

	const float centerX = 1920.f / 2.0f - baseIconSize.x / 2.f;

	for (int i = 0; i < static_cast<int>(m_upgradeIcons.size()); ++i)
	{
		auto& icon = m_upgradeIcons[i];
		float targetX = centerX + (i - newIndex) * ICON_SPACING;
		if (i == newIndex)
			targetX -= CENTER_OFFSET_X;

		sf::Vector2f targetSize = baseIconSize * ((i == newIndex) ? CENTER_SCALE : SIDE_SCALE);

		icon->tweenX = tweeny::from(icon->get_position().x).to(targetX).during(TWEEN_DURATION * 60.0f).via(tweeny::easing::quadraticOut);
		icon->tweenWidth = tweeny::from(icon->get_size().x).to(targetSize.x).during(TWEEN_DURATION * 60.0f).via(tweeny::easing::quadraticOut);
		icon->tweenHeight = tweeny::from(icon->get_size().y).to(targetSize.y).during(TWEEN_DURATION * 60.0f).via(tweeny::easing::quadraticOut);

		icon->select(i == newIndex); // Highlight selected icon
	}

	m_currentIndex = newIndex;
}

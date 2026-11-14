#include "UIUpgradeSlider.h"
#include "UIToast.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"

#include <algorithm>
#include <cmath>

namespace
{
    const sf::FloatRect UPGRADE_BUTTON{ { 60.f, 690.f }, { 470.f, 140.f } };
    const sf::FloatRect DESCRIPTION_PANEL{ { 570.f, 660.f }, { 1290.f, 396.f } };   // Down to the bottom: room for the abilities
    const float TEXT_PADDING = 50.f;
    const float ARROW_SIZE = 96.f;
    const float ARROW_Y = 412.f;
    const unsigned int NAME_SIZE = 44;
    const unsigned int DESCRIPTION_SIZE = 30;
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

	if (auto player = m_dataLoader->get_player_data().lock())
	{
		int unitUid = m_availableUpgrades[m_currentIndex];      // Currently selected unit
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
			m_describedUid = -1; // Name and description again

			m_dataLoader->save_progress();
		}
	}
}

/*
 * Initializes the array of UIUpgradeIcon objects based on player data.
 * Includes owned units and units waiting to be unlocked.
 */
void UIUpgradeSlider::init_icons()
{
	m_upgradeIcons.clear();
	m_availableUpgrades.clear();

	auto playerData = m_dataLoader->get_player_data().lock();
	if (!playerData) return;

	for (auto& [uid, _] : playerData->ownedUnits)
		m_availableUpgrades.push_back(uid);

	for (int uid : playerData->unitsWaitingToBeUnlocked)
	{
		if (playerData->ownedUnits.find(uid) == playerData->ownedUnits.end())
			m_availableUpgrades.push_back(uid);
	}

	std::sort(m_availableUpgrades.begin(), m_availableUpgrades.end(),
		[&](int a, int b) {
			bool aOwned = playerData->is_unit_owned(a);
			bool bOwned = playerData->is_unit_owned(b);
			if (aOwned != bOwned) return aOwned > bOwned; // unlocked first
			return a < b;                                 // then by UID
		});

	m_currentIndex = std::clamp(m_currentIndex, 0, std::max(0, static_cast<int>(m_availableUpgrades.size()) - 1));
	const float centerX = 1920.f / 2.0f - baseIconSize.x / 2.f;

	for (int i = 0; i < static_cast<int>(m_availableUpgrades.size()); ++i)
	{
		auto unitUid = m_availableUpgrades[i];
		auto unitLevel = playerData->get_unit_level(unitUid);
		auto unitForm = playerData->get_unit_form(unitUid);
		auto unitData = m_dataLoader->get_unit_data(unitUid, unitForm);

		auto icon = std::make_shared<UIUpgradeIcon>(m_dataLoader->get_unit_icon_texture_path(unitUid, unitForm));
		icon->select(i == m_currentIndex);
		icon->set_unit_name(unitData->name);
		icon->set_unit_level(unitLevel);
		icon->set_extra_levels(playerData->get_unit_extra_levels(unitUid));
		icon->set_upgrade_cost(calculate_upgrade_cost(unitUid, unitLevel));

		float posX = centerX + (i - m_currentIndex) * ICON_SPACING;
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

	for (int i = 0; i < static_cast<int>(m_upgradeIcons.size()); ++i)
	{
		auto& icon = m_upgradeIcons[i];
		const int uid = m_availableUpgrades[i];
		if (player)
		{
			const int level = player->get_unit_level(uid);
			icon->set_affordable(player->shells >= calculate_upgrade_cost(uid, level));
			icon->set_in_lineup(std::find(player->equippedUnits.begin(), player->equippedUnits.end(), uid) != player->equippedUnits.end());
		}
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

	// The button: UNLOCK! for a new unit, greyed out when maxed or too expensive
	const int uid = m_availableUpgrades[m_currentIndex];
	const int level = player->get_unit_level(uid);
	m_upgradeButton.set_text(level <= 0 ? "UNLOCK!" : "UPGRADE!");
	m_upgradeButton.set_enabled(level < UIUpgradeIcon::MAX_LEVEL && player->shells >= calculate_upgrade_cost(uid, level));
	m_upgradeButton.set_cost(level < UIUpgradeIcon::MAX_LEVEL ? calculate_upgrade_cost(uid, level) : -1);   // The price on the button
	if (level >= UIUpgradeIcon::MAX_LEVEL) m_upgradeButton.set_text("MAX LEVEL");
	m_upgradeButton.update(deltaTime);

	// Name and description of the selected unit (wrapped to the panel)
	if (uid != m_describedUid)
	{
		auto unitData = m_dataLoader->get_unit_data(uid, player->get_unit_form(uid));
		m_unitName.setString(unitData->name);
		m_description.setString(UISkin::wrap_text(unitData->description, DESCRIPTION_SIZE, DESCRIPTION_WIDTH, UISkin::regular_font()));
		m_abilities.set_abilities(unitData->abilities);
		m_describedUid = uid;
		m_describedLevel = -1;   // The numbers too

		// Name centered, description and abilities left-aligned under it
		UISkin::set_text_anchor(m_unitName, { 0.5f, 0.5f });
		m_unitName.setPosition({ DESCRIPTION_PANEL.position.x + TEXT_PADDING + DESCRIPTION_WIDTH / 2.f, DESCRIPTION_PANEL.position.y + 44.f });
		UISkin::set_text_anchor(m_description, { 0.f, 0.f });
		m_description.setPosition({ DESCRIPTION_PANEL.position.x + TEXT_PADDING, DESCRIPTION_PANEL.position.y + 80.f });

		const float descriptionBottom = m_description.getGlobalBounds().position.y + m_description.getGlobalBounds().size.y;
		m_abilities.set_position({ DESCRIPTION_PANEL.position.x + TEXT_PADDING, descriptionBottom + 16.f });
	}
	if (uid != m_describedUid || level != m_describedLevel) describe_upgrade(uid, level);
	m_describedUid = uid;
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

	float y = DESCRIPTION_PANEL.position.y + 44.f;
	drawText(m_isMaxLevel ? "MAX LEVEL" : "NEXT LEVEL", 34, UISkin::TITLE, { STATS_X + 190.f, y }, { 0.5f, 0.5f });
	y += 62.f;
	for (const StatRow& row : m_statRows)
	{
		if (row.isExtra)
		{
			// The extra levels: purple, one value at the end of the row (no arrow: upgrading doesn't change them)
			drawText(row.name, STAT_SIZE, UISkin::EXTRA, { STATS_X, y }, { 0.f, 0.5f });
			drawText(row.next, STAT_SIZE, UISkin::EXTRA, { STATS_X + (m_isMaxLevel ? 250.f : 385.f), y }, { 1.f, 0.5f });
			y += 48.f;
			continue;
		}
		drawText(row.name, STAT_SIZE, UISkin::TEXT, { STATS_X, y }, { 0.f, 0.5f });
		drawText(row.now, STAT_SIZE, UISkin::TEXT, { STATS_X + 250.f, y }, { 1.f, 0.5f });
		if (!m_isMaxLevel)
		{
			drawText(">", STAT_SIZE, UISkin::TITLE, { STATS_X + 285.f, y }, { 0.5f, 0.5f });
			drawText(row.next, STAT_SIZE, row.isBetter ? BETTER : UISkin::TEXT, { STATS_X + 385.f, y }, { 1.f, 0.5f });
		}
		y += 48.f;
	}
	if (!m_evolutionLine.empty())
		drawText(UISkin::wrap_text(m_evolutionLine, 28, 420.f), 28, UISkin::NUMBER, { STATS_X, y + 10.f }, { 0.f, 0.f });

	// The price is on the button; under it, what is missing
	auto player = m_dataLoader->get_player_data().lock();
	if (!player || m_upgradeIcons.empty() || m_isMaxLevel) return;
	const float centerX = UPGRADE_BUTTON.position.x + UPGRADE_BUTTON.size.x / 2.f;
	if (player->shells < m_cost)
		drawText("Need " + std::to_string(m_cost - player->shells) + " more shells", 28, CORAL, { centerX, UPGRADE_BUTTON.position.y + UPGRADE_BUTTON.size.y + 34.f }, { 0.5f, 0.5f });
}

/*
 * Draws the side cards, then the selected one on top, the arrows, the button and the description panel.
 */
void UIUpgradeSlider::render(sf::RenderWindow& window)
{
	for (int i = 0; i < static_cast<int>(m_upgradeIcons.size()); ++i)
		if (i != m_currentIndex) m_upgradeIcons[i]->render(window);
	if (!m_upgradeIcons.empty())
		m_upgradeIcons[m_currentIndex]->render(window);

	m_previousButton.render(window);
	m_nextButton.render(window);
	m_upgradeButton.render(window);

	m_descriptionPanel.render(window);
	window.draw(m_unitName);
	window.draw(m_description);
	render_upgrade_info(window);
	m_abilities.render(window);   // Last: its bubble goes over everything
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

	// The cards: a swipe sideways moves by one card; a tap on a side card brings it to the middle (on release, so a
	// swipe that starts on a card doesn't pick it). The arrows, drawn over the far cards, come first
	const auto is_on_arrow = [this](sf::Vector2f point)
		{
			return (m_previousButton.is_visible() && m_previousButton.get_bounds().contains(point))
				|| (m_nextButton.is_visible() && m_nextButton.get_bounds().contains(point));
		};
	if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
	{
		const sf::Vector2f mousePos = window.mapPixelToCoords(mousePressed->position);
		if (mousePressed->button == sf::Mouse::Button::Left && mousePos.y < DESCRIPTION_PANEL.position.y && !is_on_arrow(mousePos))
			m_swipe.press(mousePos);
	}
	else if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
	{
		m_swipe.move(window.mapPixelToCoords(mouseMoved->position));
	}
	else if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>(); mouseReleased && m_swipe.isDown)
	{
		const sf::Vector2f mousePos = window.mapPixelToCoords(mouseReleased->position);
		if (const int direction = m_swipe.release(mousePos))
			move_to_index(m_currentIndex - direction);
		else
			for (int i = 0; i < static_cast<int>(m_upgradeIcons.size()); ++i)
				if (i != m_currentIndex && m_upgradeIcons[i]->get_bounds().contains(mousePos))
				{
					move_to_index(i);
					break;
				}
	}

	m_previousButton.handle_event(event, window);
	m_nextButton.handle_event(event, window);
	m_upgradeButton.handle_event(event, window);
	m_abilities.handle_event(event, window);
}

/*
 * Animates the slider to focus on a new icon index.
 * Updates icon positions, sizes, and selection states.
 */
void UIUpgradeSlider::move_to_index(int newIndex)
{
	if (newIndex < 0 || newIndex >= static_cast<int>(m_upgradeIcons.size()) || newIndex == m_currentIndex)
		return;

	play_sound("ui_select");

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

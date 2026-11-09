#include "UIUpgradeSlider.h"
#include "../UISkin.h"
#include "../../core/DataLoader.h"

#include <algorithm>
#include <cmath>

namespace
{
    const sf::FloatRect UPGRADE_BUTTON{ { 60.f, 690.f }, { 470.f, 140.f } };
    const sf::FloatRect DESCRIPTION_PANEL{ { 570.f, 670.f }, { 1290.f, 252.f } };
    const float ARROW_SIZE = 96.f;
    const float ARROW_Y = 412.f;
    const unsigned int NAME_SIZE = 44;
    const unsigned int DESCRIPTION_SIZE = 30;
}

/*
 * Initializes the upgrade slider: upgrade button, arrows, description panel and the cards.
 */
UIUpgradeSlider::UIUpgradeSlider(std::shared_ptr<DataLoader> dataLoader) :
	m_dataLoader(dataLoader),
	m_upgradeButton(UPGRADE_BUTTON.size, UPGRADE_BUTTON.position, "UPGRADE!"),
	m_previousButton(ARROW_SIZE, { 36.f, ARROW_Y }, "icon_play"),
	m_nextButton(ARROW_SIZE, { 1920.f - 36.f - ARROW_SIZE, ARROW_Y }, "icon_play"),
	m_descriptionPanel(DESCRIPTION_PANEL.size, DESCRIPTION_PANEL.position, UISkin::PANEL_DARK, 1.2f),
	m_unitName(UISkin::heavy_font()),
	m_description(UISkin::heavy_font())
{
	m_upgradeButton.set_text_size(60);
	m_upgradeButton.set_callback([this]() { upgrade_selected(); });

	m_previousButton.set_icon_flipped(true);
	m_previousButton.set_callback([this]() { move_to_index(m_currentIndex - 1); });
	m_nextButton.set_callback([this]() { move_to_index(m_currentIndex + 1); });

	UISkin::style_text(m_unitName, NAME_SIZE, UISkin::NUMBER);
	UISkin::style_text(m_description, DESCRIPTION_SIZE);
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
			if (player->is_unit_owned(unitUid))
				player->upgrade_unit(unitUid); // Upgrade if owned
			else
				player->unlock_unit(unitUid); // Unlock if not owned

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
	int baseCost = m_dataLoader->get_unit_data(uid, 0)->baseUpgradeCost;
	if (level <= 0) return baseCost;

	float factor = 1.5f;          // Level-up multiplier
	float evolutionDiscount = 0.7f; // Discount after evolution
	std::array<int, 2> evolutionLevels = { 10, 30 }; // Levels at which unit evolves

	int evolutions = 0;
	for (int eLevel : evolutionLevels)
	{
		if (level >= eLevel)
			evolutions++;
		else
			break;
	}

	float normalCost = baseCost * std::pow(factor, level);
	float upgradeCost = normalCost * std::pow(evolutionDiscount, evolutions);

	return static_cast<int>(upgradeCost);
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
	m_upgradeButton.update(deltaTime);

	// Name and description of the selected unit (wrapped to the panel)
	if (uid != m_describedUid)
	{
		auto unitData = m_dataLoader->get_unit_data(uid, player->get_unit_form(uid));
		m_unitName.setString(unitData->name);
		m_description.setString(UISkin::wrap_text(unitData->description, DESCRIPTION_SIZE, DESCRIPTION_PANEL.size.x - 100.f));
		m_describedUid = uid;

		UISkin::set_text_anchor(m_unitName, { 0.5f, 0.5f });
		m_unitName.setPosition({ DESCRIPTION_PANEL.position.x + DESCRIPTION_PANEL.size.x / 2.f, DESCRIPTION_PANEL.position.y + 46.f });
		UISkin::set_text_anchor(m_description, { 0.5f, 0.f });
		m_description.setPosition({ DESCRIPTION_PANEL.position.x + DESCRIPTION_PANEL.size.x / 2.f, DESCRIPTION_PANEL.position.y + 86.f });
	}
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

	if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
	{
		if (mousePressed->button == sf::Mouse::Button::Left)
		{
			sf::Vector2f mousePos = window.mapPixelToCoords(mousePressed->position);

			// The arrows are drawn over the far cards: they come first
			const bool isOnArrow = (m_previousButton.is_visible() && m_previousButton.get_bounds().contains(mousePos))
				|| (m_nextButton.is_visible() && m_nextButton.get_bounds().contains(mousePos));

			for (int i = 0; i < static_cast<int>(m_upgradeIcons.size()) && !isOnArrow; ++i)
			{
				if (i != m_currentIndex && m_upgradeIcons[i]->get_bounds().contains(mousePos))
				{
					move_to_index(i);
					break;
				}
			}
		}
	}

	m_previousButton.handle_event(event, window);
	m_nextButton.handle_event(event, window);
	m_upgradeButton.handle_event(event, window);
}

/*
 * Animates the slider to focus on a new icon index.
 * Updates icon positions, sizes, and selection states.
 */
void UIUpgradeSlider::move_to_index(int newIndex)
{
	if (newIndex < 0 || newIndex >= static_cast<int>(m_upgradeIcons.size()) || newIndex == m_currentIndex)
		return;

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

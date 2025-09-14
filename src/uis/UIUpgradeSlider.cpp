#include "UIUpgradeSlider.h"
#include "../core/DataLoader.h"

/*
 * Initializes the upgrade slider UI component.
 * Sets up the lower part background, description text, upgrade button, and initializes icons.
 */
UIUpgradeSlider::UIUpgradeSlider(std::shared_ptr<DataLoader> dataLoader) :
	m_dataLoader(dataLoader),
	m_lowerPart(sf::RectangleShape(sf::Vector2f{ 1920.f, 360.f })), // Bottom panel for descriptions
	m_description(m_font),                                         // Text showing unit description
	m_upgradeButton(UIButtonElement({ 200.f, 150.f }, { 50.f, 800.f }, "UPGRADE")) // Upgrade button
{
	// Configure the lower panel appearance
	m_lowerPart.setFillColor(sf::Color(65, 145, 195));
	m_lowerPart.setPosition({ 0.f, 1080.f - 360.f });

	// Load font for the description text
	auto success = m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf");
	m_description.setFillColor(sf::Color::White);
	m_description.setCharacterSize(58);
	m_description.setOutlineColor(sf::Color::Black);
	m_description.setOutlineThickness(2.f);

	// Setup upgrade button callback
	m_upgradeButton.set_callback([this]()
		{
			// Get current player data
			if (auto player = m_dataLoader->get_player_data().lock())
			{
				int unitUid = m_availableUpgrades[m_currentIndex];      // Currently selected unit
				int level = player->get_unit_level(unitUid);           // Current level
				int upgradeCost = calculate_upgrade_cost(unitUid, level); // Calculate cost

				// Attempt to spend shells to upgrade/unlock unit
				if (player->spend_shells(upgradeCost))
				{
					if (player->is_unit_owned(unitUid))
						player->upgrade_unit(unitUid); // Upgrade if owned
					else
						player->unlock_unit(unitUid); // Unlock if not owned

					// Refresh the selected icon
					auto icon = m_upgradeIcons[m_currentIndex];
					auto unitLevel = player->get_unit_level(unitUid);
					auto unitForm = player->get_unit_form(unitUid);
					auto unitData = m_dataLoader->get_unit_data(unitUid, unitForm);

					icon->set_unit_level(unitLevel);
					icon->set_upgrade_cost(calculate_upgrade_cost(unitUid, unitLevel));
					icon->set_unit_name(unitData->name);
					icon->set_texture(m_dataLoader->get_unit_icon_texture_path(unitUid, unitForm));

					m_description.setString(unitData->description);

					icon->set_unit_level(unitLevel);
					icon->set_upgrade_cost(calculate_upgrade_cost(unitUid, unitLevel));
				}
			}
		}
	);

	// Initialize the icons for all available upgrades
	init_icons();
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

	// Add owned units first
	for (auto& [uid, _] : playerData->ownedUnits)
		m_availableUpgrades.push_back(uid);

	// Add units waiting to be unlocked (at the end)
	for (int uid : playerData->unitsWaitingToBeUnlocked)
	{
		if (playerData->ownedUnits.find(uid) == playerData->ownedUnits.end())
			m_availableUpgrades.push_back(uid);
	}

	// Sort units: unlocked units first, then by UID
	std::sort(m_availableUpgrades.begin(), m_availableUpgrades.end(),
		[&](int a, int b) {
			bool aOwned = playerData->is_unit_owned(a);
			bool bOwned = playerData->is_unit_owned(b);

			if (aOwned != bOwned) return aOwned > bOwned; // unlocked first
			return a < b;                                 // then by UID
		});

	sf::Vector2f screenCenter(1920.f / 2.0f - baseIconSize.x / 2.f,
		1080.f / 2.0f - baseIconSize.y / 2.f + 300.f);

	// Create icons for each available upgrade
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

		// Position icons horizontally, with spacing
		float xOffset = (i - m_currentIndex) * ICON_SPACING;
		float posX = screenCenter.x + xOffset;
		if (i == m_currentIndex)
			posX -= CENTER_OFFSET_X;
		icon->set_position({ screenCenter.x + xOffset, screenCenter.y });

		// Set icon size (center icon larger)
		float scale = (i == m_currentIndex) ? CENTER_SCALE : SIDE_SCALE;
		icon->set_size(baseIconSize * scale);

		// Initialize tween animations for smooth transitions
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

	// Count how many evolutions have occurred
	for (int eLevel : evolutionLevels)
	{
		if (level >= eLevel)
			evolutions++;
		else
			break;
	}

	// Compute cost based on full level progression
	float normalCost = baseCost * std::pow(factor, level);

	// Apply evolution discounts
	float upgradeCost = normalCost * std::pow(evolutionDiscount, evolutions);

	return static_cast<int>(upgradeCost);
}

/*
 * Updates all icons and refreshes the description text for the currently selected unit.
 */
void UIUpgradeSlider::update(float deltaTime)
{
	for (auto& icon : m_upgradeIcons)
	{
		icon->update(deltaTime); // Update each icon (handles tweening)
	}

	// Update unit description
	m_description.setString(m_dataLoader->get_unit_data(m_availableUpgrades[m_currentIndex], m_dataLoader->get_player_data().lock()->get_unit_form(m_availableUpgrades[m_currentIndex]))->description);

	auto bounds = m_description.getLocalBounds();
	m_description.setOrigin({
		bounds.position.x + bounds.size.x / 2.f,
		bounds.position.y
		});

	m_description.setPosition({
		1920.f / 2.f,
		1080.f - (360.f / 2.f) - 150.f
		});
}

/*
 * Draws all icons, the lower panel, description text, and the upgrade button.
 */
void UIUpgradeSlider::render(sf::RenderWindow& window)
{
	for (auto& icon : m_upgradeIcons)
		icon->render(window);

	window.draw(m_lowerPart);
	window.draw(m_description);
	m_upgradeButton.render(window);
}

/*
 * Handles keyboard and mouse input.
 * Left/Right keys navigate the slider.
 * Mouse clicks select an icon if clicked.
 */
void UIUpgradeSlider::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
	// Keyboard input
	if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
	{
		int newIndex = m_currentIndex;

		if (keyPressed->code == sf::Keyboard::Key::Left)
			newIndex--;
		else if (keyPressed->code == sf::Keyboard::Key::Right)
			newIndex++;

		if (newIndex != m_currentIndex && newIndex >= 0 && newIndex < static_cast<int>(m_upgradeIcons.size()))
			move_to_index(newIndex);
	}

	// Mouse input
	if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
	{
		if (mousePressed->button == sf::Mouse::Button::Left)
		{
			sf::Vector2f mousePos = window.mapPixelToCoords({ mousePressed->position.x, mousePressed->position.y });

			for (int i = 0; i < static_cast<int>(m_upgradeIcons.size()); ++i)
			{
				auto& icon = m_upgradeIcons[i];
				if (icon->get_bounds().contains(mousePos))
				{
					if (i != m_currentIndex)
						move_to_index(i);
				}
			}
		}
	}

	// Forward event to icons and button
	for (auto& icon : m_upgradeIcons)
		icon->handle_event(event, window);

	m_upgradeButton.handle_event(event, window);
}

/*
 * Animates the slider to focus on a new icon index.
 * Updates icon positions, sizes, and selection states.
 */
void UIUpgradeSlider::move_to_index(int newIndex)
{
	if (newIndex < 0 || newIndex >= static_cast<int>(m_upgradeIcons.size()))
		return;

	sf::Vector2f screenCenter(1920.f / 2.0f - baseIconSize.x / 2.f,
		1080.f / 2.0f - baseIconSize.y / 2.f + 300.f);

	for (int i = 0; i < static_cast<int>(m_upgradeIcons.size()); ++i)
	{
		auto& icon = m_upgradeIcons[i];

		float targetX = screenCenter.x + (i - newIndex) * ICON_SPACING;
		if (i == newIndex)
			targetX -= CENTER_OFFSET_X;

		sf::Vector2f targetSize = baseIconSize * ((i == newIndex) ? CENTER_SCALE : SIDE_SCALE);

		// Animate position and size with tweeny
		icon->tweenX = tweeny::from(icon->get_position().x).to(targetX).during(TWEEN_DURATION * 60.0f).via(tweeny::easing::quadraticOut);
		icon->tweenWidth = tweeny::from(icon->get_size().x).to(targetSize.x).during(TWEEN_DURATION * 60.0f).via(tweeny::easing::quadraticOut);
		icon->tweenHeight = tweeny::from(icon->get_size().y).to(targetSize.y).during(TWEEN_DURATION * 60.0f).via(tweeny::easing::quadraticOut);

		icon->select(i == newIndex); // Highlight selected icon
	}

	m_currentIndex = newIndex;
}

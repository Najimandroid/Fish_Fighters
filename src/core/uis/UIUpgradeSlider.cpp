#include "UIUpgradeSlider.h"

#include "../DataLoader.h"

UIUpgradeSlider::UIUpgradeSlider(std::shared_ptr<DataLoader> dataLoader):
	m_dataLoader(dataLoader),
	m_lowerPart(sf::RectangleShape(sf::Vector2f{1920.f, 360.f})),
	m_description(m_font),
	m_upgradeButton(UIButtonElement({200.f, 150.f}, {50.f, 800.f}, "UPGRADE"))
{

	m_lowerPart.setFillColor(sf::Color(65, 145, 195));
	m_lowerPart.setPosition({ 0.f, 1080.f - 360.f });

	auto success = m_font.openFromFile("assets/fonts/MPLUSRounded1c-Medium.ttf");
	m_description.setFillColor(sf::Color::White);
	m_description.setCharacterSize(58);
	m_description.setOutlineColor(sf::Color::Black);
	m_description.setOutlineThickness(2.f);


	m_upgradeButton.set_callback([this]()
		{
			if (auto player = m_dataLoader->get_player_data().lock())
			{
				int unitUid = m_availableUpgrades[m_currentIndex];
				int level = player->get_unit_level(unitUid);

				int upgradeCost = calculate_upgrade_cost(unitUid, level);

				if (player->spend_shells(upgradeCost))
				{
					if (player->is_unit_owned(unitUid)) player->upgrade_unit(unitUid);
					else player->unlock_unit(unitUid);

					//Refresh selected icon
					auto icon = m_upgradeIcons[m_currentIndex];
					auto unitUid = m_availableUpgrades[m_currentIndex];
					auto unitLevel = player->get_unit_level(unitUid);

					icon->set_unit_level(unitLevel);
					icon->set_upgrade_cost(calculate_upgrade_cost(unitUid, unitLevel));
				}
			}
		}
	);

	init_icons();
}

void UIUpgradeSlider::init_icons()
{
    m_upgradeIcons.clear();

	auto playerData = m_dataLoader->get_player_data().lock();

    sf::Vector2f screenCenter(1920.f / 2.0f - baseIconSize.x / 2.f, 1080.f / 2.0f - baseIconSize.y / 2.f + 300.f);

    for (int i = 0; i < static_cast<int>(m_availableUpgrades.size()); ++i)
    {
		auto unitUid = m_availableUpgrades[i];
		auto unitLevel = playerData->get_unit_level(unitUid);
		auto unitData = m_dataLoader->get_unit_data(unitUid);

        auto icon = std::make_shared<UIUpgradeIcon>(m_dataLoader->get_unit_icon_texture_path(unitUid));
        icon->select(i == m_currentIndex);

		icon->set_unit_name(unitData->name);
		icon->set_unit_level(unitLevel);
		icon->set_upgrade_cost(calculate_upgrade_cost(unitUid, unitLevel));

		float xOffset = (i - m_currentIndex) * ICON_SPACING;
		float posX = screenCenter.x + xOffset;
		if (i == m_currentIndex)
			posX -= CENTER_OFFSET_X;
        icon->set_position({ screenCenter.x + xOffset, screenCenter.y });

        float scale = (i == m_currentIndex) ? CENTER_SCALE : SIDE_SCALE;
        icon->set_size(baseIconSize * scale);

		icon->tweenX = tweeny::from(icon->get_position().x).to(icon->get_position().x).during(1.0f);
		icon->tweenWidth = tweeny::from(icon->get_size().x).to(icon->get_size().x).during(1.0f);
		icon->tweenHeight = tweeny::from(icon->get_size().y).to(icon->get_size().y).during(1.0f);

        m_upgradeIcons.push_back(icon);
    }
}

int UIUpgradeSlider::calculate_upgrade_cost(int uid, int level)
{
	if (level <= 0) return 0;

	int baseCost = m_dataLoader->get_unit_data(uid)->baseUpgradeCost;

	float factor = 1.45f;
	float evolutionDiscount = 0.7f; //reduction factor applied after a unit evolved

	//Define evolution levels
	std::array<int, 2> evolutionLevels = { 10, 30 };

	//Counting how many evolutions the unit has had
	int evolutions = 0;
	int previousEvolutionLevel = 0;
	for (int eLevel : evolutionLevels)
	{
		if (level >= eLevel)
		{
			evolutions++;
			previousEvolutionLevel = eLevel;
		}
		else break;
	}

	int levelInCycle = (level - 1) - previousEvolutionLevel; //Level within the current evolution cycle

	//Calculate the cost
	float upgradeCost = baseCost * pow(factor, levelInCycle) * pow(evolutionDiscount, evolutions);
		
	return static_cast<int>(upgradeCost);
}

void UIUpgradeSlider::update(float deltaTime)
{
    for (auto& icon : m_upgradeIcons)
    {
        icon->update(deltaTime);
    }

	m_description.setString(m_dataLoader->get_unit_data(m_availableUpgrades[m_currentIndex])->description);

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

void UIUpgradeSlider::render(sf::RenderWindow& window)
{
    for (auto& icon : m_upgradeIcons)
        icon->render(window);

	window.draw(m_lowerPart);
	window.draw(m_description);

	m_upgradeButton.render(window);
}

void UIUpgradeSlider::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
	if (const auto* mouseButtonPressed = event.getIf<sf::Event::KeyPressed>())
	{
		int newIndex = m_currentIndex;

		if (mouseButtonPressed->code == sf::Keyboard::Key::Left)
			newIndex--;
		else if (mouseButtonPressed->code == sf::Keyboard::Key::Right)
			newIndex++;

		if (newIndex == m_currentIndex) return;
		if (newIndex < 0 || newIndex >= static_cast<int>(m_upgradeIcons.size())) return;


		sf::Vector2f screenCenter(1920.f / 2.0f - baseIconSize.x / 2.f, 1080.f / 2.0f - baseIconSize.y / 2.f + 300.f);

		for (int i = 0; i < static_cast<int>(m_upgradeIcons.size()); ++i)
		{
			auto icon = m_upgradeIcons[i];

			float targetX = screenCenter.x + (i - newIndex) * ICON_SPACING;

			if (i == newIndex)
				targetX -= CENTER_OFFSET_X; //apply offset to center the selected icon

			sf::Vector2f targetSize = baseIconSize * ((i == newIndex) ? CENTER_SCALE : SIDE_SCALE);

			icon->tweenX = tweeny::from(icon->get_position().x).to(targetX).during(TWEEN_DURATION * 60.0f).via(tweeny::easing::quadraticOut);
			icon->tweenWidth = tweeny::from(icon->get_size().x).to(targetSize.x).during(TWEEN_DURATION * 60.0f).via(tweeny::easing::quadraticOut);
			icon->tweenHeight = tweeny::from(icon->get_size().y).to(targetSize.y).during(TWEEN_DURATION * 60.0f).via(tweeny::easing::quadraticOut);

			if(i == m_currentIndex)
				icon->select(false);
			else if(i == newIndex)
				icon->select(true);
		}

		m_currentIndex = newIndex;
	}

    for (auto& icon : m_upgradeIcons)
        icon->handle_event(event, window);

	m_upgradeButton.handle_event(event, window);
}
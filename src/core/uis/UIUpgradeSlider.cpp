#include "UIUpgradeSlider.h"

#include "../DataLoader.h"

UIUpgradeSlider::UIUpgradeSlider(std::shared_ptr<DataLoader> dataLoader):
	m_dataLoader(dataLoader)
{
	init_icons();
}

void UIUpgradeSlider::init_icons()
{
    m_upgradeIcons.clear();

    sf::Vector2f screenCenter(1920.f / 2.0f - baseIconSize.x / 2.f, 1080.f / 2.0f - baseIconSize.y / 2.f);

    // Crée les icons à partir de m_availableUpgrades
    for (int i = 0; i < static_cast<int>(m_availableUpgrades.size()); ++i)
    {
        auto icon = std::make_shared<UIUpgradeIcon>("assets/images/textures/icons/placeholder.png");
        icon->select(i == m_currentIndex);

        float xOffset = (i - m_currentIndex) * ICON_SPACING;
        icon->set_position({ screenCenter.x + xOffset, screenCenter.y });

        float scale = (i == m_currentIndex) ? CENTER_SCALE : SIDE_SCALE;
        icon->set_size(baseIconSize * scale);

		icon->tweenX = tweeny::from(icon->get_position().x).to(icon->get_position().x).during(1.0f);
		icon->tweenWidth = tweeny::from(icon->get_size().x).to(icon->get_size().x).during(1.0f);
		icon->tweenHeight = tweeny::from(icon->get_size().y).to(icon->get_size().y).during(1.0f);

        m_upgradeIcons.push_back(icon);
    }
}

void UIUpgradeSlider::update(float deltaTime)
{
    for (auto& icon : m_upgradeIcons)
    {
        icon->update(deltaTime);
    }
}

void UIUpgradeSlider::render(sf::RenderWindow& window)
{
    for (auto& icon : m_upgradeIcons)
        icon->render(window);
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


		sf::Vector2f screenCenter(1920.f / 2.0f - baseIconSize.x / 2.f, 1080.f / 2.0f - baseIconSize.y / 2.f);

		for (int i = 0; i < static_cast<int>(m_upgradeIcons.size()); ++i)
		{
			auto icon = m_upgradeIcons[i];

			float targetX = screenCenter.x + (i - newIndex) * ICON_SPACING;
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

    // Passe l'event aux icons
    for (auto& icon : m_upgradeIcons)
        icon->handle_event(event, window);
}
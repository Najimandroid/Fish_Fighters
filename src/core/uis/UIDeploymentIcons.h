#pragma once

#include "UIBattleIcon.h"
#include "../DataLoader.h"

#include <set>

class Stage;

class UIDeploymentIcons : public UIElement
{
public:
	UIDeploymentIcons(std::shared_ptr<DataLoader> dataLoader, std::shared_ptr<Stage> stage);

	void init_icons();

	void update(float deltaTime) override;
	void render(sf::RenderWindow& window) override;
	void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

	sf::FloatRect get_bounds() const override;

private:

	std::array<std::shared_ptr<UIBattleIcon>, 10> m_icons;

	std::shared_ptr<DataLoader> m_dataLoader;
	std::weak_ptr<Stage> m_stage;
};
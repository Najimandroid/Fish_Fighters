#pragma once

#include "imgui.h"
#include "imgui-SFML.h"

//#include "tweeny.h"

#include <SFML/System/Vector2.hpp>
/*
#include <SFML/Graphics/View.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
*/
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "DataLoader.h"
#include "stages/Stage.h"

class Game
{
public:

	Game();

	void run_game_loop();

	//Events
	void poll_events();
	//SubEvents
	void resize_window(sf::Vector2u newSize);

	void center_window();

public:

	float deltaTime = 0.0f;

private:

	void init_camera();

	void debug_ui();

private:

	sf::RenderWindow m_window;
	sf::View m_camera;

	sf::Clock m_deltaClock;

	const int m_frameRate = 60;
	const sf::Vector2u m_logicalResolution = {1280, 720};

	std::shared_ptr<DataLoader> m_dataLoader;
	std::shared_ptr<Stage> m_stage;
};
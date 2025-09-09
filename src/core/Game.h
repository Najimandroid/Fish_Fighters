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
#include "uis/UIManager.h"

class Game
{
public:

	Game();

	void run_game_loop();
	void terminate();

	//Events
	void poll_events();
	//SubEvents
	void resize_window(sf::Vector2u newSize);

	void center_window();

public:

	float deltaTime = 0.0f;

private:

	void init_cameras();

	void debug_ui();

private:

	bool m_isPaused = false;
	bool m_isFaster = false;

	sf::RenderWindow m_window;
	sf::View m_uiCamera; //camera used to render UIs
	sf::View m_stageCamera; //camera used to render stages' elements

	sf::Clock m_deltaClock;

	const int m_frameRate = 60;
	const sf::Vector2u m_logicalResolution = {1920, 1080};

	//Mouse related
	float m_currentZoom = 1.f;
	bool m_isDragging = false;
	sf::Vector2i m_lastMousePos{ 0, 0 };

	std::shared_ptr<DataLoader> m_dataLoader;
	std::shared_ptr<Stage> m_stage;
	std::shared_ptr<UIManager> m_uiManager;
};
#include "Game.h"

#include <iostream>

Game::Game():
    m_dataLoader(std::make_shared<DataLoader>()),
    m_stage(std::make_shared<Stage>()),
	m_uiManager(std::make_shared<UIManager>())
{
    m_window.create(sf::VideoMode::getDesktopMode(), "Fish Fighers", sf::Style::None); //sf::Style::None
    m_window.setFramerateLimit(m_frameRate);
    center_window();

    ImGui::SFML::Init(m_window);

    m_dataLoader->load_all();

    m_uiManager->init(m_dataLoader, m_stage);
    m_stage->init(m_dataLoader, m_uiManager);

    init_cameras();
}

void Game::init_cameras()
{
    m_uiCamera.setSize(static_cast<sf::Vector2f>(m_logicalResolution));
    m_uiCamera.setCenter(m_uiCamera.getSize() / 2.f);

    m_stageCamera.setSize(static_cast<sf::Vector2f>(m_logicalResolution));
    m_stageCamera.setCenter(m_stageCamera.getSize() / 2.f);

    m_uiCamera.setViewport(sf::FloatRect{
    {0.f, 0.f},
    {1.f, 1.f}
    });

    m_stageCamera.setViewport(sf::FloatRect{
    {0.f, 0.f},
    {1.f, 1.f}
    });

    m_stageCamera.zoom(0.9f);
}

void Game::debug_ui()
{
#ifdef DEBUG_MODE
    ImGui::SFML::Update(m_window, m_deltaClock.restart());

    ImGui::ShowDemoWindow();

    ImGui::Begin("Hello, world!");
    ImGui::Button("Look at this pretty button");
    ImGui::End();

    ImGui::SFML::Render(m_window);
#endif
}

void Game::run_game_loop()
{
    /*
    //Test stuff

    sf::RectangleShape shape({ 500.f, 400.f });
    shape.setFillColor(sf::Color::Green);

    //Texture and Sprite test
    const sf::Texture texture("assets/images/smile.png");
    sf::Sprite sprite(texture);
    sprite.setScale({ 1, 1 });

    sf::Vector2f start(0.f, 0.f);
    sf::Vector2f end(1280.f - texture.getSize().x / 2.f, 720.f - texture.getSize().y / 2.f);
    auto tween = tweeny::from(start).to(end).during(60 * 10).via(tweeny::easing::bounceOut);

    //SoundBuffer and Sound test
    const sf::SoundBuffer buffer("assets/audios/Jazz_Background_Music.mp3");
    sf::Sound backgroundMusic(buffer);
    backgroundMusic.setLooping(true);
    //backgroundMusic.play();
    */

  
	m_uiManager->generate_fish_tank_uis();

    while (m_window.isOpen())
    {
        //Updating deltaTime
        deltaTime = m_deltaClock.restart().asSeconds();

        //Poll events
        poll_events();

        //Updating all systems
        m_stage->update(deltaTime);

        m_uiManager->update_uis(deltaTime);
        //sprite.setPosition(tween.step(1));

        //Rendering
        m_window.clear();
        //m_window.draw(shape);
        //m_window.draw(sprite);
        m_window.setView(m_stageCamera);
        m_stage->render(m_window);

        m_window.setView(m_uiCamera);
		m_uiManager->render_uis(m_window, m_uiCamera, m_stageCamera);

        debug_ui();
        m_window.display();
    }

    ImGui::SFML::Shutdown();
}

void Game::terminate()
{
	m_dataLoader->terminate();
}

void Game::poll_events()
{
    while (const auto event = m_window.pollEvent()) {
        //ImGui::SFML::ProcessEvent(m_window, *event);

        if (event->is<sf::Event::Closed>())
        {
            m_window.close();
        }

        m_uiManager->handle_ui_events(*event, m_window);

        if (const auto* e_keycode = event->getIf<sf::Event::KeyPressed>())
        {
            if (e_keycode->code == sf::Keyboard::Key::Numpad1)
            {
                m_window.setSize({ 1920, 1080 });
            }
            else
            if (e_keycode->code == sf::Keyboard::Key::Numpad2)
            {
                m_window.setSize({ 1280, 720 });
            }
            else
            if (e_keycode->code == sf::Keyboard::Key::Numpad3)
            {
                m_window.setSize({ 640, 360 });
            }
            else
            if (e_keycode->code == sf::Keyboard::Key::Escape)
            {
                m_window.close();
            }
            else
            if (e_keycode->code == sf::Keyboard::Key::P)
            {
                m_isPaused = true;
				m_isFaster = false;
            }
            else
            if (e_keycode->code == sf::Keyboard::Key::LShift)
            {
                m_isPaused = false;
                m_isFaster = true;
            }
            else
            if (e_keycode->code == sf::Keyboard::Key::R)
            {
                m_isPaused = false;
                m_isFaster = false;
            }
           /* if (e_keycode->code == sf::Keyboard::Key::A)
            {
                //std::cout << "spawn fish 1\n";
                m_stage->spawn_unit(m_dataLoader->get_unit_data(1));
            }
            if (e_keycode->code == sf::Keyboard::Key::E)
            {
                //std::cout << "spawn fish 2\n";
                m_stage->spawn_unit(m_dataLoader->get_unit_data(2));
            }*/
        }

        if (const auto* e_wheel = event->getIf<sf::Event::MouseWheelScrolled>()) 
        {
            if (!m_stage->is_loaded()) return;

            sf::Vector2i pixel = e_wheel->position;
            sf::Vector2f uiWorldPos = m_window.mapPixelToCoords(pixel, m_uiCamera);

            if (!m_uiManager->is_mouse_over_ui(static_cast<sf::Vector2i>(uiWorldPos))) 
            {
                float factor = (e_wheel->delta > 0.f) ? 0.9f : 1.1f;
                float newZoom = m_currentZoom * factor;

                if (newZoom < 0.9f) newZoom = 0.9f;
                if (newZoom > 1.5f) newZoom = 1.5f;

                float appliedFactor = newZoom / m_currentZoom;
                m_stageCamera.zoom(appliedFactor);
                m_currentZoom = newZoom;

                // clamp camera after zoom
                auto unitBase = m_stage->get_unit_base().lock();
                auto enemyBase = m_stage->get_enemy_base().lock();
                if (unitBase && enemyBase) 
                {
                    sf::Vector2f center = m_stageCamera.getCenter();
                    float viewWidth = m_stageCamera.getSize().x;
                    const float margin = 0.25f;
                    const float baseWidth = 200.f;

                    float minCenterX = enemyBase->position.x + baseWidth + (0.5f - margin) * viewWidth;
                    float maxCenterX = unitBase->position.x - (0.5f - margin) * viewWidth;

                    if (minCenterX > maxCenterX) 
                    {
                        float mid = (enemyBase->position.x + unitBase->position.x) * 0.5f;
                        minCenterX = maxCenterX = mid;
                    }

                    center.x = std::clamp(center.x, minCenterX, maxCenterX);
                    m_stageCamera.setCenter(center);
                }
            }
        }


        if (const auto* e_mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
        {
            if (e_mousePressed->button == sf::Mouse::Button::Left)
            {
                sf::Vector2i pixelPos = e_mousePressed->position;
                sf::Vector2f worldPos = m_window.mapPixelToCoords(pixelPos, m_uiCamera);

                if (!m_uiManager->is_mouse_over_ui(static_cast<sf::Vector2i>(worldPos)))
                {
                    m_isDragging = true;
                    m_lastMousePos = pixelPos;
                }
            }
        }

        if (const auto* e_mouseReleased = event->getIf<sf::Event::MouseButtonReleased>())
        {
            if (e_mouseReleased->button == sf::Mouse::Button::Left)
            {
                m_isDragging = false;
            }
        }

        if (const auto* e_mouseMoved = event->getIf<sf::Event::MouseMoved>())
        {
            if (m_isDragging && m_stage->is_loaded())
            {
                sf::Vector2i currentPixel = e_mouseMoved->position;

                sf::Vector2f prevWorld = m_window.mapPixelToCoords(m_lastMousePos, m_stageCamera);
                sf::Vector2f curWorld = m_window.mapPixelToCoords(currentPixel, m_stageCamera);
                sf::Vector2f worldDelta = prevWorld - curWorld;
                worldDelta.y = 0.f;

                auto unitBase = m_stage->get_unit_base().lock();
                auto enemyBase = m_stage->get_enemy_base().lock();


                if (unitBase && enemyBase) {
                    m_stageCamera.move(worldDelta);

                    sf::Vector2f center = m_stageCamera.getCenter();
                    float viewWidth = m_stageCamera.getSize().x;

                    const float margin = 0.25f;
                    const float baseWidth = 200.f;

                    float minCenterX = enemyBase->position.x + baseWidth + (0.5f - margin) * viewWidth;
                    float maxCenterX = unitBase->position.x - (0.5f - margin) * viewWidth;

                    if (minCenterX > maxCenterX) {
                        float mid = (enemyBase->position.x + unitBase->position.x) * 0.5f;
                        minCenterX = maxCenterX = mid;
                    }

                    center.x = std::clamp(center.x, minCenterX, maxCenterX);
                    m_stageCamera.setCenter(center);
                }

                m_lastMousePos = currentPixel;
            }
        }

        if (const auto* e_window = event->getIf<sf::Event::Resized>())
        {
            auto newSize = e_window->size; // taille réelle de la fenêtre

            // 1. Adapter la vue UI (toujours fixe)
            m_uiCamera.setSize(static_cast<sf::Vector2f>(m_logicalResolution));
            m_uiCamera.setCenter({ m_logicalResolution.x / 2.f, m_logicalResolution.y / 2.f });

            // 2. Adapter la vue gameplay (respecte le ratio)
            float windowRatio = static_cast<float>(newSize.x) / newSize.y;
            float logicalRatio = static_cast<float>(m_logicalResolution.x) / m_logicalResolution.y;

            if (windowRatio > logicalRatio)
            {
                // Fenêtre trop large => bandes verticales
                float width = m_logicalResolution.y * windowRatio;
                m_stageCamera.setSize({ width, static_cast<float>(m_logicalResolution.y) });
            }
            else
            {
                // Fenêtre trop haute => bandes horizontales
                float height = m_logicalResolution.x / windowRatio;
                m_stageCamera.setSize({ static_cast<float>(m_logicalResolution.x), height });
            }

            // On garde le centre actuel de la caméra gameplay (utile si l’utilisateur a bougé/zoomé)
            // Rien à faire si tu veux garder le centre tel quel

            // 3. Réappliquer la vue courante à la fenêtre
            m_window.setView(m_uiCamera); // par défaut sur UI
            center_window();
        }
    }

    if(m_isPaused)
        deltaTime = 0.f;
    else if (m_isFaster)
        deltaTime *= 2.f;
}

void Game::resize_window(sf::Vector2u newSize)
{
    m_window.setSize(newSize);
}

void Game::center_window()
{
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::Vector2u windowSize = m_window.getSize();

    int posX = (desktop.size.x - windowSize.x) / 2;
    int posY = (desktop.size.y - windowSize.y) / 2;

    m_window.setPosition(sf::Vector2i(posX, posY));
}

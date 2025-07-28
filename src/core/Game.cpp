#include "Game.h"

#include <iostream>

Game::Game():
    m_dataLoader(std::make_shared<DataLoader>()),
    m_stage(std::make_shared<Stage>())
{
    m_window.create(sf::VideoMode(m_logicalResolution), "Fish Fighers", sf::Style::None);
    m_window.setFramerateLimit(m_frameRate);
    center_window();

    ImGui::SFML::Init(m_window);

    m_dataLoader->load_all();
    m_stage->init(m_dataLoader);

    init_camera();
}

void Game::init_camera()
{
    m_camera.setSize(static_cast<sf::Vector2f>(m_logicalResolution));

    m_camera.setCenter(m_camera.getSize() / 2.f);

    m_camera.setViewport(sf::FloatRect{
    {0.f, 0.f},
    {1.f, 1.f}
        });

    m_window.setView(m_camera);
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

    m_stage->load(1);

    while (m_window.isOpen())
    {
        //Updating deltaTime
        deltaTime = m_deltaClock.restart().asSeconds();

        //Poll events
        poll_events();

        //Updating all systems
        m_stage->update(deltaTime);
        //sprite.setPosition(tween.step(1));

        //Rendering
        m_window.clear();
        //m_window.draw(shape);
        //m_window.draw(sprite);
        m_stage->render(m_window);
        debug_ui();
        m_window.display();
    }

    ImGui::SFML::Shutdown();
}

void Game::poll_events()
{
    while (const auto event = m_window.pollEvent()) {
        ImGui::SFML::ProcessEvent(m_window, *event);

        if (event->is<sf::Event::Closed>())
        {
            m_window.close();
        }

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
            if (e_keycode->code == sf::Keyboard::Key::A)
            {
                std::cout << "spawn fish 1\n";
                m_stage->spawn_unit(m_dataLoader->get_unit_data(1));
            }
            if (e_keycode->code == sf::Keyboard::Key::E)
            {
                std::cout << "spawn fish 2\n";
                m_stage->spawn_unit(m_dataLoader->get_unit_data(2));
            }
        }

        if (const auto* e_window = event->getIf<sf::Event::Resized>())
        {
            m_camera.setViewport(sf::FloatRect{
                    {0.f, 0.f},
                    {1.f, 1.f}
                });
            m_window.setView(m_camera);
            center_window();
        }
    }
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

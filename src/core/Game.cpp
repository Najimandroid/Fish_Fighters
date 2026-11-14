#include "Game.h"
#include "Platform.h"
#include "../uis/UISkin.h"

#include <algorithm>
#include <iostream>
#include <vector>

namespace
{
    const sf::Color LETTERBOX_COLOR{ 16, 24, 34 };   // The bars around the UI on screens that aren't 16:9 (UI navy)
}

/*
 * Constructor
 * Initializes:
 *  - SFML window (fullscreen)
 *  - Developer tools in their own window (DEBUG_MODE)
 *  - DataLoader, Stage, UIManager
 *  - Cameras
 */
Game::Game():
    m_dataLoader(std::make_shared<DataLoader>()),
    m_stage(std::make_shared<Stage>()),
	m_uiManager(std::make_shared<UIManager>()),
    m_audio(std::make_shared<AudioManager>())
{
    // The player's options first: they choose the window mode
    m_dataLoader->load_settings();
    create_window(m_dataLoader->get_settings().fullscreen);

    // Load all game data (units, enemies, stages, player)
    // If anything is missing or broken, the reason is in error_log.txt and the game closes
    // instead of running with incomplete data (the save is not touched in that case)
    if (!m_dataLoader->load_all())
    {
        m_window.close();
    }

    // Music and sounds (without them the game still runs, silent; the reason is in the console)
    m_audio->load();
    m_audio->apply_settings(m_dataLoader->get_settings());

    // Initialize UIManager and Stage with references to DataLoader
    m_uiManager->init(m_dataLoader, m_stage, m_audio);

	// Set up game speed and quit callbacks for UIManager
    m_uiManager->set_game_speed_callback([this](GameSpeedState s) {
        set_game_speed(s);
    });
    m_uiManager->set_get_game_speed_callback([this]() -> GameSpeedState {
        return m_speedState;
    });
    m_uiManager->set_quit_game_callback([this]() {m_window.close();});
    // Options > Display: applied between two frames (not while the window's events are being read)
    m_uiManager->set_fullscreen_callback([this](bool fullscreen) { m_requestedFullscreen = fullscreen; });

    m_stage->init(m_dataLoader, m_uiManager, m_audio, &m_stageCamera);

    // Set up cameras for UI and stage rendering
    init_cameras();

#ifdef DEBUG_MODE
    // Developer tools in a second window, so the game keeps its full screen
    DebugContext debugContext;
    debugContext.dataLoader = m_dataLoader;
    debugContext.stage = m_stage;
    debugContext.uiManager = m_uiManager;
    debugContext.getGameSpeed = [this]() { return m_speedState; };
    debugContext.setGameSpeed = [this](GameSpeedState speed) { set_game_speed(speed); };
    debugContext.requestStepFrame = [this]() { m_isStepFrameRequested = true; };
    m_debugTools = std::make_unique<DebugTools>(debugContext);
#endif
}

/*
 * Initialize cameras
 * - m_uiCamera: fixed screen-space UI
 * - m_stageCamera: world-space stage view (zoomable and pannable)
 */
void Game::init_cameras()
{
    m_uiCamera.setSize(static_cast<sf::Vector2f>(m_logicalResolution));
    m_uiCamera.setCenter(m_uiCamera.getSize() / 2.f);

    m_stageCamera.setSize(static_cast<sf::Vector2f>(m_logicalResolution));
    m_stageCamera.setCenter(m_stageCamera.getSize() / 2.f);

    m_stageCamera.setViewport(sf::FloatRect{ {0.f, 0.f}, {1.f, 1.f} });
    m_stageCamera.zoom(0.9f); // initial zoom-out for better stage visibility

    update_viewports();
}

/*
 * Fits the views to the window's shape:
 *  - the UI keeps its 1920x1080 proportions, scaled uniformly and centered (bars on the sides on wider
 *    screens, above and below on taller ones), never stretched
 *  - the battlefield fills the whole window: a wider screen shows more of the lane, at the same zoom
 */
void Game::update_viewports()
{
    const sf::Vector2f window = static_cast<sf::Vector2f>(m_window.getSize());
    if (window.x <= 0.f || window.y <= 0.f) return;

    // The UI fills the window: its 1920 x 1080 layout in the middle, a wider screen (a phone) showing more on the sides,
    // a taller one (a tablet) more above and below; backgrounds and bars fill that (UISkin::screen_rect)
    const sf::Vector2f logical = static_cast<sf::Vector2f>(m_logicalResolution);
    sf::Vector2f shown = logical;
    if (window.x / window.y > logical.x / logical.y) shown.x = logical.y * window.x / window.y;
    else shown.y = logical.x * window.y / window.x;
    m_uiCamera.setSize(shown);
    m_uiCamera.setCenter(logical / 2.f);
    m_uiCamera.setViewport(sf::FloatRect{ { 0.f, 0.f }, { 1.f, 1.f } });
    UISkin::set_screen_rect({ logical / 2.f - shown / 2.f, shown });

    // The stage camera's height carries its zoom; its width follows the window
    const float height = m_stageCamera.getSize().y;
    m_stageCamera.setSize({ height * window.x / window.y, height });
}

void Game::calculate_delta_time()
{
    // A frame never counts more than MAX_FRAME_SECONDS: after a stall (loading a battle, the phone coming back from the
    // background, a dragged window) the game carries on where it was instead of jumping ahead
    const float MAX_FRAME_SECONDS = 0.1f;
    const float frameSeconds = m_deltaClock.restart().asSeconds();
    if (frameSeconds > 0.25f) Platform::log("Slow frame: " + std::to_string(static_cast<int>(frameSeconds * 1000.f)) + " ms");
    m_deltaTime = std::min(frameSeconds, MAX_FRAME_SECONDS);
    m_realDeltaTime = m_deltaTime;

    switch (m_speedState)
    {
    case GameSpeedState::PAUSED:
        // Debug frame step: exactly one frame at normal speed
        m_deltaTime = m_isStepFrameRequested ? 1.f / static_cast<float>(m_frameRate) : 0.f;
        m_isStepFrameRequested = false;
		break;

    case GameSpeedState::NORMAL:
        // No modification
		break;

    case GameSpeedState::FASTER:
		m_deltaTime *= 2.f;
        break;

    default:
		// Should not reach here
		break;
    }
}

void Game::run_game_loop()
{
    // The game opens on the title screen
	m_uiManager->generate_title_uis();

    while (m_window.isOpen())
    {
        calculate_delta_time();

        poll_events();

        if (m_requestedFullscreen)
        {
            create_window(*m_requestedFullscreen);
            m_requestedFullscreen.reset();
        }

        m_stage->update(m_deltaTime);
        m_uiManager->update_uis(m_deltaTime);

        // The music plays quieter while a battle is paused; real time, so it keeps fading during the pause
        m_audio->set_music_ducked(m_stage->is_loaded() && m_speedState == GameSpeedState::PAUSED);
        m_audio->update(m_realDeltaTime);

        m_window.clear(LETTERBOX_COLOR);

        // Render stage (world)
        m_window.setView(m_stageCamera);
        m_stage->render(m_window);

#ifdef DEBUG_MODE
        // Hitboxes, ranges, labels and selection, drawn on the battlefield under the UI
        m_debugTools->draw_world_overlays(m_window);
#endif

        // Render UI (screen-space)
        m_window.setView(m_uiCamera);
        m_uiManager->render_uis(m_window, m_uiCamera, m_stageCamera);

        m_window.display();

#ifdef DEBUG_MODE
        m_debugTools->update(m_realDeltaTime);
#endif
    }

#ifdef DEBUG_MODE
    // Closes the tools window together with the game
    m_debugTools.reset();
#endif
}

/*
 * Save persistent data on game exit
 */
void Game::terminate()
{
    m_dataLoader->terminate();
}


/*
 * Process all pending events from the window
 */
std::vector<sf::Event> Game::touch_as_mouse(const sf::Event& event)
{
    const auto left = sf::Mouse::Button::Left;
    if (const auto* down = event.getIf<sf::Event::TouchBegan>())
    {
        if (down->finger != 0) return {};
        return { sf::Event::MouseMoved{ down->position }, sf::Event::MouseButtonPressed{ left, down->position } };
    }
    if (const auto* moved = event.getIf<sf::Event::TouchMoved>())
    {
        if (moved->finger != 0) return {};
        return { sf::Event::MouseMoved{ moved->position } };
    }
    if (const auto* up = event.getIf<sf::Event::TouchEnded>())
    {
        if (up->finger != 0) return {};
        return { sf::Event::MouseButtonReleased{ left, up->position }, sf::Event::MouseMoved{ { -100000, -100000 } } };
    }
    return { event };
}

void Game::poll_events()
{
    while (const auto polled = m_window.pollEvent())
    for (const sf::Event& input : touch_as_mouse(*polled)) {
        const sf::Event* event = &input;

        // Window close event
        if (event->is<sf::Event::Closed>())
        {
            m_window.close();
        }

        // Forward event to UIManager for UI interaction
        m_uiManager->handle_ui_events(*event, m_window);

#ifdef DEBUG_MODE
        // Developer shortcuts (the player's pause and speed keys are on the battle UI: P / Space / Escape, F)
        if (const auto* e_keycode = event->getIf<sf::Event::KeyPressed>())
        {
            // Window size shortcuts
            if (e_keycode->code == sf::Keyboard::Key::Numpad1) m_window.setSize({ 1920, 1080 });
            else if (e_keycode->code == sf::Keyboard::Key::Numpad2) m_window.setSize({ 1280, 720 });
            else if (e_keycode->code == sf::Keyboard::Key::Numpad3) m_window.setSize({ 640, 360 });
            // Show or hide the developer tools window
            else if (e_keycode->code == sf::Keyboard::Key::F1) m_debugTools->toggle_window();
            // Pause and speed without the pause menu
            else if (e_keycode->code == sf::Keyboard::Key::F2) { m_speedState = GameSpeedState::PAUSED; }
            else if (e_keycode->code == sf::Keyboard::Key::F3) { m_speedState = GameSpeedState::NORMAL; }
            else if (e_keycode->code == sf::Keyboard::Key::F4) { m_speedState = GameSpeedState::FASTER; }
        }
#endif

        // Mouse wheel event for camera zoom
        if (const auto* e_wheel = event->getIf<sf::Event::MouseWheelScrolled>())
        {
            if (!m_stage->is_loaded()) return;

            // Map mouse position to UI world coordinates
            sf::Vector2i pixel = e_wheel->position;
            sf::Vector2f uiWorldPos = m_window.mapPixelToCoords(pixel, m_uiCamera);

            // Only zoom if not over UI
            if (!m_uiManager->is_mouse_over_ui(static_cast<sf::Vector2i>(uiWorldPos)))
            {
                // Calculate new zoom factor
                m_stage->apply_zoom((e_wheel->delta > 0.f) ? 0.9f : 1.1f);

                // Clamp camera to keep stage within view
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

                    // Adjust center if limits overlap
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

        // Mouse button pressed event (start drag)
        if (const auto* e_mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
        {
            if (e_mousePressed->button == sf::Mouse::Button::Left)
            {
                sf::Vector2i pixelPos = e_mousePressed->position;
                sf::Vector2f worldPos = m_window.mapPixelToCoords(pixelPos, m_uiCamera);

                // Start dragging if mouse not over UI
                if (!m_uiManager->is_mouse_over_ui(static_cast<sf::Vector2i>(worldPos)))
                {
                    m_isDragging = true;
                    m_lastMousePos = pixelPos;
                }
            }
        }

        // Mouse button released event (stop drag)
        if (const auto* e_mouseReleased = event->getIf<sf::Event::MouseButtonReleased>())
        {
            if (e_mouseReleased->button == sf::Mouse::Button::Left)
            {
                m_isDragging = false;
            }
        }

        // Mouse movement event (camera panning)
        if (const auto* e_mouseMoved = event->getIf<sf::Event::MouseMoved>())
        {
            if (m_isDragging && m_stage->is_loaded())
            {
                sf::Vector2i currentPixel = e_mouseMoved->position;

                // Calculate delta movement in world space
                sf::Vector2f prevWorld = m_window.mapPixelToCoords(m_lastMousePos, m_stageCamera);
                sf::Vector2f curWorld = m_window.mapPixelToCoords(currentPixel, m_stageCamera);
                sf::Vector2f worldDelta = prevWorld - curWorld;
                worldDelta.y = 0.f; // restrict vertical movement

                auto unitBase = m_stage->get_unit_base().lock();
                auto enemyBase = m_stage->get_enemy_base().lock();

                if (unitBase && enemyBase)
                {
                    // Move camera by delta
                    m_stageCamera.move(worldDelta);

                    // Clamp camera within stage boundaries
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

        // Window resize event
        if (event->is<sf::Event::Resized>())
        {
            update_viewports();
            m_window.setView(m_uiCamera);
        }

        // Focus: the game goes quiet in the background (Options > Audio)
        if (event->is<sf::Event::FocusLost>()) m_audio->set_focus_muted(true);
        if (event->is<sf::Event::FocusGained>()) m_audio->set_focus_muted(false);
    }
}


/*
 * Creates (or re-creates) the window:
 *  - fullscreen: borderless at the desktop size
 *  - windowed: a resizable 16:9 window, 80% of the desktop, with a title bar
 * The views are fitted to it right away (the UI keeps its 16:9 shape in any window)
 */
void Game::create_window(bool fullscreen)
{
    const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    if constexpr (Platform::IS_ANDROID)
    {
        // The phone's whole screen, always
        m_window.create(desktop, "Fish Fighters", sf::State::Fullscreen);
        m_window.setFramerateLimit(m_frameRate);
        update_viewports();
        return;
    }
    if (fullscreen)
    {
        m_window.create(desktop, "Fish Fighters", sf::Style::None);
    }
    else
    {
        sf::Vector2f size{ desktop.size.x * 0.8f, desktop.size.x * 0.8f * 9.f / 16.f };
        if (size.y > desktop.size.y * 0.8f) size = { desktop.size.y * 0.8f * 16.f / 9.f, desktop.size.y * 0.8f };
        m_window.create(sf::VideoMode(static_cast<sf::Vector2u>(size)), "Fish Fighters", sf::Style::Default);
    }
    m_window.setFramerateLimit(m_frameRate);

    // Window icon (the exe's own icon comes from resources/game.rc)
    sf::Image icon;
    if (icon.loadFromFile("assets/images/app/game_icon.png"))
        m_window.setIcon(icon);
    center_window();
    update_viewports();
}

/*
 * Change window size programmatically
 */
void Game::resize_window(sf::Vector2u newSize)
{
    m_window.setSize(newSize);
}

/*
 * Center window on desktop
 */
void Game::center_window()
{
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::Vector2u windowSize = m_window.getSize();

    int posX = (desktop.size.x - windowSize.x) / 2;
    int posY = (desktop.size.y - windowSize.y) / 2;

    m_window.setPosition(sf::Vector2i(posX, posY));
}

void Game::set_game_speed(GameSpeedState speedState)
{
	m_speedState = speedState;
}

GameSpeedState Game::get_game_speed() const
{
    return m_speedState;
}

float Game::get_delta_time() const
{
    return m_deltaTime;
}

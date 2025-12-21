#include "Game.h"

#include <iostream>

/*
 * Constructor
 * Initializes:
 *  - SFML window (fullscreen)
 *  - ImGui for debug
 *  - DataLoader, Stage, UIManager
 *  - Cameras
 */
Game::Game():
    m_dataLoader(std::make_shared<DataLoader>()),
    m_stage(std::make_shared<Stage>()),
	m_uiManager(std::make_shared<UIManager>())
{
    // Create fullscreen window without borders
    m_window.create(sf::VideoMode::getDesktopMode(), "Fish Fighers", sf::Style::None); //sf::Style::None
    m_window.setFramerateLimit(m_frameRate);
    center_window();

    // Initialize ImGui for debug UI
#ifdef DEBUG_MODE
    ImGui::SFML::Init(m_window);

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF("assets/fonts/MPLUSRounded1c-Medium.ttf", 24.0f);
    ImGui::SFML::UpdateFontTexture();
#endif
    // Load all game data (units, enemies, stages, player)
    m_dataLoader->load_all();

    // Initialize UIManager and Stage with references to DataLoader
    m_uiManager->init(m_dataLoader, m_stage);

	// Set up game speed callbacks for UIManager
    m_uiManager->set_game_speed_callback([this](GameSpeedState s) {
        set_game_speed(s);
    });
    m_uiManager->set_get_game_speed_callback([this]() -> GameSpeedState {
        return m_speedState;
    });

    m_stage->init(m_dataLoader, m_uiManager, &m_stageCamera);

    // Set up cameras for UI and stage rendering
    init_cameras();
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

#ifdef DEBUG_MODE
    m_uiCamera.setViewport(sf::FloatRect{ {0.25f, 0.25f}, {0.5f, 0.5f} });
    m_stageCamera.setViewport(sf::FloatRect{ {0.25f, 0.25f}, {0.5f, 0.5f} });
#else
    m_uiCamera.setViewport(sf::FloatRect{ {0.f, 0.f}, {1.f, 1.f} });
    m_stageCamera.setViewport(sf::FloatRect{ {0.f, 0.f}, {1.f, 1.f} });
#endif

    m_stageCamera.zoom(0.9f); // initial zoom-out for better stage visibility
}

/*
 * Renders debug UI using ImGui
 * Only active when DEBUG_MODE is defined
 */
void Game::debug_ui()
{
#ifdef DEBUG_MODE
    ImGui::SFML::Update(m_window, m_deltaClockDebug.restart());

	bool isStageLoaded = m_stage->is_loaded();

    /*
    * ======================================================================================================
    * DEBUG TIME AND PERFORMANCE PANEL
    * ---------------------------------------
    * FPS, delta time, pause game
    */
	const ImVec2 timeWindowSize = isStageLoaded ? ImVec2(500.f, 200.f) : ImVec2(500.f, 100.f);
    ImGui::SetNextWindowSize(timeWindowSize, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(1920.f / 2.f - timeWindowSize.x / 2.f, 0.f), ImGuiCond_Always);
    ImGui::Begin("Time & Performance");

    // FPS
    const float fps = (m_deltaTime > 0.f) ? (1.f / m_deltaTime) : 0.f;
    ImGui::Text("FPS: %.1f", fps);
	// Delta time
	ImGui::Text("Delta Time: %.4f s", m_debugDeltaTime);

    // Stage specific parameters
    if (isStageLoaded)
    {
        ImGui::Separator();

        // Pause button
        static bool isPaused = false;
        if (ImGui::Checkbox("Pause", &isPaused))
            m_speedState = isPaused ? GameSpeedState::PAUSED : GameSpeedState::NORMAL; // Toggle pause

        // Frame step button
        if (isPaused)
        {
            if (ImGui::Button("Step One Frame"))
            {
                m_deltaTime = 1.f / static_cast<float>(m_frameRate); // Advance one frame
            }
        }
    }

    ImGui::End();

    /*
	* =======================================================================================================
	* Game Parameters / Stage Control Panel
    * -----------------------------------------------------------------
	* Control stage loading, spawn units/enemies, modify player shells
    */

    const ImVec2 gameParametersWindowSize = ImVec2(480.f, 1080.f);
    ImGui::SetNextWindowSize(gameParametersWindowSize, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(1920.f - gameParametersWindowSize.x, 0.f), ImGuiCond_Always);

    ImGui::Begin("Game Data Tools");

    if (!isStageLoaded)
    {
        ImGui::Text("Main Menu Debug Tools");

        static int levelToLoad = 0;
        ImGui::InputInt("Level ID", &levelToLoad);

        if (ImGui::Button("Load Level"))
        {
            m_stage->load(levelToLoad);
            m_uiManager->generate_battle_uis();
        }

        ImGui::Separator();

        static int shellAmount = 0;
        ImGui::InputInt("Add Shells To Player", &shellAmount);

        if (ImGui::Button("Add Shells"))
        {
            auto playerData = m_dataLoader->get_player_data().lock();
            if (playerData) playerData->gain_shells(shellAmount);
        }
    }
    else
    {
        ImGui::Text("Spawn Units");

        /*
        * =======================================================================================================
        * Spawn Unit
        * -------------------------------------------------------------------------------------------------------
        * Allows spawning any unit by ID and form (0 or 1).
        */
        static int unitId = 1;
        ImGui::InputInt("Unit ID", &unitId);

        static int unitForm = 0;
        const char* formOptions[] = { "Base", "Evolved" };
        ImGui::Combo("Unit Form", &unitForm, formOptions, IM_ARRAYSIZE(formOptions));

        auto unitData = m_dataLoader->get_unit_data(unitId, unitForm);

        // Display unit preview
       if (unitData)
        {
            static sf::Texture previewTexture;
			static sf::Sprite sprite(previewTexture);
			static int previousUnitId = -1;
			static int previousUnitForm = 0;

            if(previousUnitId != unitId || previousUnitForm != unitForm)
            {
                previewTexture.loadFromFile(unitData->texture);

                int frameWidth = previewTexture.getSize().x / unitData->frameCount;
                int frameHeight = previewTexture.getSize().y;

				sprite.setTexture(previewTexture);
				sprite.setTextureRect(sf::IntRect({ 0, 0 }, { frameWidth, frameHeight }));

                ImVec2 size(frameWidth, frameHeight);
            
                float maxPreviewSize = 200.f;
                float scale = std::min(maxPreviewSize / size.x, maxPreviewSize / size.y);

                size.x *= scale;
                size.y *= scale;

                sprite.setScale({ scale, scale });
			}

            ImGui::Image(sprite);
			previousUnitId = unitId;
			previousUnitForm = unitForm;
        }

        if (ImGui::Button("Spawn Unit") && unitData)
        {
            m_stage->spawn_unit(unitData);
        }

        ImGui::Separator();
        ImGui::Text("Spawn Enemies");

        /*
        * =======================================================================================================
        * Spawn Enemy
        * -------------------------------------------------------------------------------------------------------
        * Controls all spawn_enemy parameters:
        * - magnification.x = HP multiplier
        * - magnification.y = Attack multiplier
        * - layer: -1 to 50 (-1 means random)
        * - isBoss
        * - generateShockwave
        * - bypassEnemyLimit
        */

        static int enemyId = 1;
        ImGui::InputInt("Enemy ID", &enemyId);

        // Magnification
        static float hpMultiplier = 1.0f;
        static float attackMultiplier = 1.0f;

        ImGui::SliderFloat("HP Multiplier", &hpMultiplier, 1.f, 100.0f);
        ImGui::SliderFloat("Attack Multiplier", &attackMultiplier, 1.f, 100.0f);

        // Layer
        static int enemyLayer = -1;
        ImGui::SliderInt("Layer", &enemyLayer, -1, 50);

        // Flags
        static bool isBoss = false;
        ImGui::Checkbox("Is Boss", &isBoss);

        static bool bypassEnemyLimit = false;
        ImGui::Checkbox("Bypass Enemy Limit", &bypassEnemyLimit);

        auto data = m_dataLoader->get_enemy_data(enemyId);

		// Display enemy preview
        if (data)
        {
            static sf::Texture previewTexture;
            static sf::Sprite sprite(previewTexture);
            static int previousEnemyId = -1;

            if (previousEnemyId != enemyId)
            {
                previewTexture.loadFromFile(data->texture);

                int frameWidth = previewTexture.getSize().x / data->frameCount;
                int frameHeight = previewTexture.getSize().y;

                sprite.setTexture(previewTexture);
                sprite.setTextureRect(sf::IntRect({ 0, 0 }, { frameWidth, frameHeight }));

                ImVec2 size(frameWidth, frameHeight);

                float maxPreviewSize = 200.f;
                float scale = std::min(maxPreviewSize / size.x, maxPreviewSize / size.y);

                size.x *= scale;
                size.y *= scale;

                sprite.setScale({ scale, scale });
            }

            ImGui::Image(sprite);
            previousEnemyId = enemyId;
        }

        if (ImGui::Button("Spawn Enemy") && data)
        {
            // Build magnification vector
            sf::Vector2f magnification(hpMultiplier, attackMultiplier);

            // Spawn enemy with full parameter set
            m_stage->spawn_enemy(
                data,
                magnification,
                enemyLayer,
                isBoss,
                bypassEnemyLimit
            );
        }

        ImGui::Separator();
        ImGui::Text("Stage Parameters");

        /*
        * =======================================================================================================
        * Exit current stage
        */
        if (ImGui::Button("Exit Stage"))
        {
            m_stage->unload();
            m_uiManager->generate_fish_tank_uis();
        }
    }

    ImGui::End();

    //======================================================================================================

    ImGui::SFML::Render(m_window);
#endif
}

void Game::calculate_delta_time()
{
    m_deltaTime = m_deltaClock.restart().asSeconds();

#ifdef DEBUG_MODE
    m_debugDeltaTime = m_deltaClockDebug.restart().asSeconds();
#endif

    switch (m_speedState)
    {
    case GameSpeedState::PAUSED:
        m_deltaTime = 0.f;
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

#ifdef DEBUG_MODE
ImTextureID Game::get_imgui_texture(const sf::Texture& texture)
{
    const sf::Texture* key = &texture;
    auto it = m_imguiTextureCache.find(key);
    if (it != m_imguiTextureCache.end())
        return it->second;

	// TODO: Texture SFML => IMGUI conversion
}
#endif

void Game::run_game_loop()
{
    // Initialize UI to Fish Tank screen
	m_uiManager->generate_fish_tank_uis(); // TODO: add a main menu before this

    while (m_window.isOpen())
    {
        calculate_delta_time();

        poll_events();

        m_stage->update(m_deltaTime);
        m_uiManager->update_uis(m_deltaTime);

        m_window.clear();

        // Render stage (world)
        m_window.setView(m_stageCamera);
        m_stage->render(m_window);

        // Render UI (screen-space)
        m_window.setView(m_uiCamera);
        m_uiManager->render_uis(m_window, m_uiCamera, m_stageCamera);

#ifdef DEBUG_MODE
        debug_ui();
#endif
        m_window.display();
    }

#ifdef DEBUG_MODE
    ImGui::SFML::Shutdown();
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
void Game::poll_events()
{
    while (const auto event = m_window.pollEvent()) {

#ifdef DEBUG_MODE
        ImGui::SFML::ProcessEvent(m_window, *event);
#endif 

        // Window close event
        if (event->is<sf::Event::Closed>())
        {
            m_window.close();
        }

        // Forward event to UIManager for UI interaction
        m_uiManager->handle_ui_events(*event, m_window);

        // Keyboard input handling
        if (const auto* e_keycode = event->getIf<sf::Event::KeyPressed>())
        {
            // Window size shortcuts
            if (e_keycode->code == sf::Keyboard::Key::Numpad1) m_window.setSize({ 1920, 1080 });
            else if (e_keycode->code == sf::Keyboard::Key::Numpad2) m_window.setSize({ 1280, 720 });
            else if (e_keycode->code == sf::Keyboard::Key::Numpad3) m_window.setSize({ 640, 360 });
            // Close game
            else if (e_keycode->code == sf::Keyboard::Key::Escape) m_window.close();
            // Pause and speed control
            else if (e_keycode->code == sf::Keyboard::Key::P) { m_speedState = GameSpeedState::PAUSED; }
            else if (e_keycode->code == sf::Keyboard::Key::LShift) { m_speedState = GameSpeedState::FASTER; }
            else if (e_keycode->code == sf::Keyboard::Key::R) { m_speedState = GameSpeedState::NORMAL; }
        }

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
        if (const auto* e_window = event->getIf<sf::Event::Resized>())
        {
            auto newSize = e_window->size; // actual new window size

            // 1. Adjust UI camera (always fixed)
            m_uiCamera.setSize(static_cast<sf::Vector2f>(m_logicalResolution));
            m_uiCamera.setCenter({ m_logicalResolution.x / 2.f, m_logicalResolution.y / 2.f });

            // 2. Adjust stage camera while maintaining aspect ratio
            float windowRatio = static_cast<float>(newSize.x) / newSize.y;
            float logicalRatio = static_cast<float>(m_logicalResolution.x) / m_logicalResolution.y;

            if (windowRatio > logicalRatio) // window too wide => vertical bars
            {
                float width = m_logicalResolution.y * windowRatio;
                m_stageCamera.setSize({ width, static_cast<float>(m_logicalResolution.y) });
            }
            else // window too tall => horizontal bars
            {
                float height = m_logicalResolution.x / windowRatio;
                m_stageCamera.setSize({ static_cast<float>(m_logicalResolution.x), height });
            }

            // 3. Reapply UI view and center window
            m_window.setView(m_uiCamera);
            center_window();
        }
    }
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

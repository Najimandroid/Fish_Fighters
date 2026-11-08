#include "Game.h"

#include <algorithm>
#include <climits>
#include <iostream>
#include <string>

#ifdef DEBUG_MODE
namespace
{
    /*
     * Debug editor for a list of entity types: one checkbox per type.
     */
    void edit_type_list(std::vector<EntityType>& types)
    {
        for (int i = 0; i < static_cast<int>(EntityType::COUNT); ++i)
        {
            EntityType type = static_cast<EntityType>(i);
            bool hasType = std::find(types.begin(), types.end(), type) != types.end();

            ImGui::PushID(i);
            if (ImGui::Checkbox(to_string(type), &hasType))
            {
                if (hasType) types.push_back(type);
                else types.erase(std::remove(types.begin(), types.end(), type), types.end());
            }
            ImGui::PopID();

            if (i + 1 < static_cast<int>(EntityType::COUNT)) ImGui::SameLine();
        }
    }

    /*
     * Debug editor for an entity's types and abilities, used before spawning.
     * The lists are copies: the data loaded from the JSON is never changed.
     */
    void edit_types_and_abilities(std::vector<EntityType>& types, std::vector<AbilityData>& abilities)
    {
        ImGui::SeparatorText("Types");
        ImGui::PushID("Types");
        edit_type_list(types);
        ImGui::PopID();

        ImGui::SeparatorText("Abilities");
        int abilityToRemove = -1;

        for (int a = 0; a < static_cast<int>(abilities.size()); ++a)
        {
            AbilityData& ability = abilities[a];
            ImGui::PushID(a);

            if (ImGui::BeginCombo("Ability", to_string(ability.type)))
            {
                for (int t = static_cast<int>(AbilityType::NONE) + 1; t < static_cast<int>(AbilityType::COUNT); ++t)
                {
                    AbilityType abilityType = static_cast<AbilityType>(t);
                    if (ImGui::Selectable(to_string(abilityType), abilityType == ability.type))
                    {
                        ability.type = abilityType;

                        // Sensible defaults so a new on-hit ability works right away
                        if (is_on_hit(abilityType) && ability.duration <= 0.f)
                        {
                            ability.value = 0.5f;
                            ability.chance = 100.f;
                            ability.duration = 3.f;
                        }
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::TextUnformatted("Targets:");
            ImGui::SameLine();
            ImGui::PushID("Targets");
            edit_type_list(ability.targetTypes);
            ImGui::PopID();

            if (is_on_hit(ability.type))
            {
                if (ability.type != AbilityType::PARALYZE_ATTACK)
                    ImGui::SliderFloat("Value", &ability.value, 0.05f, 2.f, "x%.2f");
                ImGui::SliderFloat("Chance", &ability.chance, 0.f, 100.f, "%.0f%%");
                ImGui::SliderFloat("Duration", &ability.duration, 0.1f, 30.f, "%.1f s");
            }

            if (ImGui::SmallButton("Remove ability")) abilityToRemove = a;
            ImGui::Separator();

            ImGui::PopID();
        }

        if (abilityToRemove >= 0) abilities.erase(abilities.begin() + abilityToRemove);

        if (ImGui::Button("Add ability"))
            abilities.push_back(AbilityData{ AbilityType::STRONG_AGAINST, {} });
    }

    /*
     * Shows the first frame of a sprite sheet, scaled to fit 200x200.
     * Each preview keeps its own texture, reloaded only when the path changes.
     */
    void show_debug_preview(const std::string& id, const std::string& texturePath, int frameCount)
    {
        struct Preview
        {
            sf::Texture texture;
            sf::Sprite sprite{ texture };
            std::string loadedPath;
        };
        static std::map<std::string, std::unique_ptr<Preview>> previews;

        auto& preview = previews[id];
        if (!preview) preview = std::make_unique<Preview>();

        if (preview->loadedPath != texturePath)
        {
            if (!preview->texture.loadFromFile(texturePath)) return;
            preview->loadedPath = texturePath;

            int frameWidth = preview->texture.getSize().x / std::max(1, frameCount);
            int frameHeight = preview->texture.getSize().y;

            preview->sprite.setTexture(preview->texture);
            preview->sprite.setTextureRect(sf::IntRect({ 0, 0 }, { frameWidth, frameHeight }));

            float maxPreviewSize = 200.f;
            float scale = std::min(maxPreviewSize / frameWidth, maxPreviewSize / frameHeight);
            preview->sprite.setScale({ scale, scale });
        }

        ImGui::Image(preview->sprite);
    }
}
#endif

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
    m_window.create(sf::VideoMode::getDesktopMode(), "Fish Fighters", sf::Style::None); //sf::Style::None
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
    // If anything is missing or broken, the reason is in error_log.txt and the game closes
    // instead of running with incomplete data (the save is not touched in that case)
    if (!m_dataLoader->load_all())
    {
        m_window.close();
    }

    // Initialize UIManager and Stage with references to DataLoader
    m_uiManager->init(m_dataLoader, m_stage);

	// Set up game speed and quit callbacks for UIManager
    m_uiManager->set_game_speed_callback([this](GameSpeedState s) {
        set_game_speed(s);
    });
    m_uiManager->set_get_game_speed_callback([this]() -> GameSpeedState {
        return m_speedState;
    });
    m_uiManager->set_quit_game_callback([this]() {m_window.close();});

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

        // Only defined stages can be picked (the DEBUG stage is UID -1)
        std::vector<std::shared_ptr<StageData>> stages;
        for (const auto& [uid, data] : m_dataLoader->get_stages_database()) stages.push_back(data);
        std::sort(stages.begin(), stages.end(), [](const auto& a, const auto& b) { return a->UID < b->UID; });

        static int levelToLoad = -1;
        auto stageLabel = [](const StageData& data) { return std::to_string(data.UID) + " - " + data.name; };

        std::shared_ptr<StageData> selectedStage;
        for (const auto& data : stages) if (data->UID == levelToLoad) selectedStage = data;
        if (!selectedStage && !stages.empty()) { selectedStage = stages.front(); levelToLoad = selectedStage->UID; }

        if (selectedStage)
        {
            if (ImGui::BeginCombo("Stage", stageLabel(*selectedStage).c_str()))
            {
                for (const auto& data : stages)
                {
                    if (ImGui::Selectable(stageLabel(*data).c_str(), data->UID == levelToLoad))
                        levelToLoad = data->UID;
                }
                ImGui::EndCombo();
            }

            if (ImGui::Button("Load Stage"))
            {
                m_stage->load(levelToLoad);
                m_uiManager->generate_battle_uis();
            }
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
        /*
        * =======================================================================================================
        * Exit current stage (no rewards)
        */
        if (ImGui::Button("Exit Stage", ImVec2(-FLT_MIN, 0.f)))
        {
            m_speedState = GameSpeedState::NORMAL;
            m_stage->unload();
            m_uiManager->generate_fish_tank_uis();
        }

        /*
        * =======================================================================================================
        * Spawn Unit
        * -------------------------------------------------------------------------------------------------------
        * Pick any unit defined in units.json and one of its forms, edit its types and abilities, then spawn it.
        * Edits are made on a copy: the data loaded from the JSON is never changed.
        */
        if (ImGui::CollapsingHeader("Spawn Unit", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const auto& units = m_dataLoader->get_units_database();

            static std::pair<int, int> selectedUnit = { -1, -1 }; // {UID, form}
            static std::pair<int, int> editedUnit = { -1, -1 };   // Unit the editor below was filled from
            static std::vector<EntityType> unitTypes;
            static std::vector<AbilityData> unitAbilities;
            static bool isFreeUnitSpawn = true;

            if (units.find(selectedUnit) == units.end() && !units.empty())
                selectedUnit = units.begin()->first;

            auto selected = units.find(selectedUnit);
            if (selected != units.end())
            {
                // One entry per unit (named after its base form). Only defined units can be picked
                auto unitLabel = [&units](int uid)
                    {
                        auto baseForm = units.lower_bound({ uid, INT_MIN });
                        return std::to_string(uid) + " - " + baseForm->second->name;
                    };

                if (ImGui::BeginCombo("Unit", unitLabel(selectedUnit.first).c_str()))
                {
                    int previousUid = INT_MIN;
                    for (const auto& [key, data] : units)
                    {
                        if (key.first == previousUid) continue; // Other forms of the same unit
                        previousUid = key.first;

                        if (ImGui::Selectable(unitLabel(key.first).c_str(), key.first == selectedUnit.first))
                            selectedUnit = units.lower_bound({ key.first, INT_MIN })->first; // Base form
                    }
                    ImGui::EndCombo();
                }

                // Only the forms this unit has can be picked
                auto formLabel = [](const std::pair<int, int>& key, const EntityData& data)
                    {
                        return (key.second == 0 ? std::string("Base") : "Evolved " + std::to_string(key.second)) + " - " + data.name;
                    };

                selected = units.find(selectedUnit);
                if (ImGui::BeginCombo("Form", formLabel(selected->first, *selected->second).c_str()))
                {
                    for (auto it = units.lower_bound({ selectedUnit.first, INT_MIN }); it != units.end() && it->first.first == selectedUnit.first; ++it)
                    {
                        if (ImGui::Selectable(formLabel(it->first, *it->second).c_str(), it->first == selectedUnit))
                            selectedUnit = it->first;
                    }
                    ImGui::EndCombo();
                }

                selected = units.find(selectedUnit);
                const std::shared_ptr<EntityData>& unitData = selected->second;

                // New unit picked: start from its JSON types and abilities
                if (editedUnit != selectedUnit)
                {
                    unitTypes = unitData->types;
                    unitAbilities = unitData->abilities;
                    editedUnit = selectedUnit;
                }

                show_debug_preview("UnitPreview", unitData->texture, unitData->frameCount);

                ImGui::PushID("Unit");
                edit_types_and_abilities(unitTypes, unitAbilities);
                if (ImGui::Button("Reset to JSON data"))
                {
                    unitTypes = unitData->types;
                    unitAbilities = unitData->abilities;
                }
                ImGui::PopID();

                ImGui::Checkbox("Free spawn (ignore cost and units limit)", &isFreeUnitSpawn);

                // "##Unit" keeps the button's ID different from the "Spawn Unit" header above
                if (ImGui::Button("Spawn##Unit", ImVec2(-FLT_MIN, 0.f)))
                {
                    auto customData = std::make_shared<EntityData>(*unitData);
                    customData->types = unitTypes;
                    customData->abilities = unitAbilities;
                    m_stage->spawn_unit(customData, isFreeUnitSpawn);
                }
            }
        }

        /*
        * =======================================================================================================
        * Spawn Enemy
        * -------------------------------------------------------------------------------------------------------
        * Pick any enemy defined in enemies.json, edit its types and abilities, then spawn it with all
        * spawn_enemy parameters:
        * - magnification.x = HP multiplier
        * - magnification.y = Attack multiplier
        * - layer: -1 to 50 (-1 means random)
        * - isBoss (shockwave)
        * - bypassEnemyLimit
        */
        if (ImGui::CollapsingHeader("Spawn Enemy", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // Sorted by UID so the list doesn't change order
            std::vector<std::shared_ptr<EntityData>> enemies;
            for (const auto& [uid, data] : m_dataLoader->get_enemies_database()) enemies.push_back(data);
            std::sort(enemies.begin(), enemies.end(), [](const auto& a, const auto& b) { return a->UID < b->UID; });

            static int selectedEnemy = -1; // UID
            static int editedEnemy = -1;   // Enemy the editor below was filled from
            static std::vector<EntityType> enemyTypes;
            static std::vector<AbilityData> enemyAbilities;

            auto findEnemy = [&enemies](int uid) -> std::shared_ptr<EntityData>
                {
                    for (const auto& data : enemies) if (data->UID == uid) return data;
                    return nullptr;
                };

            if (!findEnemy(selectedEnemy) && !enemies.empty())
                selectedEnemy = enemies.front()->UID;

            auto enemyLabel = [](const EntityData& data) { return std::to_string(data.UID) + " - " + data.name; };

            if (auto enemyData = findEnemy(selectedEnemy))
            {
                // Only defined enemies can be picked
                if (ImGui::BeginCombo("Enemy", enemyLabel(*enemyData).c_str()))
                {
                    for (const auto& data : enemies)
                    {
                        if (ImGui::Selectable(enemyLabel(*data).c_str(), data->UID == selectedEnemy))
                            selectedEnemy = data->UID;
                    }
                    ImGui::EndCombo();
                }

                enemyData = findEnemy(selectedEnemy);

                // New enemy picked: start from its JSON types and abilities
                if (editedEnemy != selectedEnemy)
                {
                    enemyTypes = enemyData->types;
                    enemyAbilities = enemyData->abilities;
                    editedEnemy = selectedEnemy;
                }

                show_debug_preview("EnemyPreview", enemyData->texture, enemyData->frameCount);

                ImGui::PushID("Enemy");
                edit_types_and_abilities(enemyTypes, enemyAbilities);
                if (ImGui::Button("Reset to JSON data"))
                {
                    enemyTypes = enemyData->types;
                    enemyAbilities = enemyData->abilities;
                }
                ImGui::PopID();

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

                // "##Enemy" keeps the button's ID different from the "Spawn Enemy" header above
                if (ImGui::Button("Spawn##Enemy", ImVec2(-FLT_MIN, 0.f)))
                {
                    auto customData = std::make_shared<EntityData>(*enemyData);
                    customData->types = enemyTypes;
                    customData->abilities = enemyAbilities;

                    // Spawn enemy with full parameter set
                    m_stage->spawn_enemy(
                        customData,
                        sf::Vector2f(hpMultiplier, attackMultiplier),
                        enemyLayer,
                        isBoss,
                        bypassEnemyLimit
                    );
                }
            }
        }

    }

    ImGui::End();


    /*
    * =======================================================================================================
    * Statuses Panel
    * -----------------------------------------------------------------
    * Guaranteed / normal chances, active statuses on the field, and the log of every status request
    */
    if (isStageLoaded)
    {
        const ImVec2 statusWindowSize = ImVec2(480.f, 1080.f);
        ImGui::SetNextWindowSize(statusWindowSize, ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);

        ImGui::Begin("Statuses");

        StatusManager& statusManager = m_stage->get_status_manager();

        // Guaranteed mode makes effects easy to see, normal mode keeps the real chances for balancing
        bool isGuaranteed = statusManager.is_guaranteed_mode();
        if (ImGui::Checkbox("Guaranteed effects (every chance = 100%)", &isGuaranteed))
            statusManager.set_guaranteed_mode(isGuaranteed);

        ImGui::SeparatorText("Active statuses");

        bool hasAnyStatus = false;
        auto listStatuses = [&](const char* side, const BattleEntitiesMap_t& entities)
            {
                for (const auto& [layer, list] : entities)
                {
                    for (const auto& entity : list)
                    {
                        if (entity->get_statuses().empty() || !entity->data) continue;
                        hasAnyStatus = true;

                        ImGui::Text("%s %s (%.0f HP)", side, entity->data->name.c_str(), entity->currentHealth);
                        for (const StatusEffect& status : entity->get_statuses())
                            ImGui::BulletText("%s  x%.2f  %.1f s left", to_string(status.type), status.value, status.durationRemaining);
                    }
                }
            };

        listStatuses("[Unit]", m_stage->get_units());
        listStatuses("[Enemy]", m_stage->get_enemies());
        if (!hasAnyStatus) ImGui::TextDisabled("No entity has a status");

        ImGui::SeparatorText("Log (newest first)");
        if (ImGui::Button("Clear log")) statusManager.clear_log();

        ImGui::BeginChild("StatusLog");
        const auto& log = statusManager.get_log();
        for (auto it = log.rbegin(); it != log.rend(); ++it)
        {
            const char* outcome = "MISSED";
            if (it->outcome == StatusOutcome::APPLIED) outcome = "APPLIED";
            else if (it->outcome == StatusOutcome::STACKED) outcome = "STACKED";

            ImVec4 color = (it->outcome == StatusOutcome::MISSED) ? ImVec4(0.6f, 0.6f, 0.6f, 1.f) : ImVec4(0.5f, 1.f, 0.6f, 1.f);
            ImGui::TextColored(color, "[%.1fs] %s -> %s: %s %s", it->time, it->source.c_str(), it->target.c_str(), to_string(it->status), outcome);

            if (it->guaranteed)
                ImGui::TextDisabled("    guaranteed, x%.2f for %.1f s", it->value, it->duration);
            else
                ImGui::TextDisabled("    roll %.1f vs %.0f%% chance, x%.2f for %.1f s", it->roll, it->chance, it->value, it->duration);
        }
        if (log.empty()) ImGui::TextDisabled("No status requested yet");
        ImGui::EndChild();

        ImGui::End();
    }
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
    return ImTextureID{};
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
            // Close game (removed)
            //else if (e_keycode->code == sf::Keyboard::Key::Escape) m_window.close();
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

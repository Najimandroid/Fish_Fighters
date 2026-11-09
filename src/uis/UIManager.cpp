#include "UIManager.h"

#include "../core/stages/Stage.h"
#include "../core/Game.h"

#include "widgets/UIDeploymentIcons.h"
#include "widgets/UIStageCash.h"
#include "widgets/UICashUp.h"
#include "widgets/UIBaseHealthInfo.h"
#include "widgets/UIStatusIcons.h"
#include "widgets/UIUpgradeSlider.h"
#include "widgets/UIEquipSlider.h"
#include "widgets/UIPlayerShellsInfo.h"
#include "widgets/UIChapterMap.h"
#include "widgets/UIRoundButton.h"
#include "widgets/UIPauseMenu.h"
#include "widgets/UITitleBar.h"
#include "widgets/UIPatternBackground.h"
#include "widgets/UIEncyclopedia.h"

#include <algorithm>
#include <iostream>

// Initialize the manager with references to the data loader and current stage
void UIManager::init(std::shared_ptr<DataLoader> dataLoader, std::shared_ptr<Stage> stage)
{
    m_dataLoader = dataLoader;
    m_stage = stage;
}

// Add a UI element to the manager
void UIManager::add_ui_element(std::shared_ptr<UIElement> element)
{
    m_uiElements.push_back(element);
}

// Update all UI elements (called each frame)
void UIManager::update_uis(float deltaTime)
{
    for (auto& element : m_uiElements)
    {
        element->update(deltaTime);
    }
}

// Render all UI elements
// Handles world-space vs screen-space rendering by switching the view as needed
void UIManager::render_uis(sf::RenderWindow& window, sf::View& uiView, sf::View& worldView)
{
    for (auto& element : m_uiElements)
    {
        if (element->isWorldSpaceUi)
        {
            window.setView(worldView);
            element->render(window);
            window.setView(uiView);
        }
        else
        {
            element->render(window);
        }
    }
}

// Handle input events for all UI elements
// Executes deferred actions after processing events
void UIManager::handle_ui_events(const sf::Event& event, const sf::RenderWindow& window)
{
    // A modal window (the pause menu) gets the events alone; the last one added is the one on top
    auto blocking = std::find_if(m_uiElements.rbegin(), m_uiElements.rend(),
        [](const auto& uiElement) { return uiElement->is_visible() && uiElement->is_blocking_input(); });

    if (blocking != m_uiElements.rend())
    {
        (*blocking)->handle_event(event, window);
    }
    else
    {
        for (auto& uiElement : m_uiElements)
        {
            uiElement->handle_event(event, window);
        }
    }

    // Execute deferred action if set by button callbacks
    if (m_pendingAction)
    {
        auto action = m_pendingAction;
        m_pendingAction = nullptr;
        action();
    }
}

/*
 * Generate Fish Tank UI screen (the hub, laid out like The Battle Cats' Cat Base):
 *   - The fish tank background, the steel title bar and the player's shells
 *   - A column of big yellow buttons: BATTLE! (chapter map), UPGRADE, EQUIP, QUIT
 *   - Each button sets a deferred callback using m_pendingAction
 */
void UIManager::generate_fish_tank_uis()
{
    m_uiElements.clear();

    auto bg = std::make_shared<UITextureElement>(
        sf::Vector2f{ 1920.f, 1080.f },
        sf::Vector2f{ 0.f, 0.f },
        "assets/images/textures/backgrounds/menus/fish_tank.png"
    );

    auto titleBar = std::make_shared<UITitleBar>("Fish Tank");
    auto shellsInfo = std::make_shared<UIPlayerShellsInfo>(m_dataLoader);

    // Column of buttons on the left
    const float columnX = 70.f;
    const float columnWidth = 560.f;

    // Battle button opens the chapter map
    auto battleButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ columnWidth, 150.f },
        sf::Vector2f{ columnX, 190.f },
        "BATTLE!"
    );
    battleButton->set_text_size(70);
    battleButton->set_callback([this]()
        {
            m_pendingAction = [this]() {
                    generate_chapter_map_uis();
                };
        });

    // Upgrade button switches UI to upgrade screen
    auto upgradeButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ columnWidth, 120.f },
        sf::Vector2f{ columnX, 372.f },
        "UPGRADE"
    );
    upgradeButton->set_text_size(52);
    upgradeButton->set_callback([this]()
        {
            m_pendingAction = [this]() {
                generate_upgrade_menu_uis();
                };
        });

    // Equip button switches UI to equip screen
    auto equipButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ columnWidth, 120.f },
        sf::Vector2f{ columnX, 524.f },
        "EQUIP"
    );
    equipButton->set_text_size(52);
    equipButton->set_callback([this]()
        {
            m_pendingAction = [this]() {
                generate_equip_menu_uis();
                };
        });

    // Encyclopedia button: every fish and every enemy met
    auto encyclopediaButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ columnWidth, 120.f },
        sf::Vector2f{ columnX, 676.f },
        "ENCYCLOPEDIA"
    );
    encyclopediaButton->set_text_size(52);
    encyclopediaButton->set_callback([this]()
        {
            m_pendingAction = [this]() {
                generate_encyclopedia_uis();
                };
        });

    // Quit button closes the game (smaller, orange)
    auto quitButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ 300.f, 96.f },
        sf::Vector2f{ columnX, 828.f },
        "QUIT"
    );
    quitButton->set_text_size(40);
    quitButton->set_fill_color(sf::Color(255, 150, 130));
    quitButton->set_callback([this]()
        {
            m_pendingAction = [this]() {
                if (m_quitGameCallback)
                {
                    m_quitGameCallback();
                }
                };
        });

    add_ui_element(bg);
    add_ui_element(titleBar);
    add_ui_element(shellsInfo);
    add_ui_element(battleButton);
    add_ui_element(upgradeButton);
    add_ui_element(equipButton);
    add_ui_element(encyclopediaButton);
    add_ui_element(quitButton);
}

/*
 * Generate the Encyclopedia: every fish and every enemy (black until owned / met), their stats and abilities
 */
void UIManager::generate_encyclopedia_uis()
{
    m_uiElements.clear();

    auto bg = std::make_shared<UIPatternBackground>("pattern_hull");
    auto encyclopedia = std::make_shared<UIEncyclopedia>(m_dataLoader);
    auto titleBar = std::make_shared<UITitleBar>("Encyclopedia");
    auto shellsInfo = std::make_shared<UIPlayerShellsInfo>(m_dataLoader);
    auto backButton = make_back_button([this]() { generate_fish_tank_uis(); });

    add_ui_element(bg);
    add_ui_element(titleBar);
    add_ui_element(shellsInfo);
    add_ui_element(backButton);
    add_ui_element(encyclopedia);   // Last: its ability bubbles go over everything
}

/*
 * The round back button of the menus (bottom left), like The Battle Cats' curved arrow
 */
std::shared_ptr<UIRoundButton> UIManager::make_back_button(std::function<void()> onBack)
{
    const float size = 120.f;
    auto backButton = std::make_shared<UIRoundButton>(size, sf::Vector2f{ 24.f, 1080.f - 24.f - size }, "icon_back");
    backButton->set_callback([this, onBack]()
        {
            m_pendingAction = onBack;
        });
    return backButton;
}

/*
 * Generate the Upgrade screen: wave pattern, title bar, shells, the upgrade carousel, back to the Fish Tank
 */
void UIManager::generate_upgrade_menu_uis()
{
    m_uiElements.clear();

    auto bg = std::make_shared<UIPatternBackground>("pattern_hull");
    auto upgradeSlider = std::make_shared<UIUpgradeSlider>(m_dataLoader);
    auto titleBar = std::make_shared<UITitleBar>("Upgrade");
	auto shellsInfo = std::make_shared<UIPlayerShellsInfo>(m_dataLoader);
    auto backButton = make_back_button([this]() { generate_fish_tank_uis(); });

    add_ui_element(bg);
    add_ui_element(upgradeSlider);
    add_ui_element(titleBar);
    add_ui_element(shellsInfo);
    add_ui_element(backButton);
}

/*
 * Generate the Equip screen: wave pattern, title bar, shells, the cards and the lineup,
 * back to the Fish Tank (saves the lineup)
 */
void UIManager::generate_equip_menu_uis()
{
    m_uiElements.clear();

    auto bg = std::make_shared<UIPatternBackground>("pattern_hull");
    auto equipSlider = std::make_shared<UIEquipSlider>(m_dataLoader);
    auto titleBar = std::make_shared<UITitleBar>("Equip");
    auto shellsInfo = std::make_shared<UIPlayerShellsInfo>(m_dataLoader);
    auto backButton = make_back_button([this]()
        {
            // Save the loadout when leaving the equip menu
            m_dataLoader->save_progress();
            generate_fish_tank_uis();
        });

    add_ui_element(bg);
    add_ui_element(equipSlider);
    add_ui_element(titleBar);
    add_ui_element(shellsInfo);
    add_ui_element(backButton);
}

/*
 * Generate the battle screen (Battle Cats layout):
 *   - Round pause and speed buttons at the top left, cash at the top right
 *   - The 10 deploy slots at the bottom, Cash Up at the bottom left
 *   - Health over the bases and status icons under the entities (world space)
 *   - The pause menu (Continue / Retreat), drawn over everything while the battle is paused
 */
void UIManager::generate_battle_uis()
{
	m_uiElements.clear();

    std::string stageName;
    if (auto stage = m_stage.lock())
    {
        if (auto stageData = m_dataLoader->get_stage_data(stage->get_current_uid()))
            stageName = stageData->name;
    }

    auto pauseMenu = std::make_shared<UIPauseMenu>(stageName);
    std::weak_ptr<UIPauseMenu> weakPauseMenu = pauseMenu; // The menu keeps these callbacks: no shared_ptr to itself

    // Pauses the battle (remembering if it ran faster) and opens the menu
    auto openPauseMenu = [this, weakPauseMenu]() {
        // defer to main loop to avoid changing state during event iteration
        m_pendingAction = [this, weakPauseMenu]() {
            if (!m_getGameSpeedCallback || !m_setGameSpeedCallback) return;
            m_wasFasterBeforePause = m_getGameSpeedCallback() == GameSpeedState::FASTER;
            m_setGameSpeedCallback(GameSpeedState::PAUSED);
            if (auto menu = weakPauseMenu.lock()) menu->open();
            };
        };

    pauseMenu->set_open_callback(openPauseMenu);
    pauseMenu->set_continue_callback([this, weakPauseMenu]() {
        m_pendingAction = [this, weakPauseMenu]() {
            if (m_setGameSpeedCallback)
                m_setGameSpeedCallback(m_wasFasterBeforePause ? GameSpeedState::FASTER : GameSpeedState::NORMAL);
            if (auto menu = weakPauseMenu.lock()) menu->close();
            };
        });
    pauseMenu->set_is_paused_callback([this]() {
        return m_getGameSpeedCallback && m_getGameSpeedCallback() == GameSpeedState::PAUSED;
        });

    // Retreat: leaves the battle without rewards and goes back to the chapter map
    pauseMenu->set_retreat_callback([this]() {
        m_pendingAction = [this]() {
            if (m_setGameSpeedCallback) m_setGameSpeedCallback(GameSpeedState::NORMAL);
            m_dataLoader->save_progress(); // No rewards, but the enemies met stay met
            if (auto stage = m_stage.lock())
            {
                stage->unload();
                generate_chapter_map_uis();
            }
            };
        });

    // Pause button (top left)
    auto pauseButton = std::make_shared<UIRoundButton>(100.f, sf::Vector2f{ 24.f, 24.f }, "icon_pause");
    pauseButton->set_callback(openPauseMenu);

	// Speed up button: lit while the battle runs faster
    auto speedButton = std::make_shared<UIRoundButton>(100.f, sf::Vector2f{ 140.f, 24.f }, "icon_speed");
    speedButton->set_callback([this]() {
        m_pendingAction = [this]() {
            if (!m_getGameSpeedCallback || !m_setGameSpeedCallback) return;
            auto current = m_getGameSpeedCallback();
            if (current == GameSpeedState::FASTER)
                m_setGameSpeedCallback(GameSpeedState::NORMAL);
            else
                m_setGameSpeedCallback(GameSpeedState::FASTER);
            };
        });
    speedButton->set_active_callback([this]() {
        return m_getGameSpeedCallback && m_getGameSpeedCallback() == GameSpeedState::FASTER;
        });

	auto deploymentIcons = std::make_shared<UIDeploymentIcons>(m_dataLoader, m_stage.lock());
	auto stageCash = std::make_shared<UIStageCash>(m_stage.lock());
	auto cashUp = std::make_shared<UICashUp>(m_stage.lock());
	auto baseHealthInfo = std::make_shared<UIBaseHealthInfo>(m_stage.lock());
	auto statusIcons = std::make_shared<UIStatusIcons>(m_stage.lock(), m_dataLoader); // Status icons under every entity

    // World-space widgets first, so the screen widgets are drawn over them
	add_ui_element(statusIcons);
	add_ui_element(baseHealthInfo);
    add_ui_element(pauseButton);
    add_ui_element(speedButton);
	add_ui_element(deploymentIcons);
	add_ui_element(stageCash);
	add_ui_element(cashUp);
    add_ui_element(pauseMenu); // Last: on top, and the first to get the input
}

/*
 * Generate the chapter map: the map with its stages and the selected stage's panel (UIChapterMap),
 * the chapter's name in the title bar, a big BATTLE! button, back to the Fish Tank
 */
void UIManager::generate_chapter_map_uis()
{
    m_uiElements.clear();

    auto chapterMap = std::make_shared<UIChapterMap>(m_dataLoader);

    // Battle button triggers stage load and switches UI
    auto battleButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ 420.f, 160.f },
        sf::Vector2f{ 1470.f, 880.f },
        "BATTLE!"
    );
    battleButton->set_text_size(66);
    battleButton->set_callback([this, chapterMap]()
        {
            m_pendingAction = [this, chapterMap]() {
                if (auto stage = m_stage.lock())
                {
                    stage->load(chapterMap->get_selected_stage_uid());
                    generate_battle_uis();
                }
                };
        });

    const int chapterUid = m_dataLoader->get_player_data().lock()->currentChapter;
    chapterMap->load_chapter(chapterUid);

    // "Chapter 1 - Shallow Waters"
    std::string title = "Chapter " + std::to_string(chapterUid);
    if (auto chapter = m_dataLoader->get_chapter_data(chapterUid))
        title += "  -  " + chapter->name;
    auto titleBar = std::make_shared<UITitleBar>(title);
    auto shellsInfo = std::make_shared<UIPlayerShellsInfo>(m_dataLoader);

    auto backButton = make_back_button([this]() { generate_fish_tank_uis(); });

    add_ui_element(chapterMap);
    add_ui_element(titleBar);
    add_ui_element(shellsInfo);
    add_ui_element(battleButton);
    add_ui_element(backButton);
}

void UIManager::generate_victory_uis()
{
    m_uiElements.clear();

	//std::cout << m_stage.lock()->get_current_uid() << "\n";

    m_setGameSpeedCallback(GameSpeedState::NORMAL);

    // Victory Text
    auto title = std::make_shared<UITextElement>(
        sf::Vector2f{ 200.f, 100.f },
        sf::Vector2f{ 0.f, 0.f },
        "Victory!",
        72
    );

    sf::FloatRect textBounds = title->get_text().getLocalBounds();
    title->get_text().setOrigin({
        textBounds.position.x + textBounds.size.x / 2.f,
        textBounds.position.y + textBounds.size.y / 2.f
        });
    title->set_position({ 1920.f / 2.f, 1080.f / 2.f });

    // Rewards
    auto rewards = m_dataLoader->get_stage_data(m_stage.lock()->get_current_uid())->rewards;
    int gainedShells = 0;
	bool isNewUnitAvailable = false;
	int newUnitUID = -1;

    for (auto& reward : rewards)
    {
        if(reward->type == "shells")
        {
            gainedShells += reward->amount;
            continue;
		}

        if (reward->type == "unit")
        {
            isNewUnitAvailable = true;
			newUnitUID = reward->unitUID;
			continue;
        }
    }

	// Shells Reward Text
    auto shellRewardText = std::make_shared<UITextElement>(
        sf::Vector2f{ 200.f, 100.f },
        sf::Vector2f{ 0.f, 0.f },
        '+' + std::to_string(gainedShells) + " \xA4", // \xA4: the shells symbol (currency sign) in the game's code page
        60
    );

    sf::FloatRect shellsTextBound = shellRewardText->get_text().getLocalBounds();
    shellRewardText->get_text().setOrigin({
        shellsTextBound.position.x + shellsTextBound.size.x / 2.f,
        shellsTextBound.position.y + shellsTextBound.size.y / 2.f
        });
    shellRewardText->set_position({ 1920.f / 2.f, 1080.f / 2.f + 100.f});

    shellRewardText->set_text_color(sf::Color(0, 255, 255, 255));

	// New Unit Available Text
    auto newUnitText = std::make_shared<UITextElement>(
        sf::Vector2f{ 200.f, 100.f },
        sf::Vector2f{ 0.f, 0.f },
        "A NEW UNIT IS AVAILABLE IN THE UPGRADES MENU!",
        46
    );

    sf::FloatRect unitTextBound = newUnitText->get_text().getLocalBounds();
    newUnitText->get_text().setOrigin({
        unitTextBound.position.x + unitTextBound.size.x / 2.f,
        unitTextBound.position.y + unitTextBound.size.y / 2.f
        });
    newUnitText->set_position({ 1920.f / 2.f, 1080.f / 2.f + 200.f });

	// Show the new unit text only if a new unit is available and not already owned by the player
    if(isNewUnitAvailable && !(m_dataLoader->get_player_data().lock()->is_unit_owned(newUnitUID)))
        newUnitText->set_text_color(sf::Color(255, 0, 255, 255));
    else
        newUnitText->set_text_color(sf::Color(255, 0, 255, 0));


    // Back to menu button
    auto menuButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ 100.f, 100.f },
        sf::Vector2f{ 20.f, 1080.f - 100.f - 20.f },
        "MENU"
    );
    menuButton->set_callback([this]()
        {
            m_pendingAction = [this]() {
                if (auto stage = m_stage.lock())
                {
                    m_stage.lock()->unload();
                    generate_fish_tank_uis();
                }
                };
        });

    //back to map button
    auto mapButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ 100.f, 100.f },
        sf::Vector2f{ 150.f, 1080.f - 100.f - 20.f },
        "MAP"
    );
    mapButton->set_callback([this]()
        {
            m_pendingAction = [this]() {
                if (auto stage = m_stage.lock())
                {
                    m_stage.lock()->unload();
                    generate_chapter_map_uis();
                }
                };
        });

    add_ui_element(title);
    add_ui_element(shellRewardText);
    add_ui_element(newUnitText);

    add_ui_element(menuButton);
    add_ui_element(mapButton);
}

void UIManager::generate_defeat_uis()
{
    m_uiElements.clear();

    m_setGameSpeedCallback(GameSpeedState::NORMAL);

    // Victory Text
    auto title = std::make_shared<UITextElement>(
        sf::Vector2f{ 200.f, 100.f },
        sf::Vector2f{ 0.f, 0.f },
        "Defeat...",
        72
    );

    sf::FloatRect textBounds = title->get_text().getLocalBounds();
    title->get_text().setOrigin({
        textBounds.position.x + textBounds.size.x / 2.f,
        textBounds.position.y + textBounds.size.y / 2.f
        });
    title->set_position({ 1920.f / 2.f, 1080.f / 2.f });

    //back to menu button
    auto menuButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ 100.f, 100.f },
        sf::Vector2f{ 20.f, 1080.f - 100.f - 20.f },
        "MENU"
    );
    menuButton->set_callback([this]()
        {
            m_pendingAction = [this]() {
                if (auto stage = m_stage.lock())
                {
                    m_stage.lock()->unload();
                    generate_fish_tank_uis();
                }
                };
        });

    //back to map button
    auto mapButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ 100.f, 100.f },
        sf::Vector2f{ 150.f, 1080.f - 100.f - 20.f },
        "MAP"
    );
    mapButton->set_callback([this]()
        {
            m_pendingAction = [this]() {
                if (auto stage = m_stage.lock())
                {
                    m_stage.lock()->unload();
                    generate_chapter_map_uis();
                }
                };
        });

    add_ui_element(title);
    add_ui_element(menuButton);
    add_ui_element(mapButton);
}

bool UIManager::is_mouse_over_ui(const sf::Vector2i& worldPosition) const
{
    for (auto& e : m_uiElements)
    {
        if (!e) continue;
        sf::FloatRect b = e->get_bounds();
        if (b.contains(static_cast<sf::Vector2f>(worldPosition))) return true;
    }
    return false;
}

void UIManager::set_game_speed_callback(std::function<void(GameSpeedState)> callback)
{
    m_setGameSpeedCallback = callback;
}

void UIManager::set_get_game_speed_callback(std::function<GameSpeedState()> callback)
{
    m_getGameSpeedCallback = callback;
}

void UIManager::set_quit_game_callback(std::function<void()> callback)
{
    m_quitGameCallback = callback;
}

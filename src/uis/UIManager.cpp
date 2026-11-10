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
#include "widgets/UITitleScreen.h"
#include "widgets/UICashPopups.h"
#include "widgets/UIConfirmDialog.h"
#include "widgets/UIOptionsMenu.h"
#include "widgets/UIResultsPanel.h"
#include "widgets/UIBadge.h"
#include "widgets/UIUpgradeIcon.h"
#include "UISkin.h"

#include "../audio/AudioManager.h"

#include <algorithm>
#include <iostream>

namespace
{
    const std::string MENU_MUSIC = "theme_base";     // The Fish Tank menus
    const std::string TITLE_MUSIC = "theme_title";   // The title screen
    const float TITLE_MUSIC_TEMPO = 168.f;           // Its tempo: the title bounces on the beat
}

// Initialize the manager with references to the data loader, current stage and audio
void UIManager::init(std::shared_ptr<DataLoader> dataLoader, std::shared_ptr<Stage> stage, std::shared_ptr<AudioManager> audio)
{
    m_dataLoader = dataLoader;
    m_stage = stage;
    m_audio = audio;

    // Every widget's sounds (clicks, selections, purchases...) go to the AudioManager
    std::weak_ptr<AudioManager> weakAudio = audio;
    UIElement::set_sound_player([weakAudio](const std::string& name, float pitch, float pan)
        {
            if (auto audio = weakAudio.lock()) audio->play_sound(name, 1.f, pan, pitch);
        });
}

/*
 * Clears the screen for a Fish Tank menu: the Fish Tank theme (it keeps playing from one menu to the next)
 * and a swish of water between the screens
 */
void UIManager::enter_menu_screen()
{
    m_uiElements.clear();
    if (m_audio)
    {
        m_audio->play_music(MENU_MUSIC);
        m_audio->play_sound("ui_transition");
    }
}

/*
 * Title screen: the fish tank behind the bouncing "FISH FIGHTERS" title, bubbles, the title theme,
 * a big START button (or any key), QUIT in the corner
 */
void UIManager::generate_title_uis()
{
    m_uiElements.clear();
    if (m_audio) m_audio->play_music(TITLE_MUSIC, 0.f);

    auto bg = std::make_shared<UITextureElement>(
        sf::Vector2f{ 1920.f, 1080.f },
        sf::Vector2f{ 0.f, 0.f },
        "assets/images/textures/backgrounds/menus/fish_tank.png"
    );
    auto title = std::make_shared<UITitleScreen>(TITLE_MUSIC_TEMPO);

    // START (or any key): the splash sound, then the Fish Tank
    auto start = [this]()
        {
            m_pendingAction = [this]() { generate_fish_tank_uis(); };
        };
    title->set_start_callback([this, start]()
        {
            if (m_audio) m_audio->play_sound("ui_start");
            start();
        });

    auto startButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ 560.f, 170.f },
        sf::Vector2f{ 960.f - 280.f, 650.f },
        "START"
    );
    startButton->set_text_size(84);
    startButton->set_click_sound("ui_start");
    startButton->set_callback(start);

    auto quitButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ 240.f, 90.f },
        sf::Vector2f{ 30.f, 1080.f - 30.f - 90.f },
        "QUIT"
    );
    quitButton->set_text_size(38);
    quitButton->set_fill_color(sf::Color(255, 150, 130));
    auto quitDialog = make_quit_dialog(quitButton);

    auto optionsMenu = make_options_menu();
    auto optionsButton = make_options_button(optionsMenu, { 280.f, 90.f }, { 290.f, 1080.f - 30.f - 90.f }, 38);

    add_ui_element(bg);
    add_ui_element(title);
    add_ui_element(startButton);
    add_ui_element(quitButton);
    add_ui_element(optionsButton);
    add_ui_element(optionsMenu);  // Windows last: on top, and the only ones getting the input while open
    add_ui_element(quitDialog);
}

/*
 * "Quit Fish Fighters?" before closing the game (title screen and Fish Tank): quitButton opens it.
 * The button keeps a weak_ptr (the screen's element list owns the dialog).
 */
std::shared_ptr<UIConfirmDialog> UIManager::make_quit_dialog(const std::shared_ptr<UIButtonElement>& quitButton)
{
    auto dialog = std::make_shared<UIConfirmDialog>("Quit Fish Fighters?", "Your progress is saved.", "QUIT", "CANCEL");
    std::weak_ptr<UIConfirmDialog> weakDialog = dialog;
    quitButton->set_callback([this, weakDialog]()
        {
            if (auto dialog = weakDialog.lock())
                dialog->open([this]() { m_pendingAction = [this]() { if (m_quitGameCallback) m_quitGameCallback(); }; });
        });
    return dialog;
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
            // A window (pause, options, confirmation) dims the screen inside the 16:9 UI area itself;
            // on other screen shapes the bars around that area are dimmed with it
            if (element->is_visible() && element->is_blocking_input())
                dim_letterbox_bars(window, uiView);
            element->render(window);
        }
    }
}

/*
 * Darkens the parts of the window outside the UI view (the bars on screens that aren't 16:9),
 * like the modal windows darken the screen
 */
void UIManager::dim_letterbox_bars(sf::RenderWindow& window, const sf::View& uiView) const
{
    const sf::Vector2f size = static_cast<sf::Vector2f>(window.getSize());
    const sf::FloatRect area{ { uiView.getViewport().position.x * size.x, uiView.getViewport().position.y * size.y },
        { uiView.getViewport().size.x * size.x, uiView.getViewport().size.y * size.y } };
    if (area.size.x >= size.x - 1.f && area.size.y >= size.y - 1.f) return;   // 16:9: no bars

    window.setView(sf::View(sf::FloatRect{ { 0.f, 0.f }, size }));
    sf::RectangleShape bar;
    bar.setFillColor(sf::Color(0, 0, 0, 150));
    const sf::FloatRect bars[] = {
        { { 0.f, 0.f }, { area.position.x, size.y } },                                        // Left
        { { area.position.x + area.size.x, 0.f }, { size.x - area.position.x - area.size.x, size.y } },   // Right
        { { area.position.x, 0.f }, { area.size.x, area.position.y } },                       // Top
        { { area.position.x, area.position.y + area.size.y }, { area.size.x, size.y - area.position.y - area.size.y } } // Bottom
    };
    for (const sf::FloatRect& rect : bars)
    {
        if (rect.size.x <= 0.f || rect.size.y <= 0.f) continue;
        bar.setPosition(rect.position);
        bar.setSize(rect.size);
        window.draw(bar);
    }
    window.setView(uiView);
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
    enter_menu_screen();

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
    // Under BATTLE!: the stage the map will open on
    auto nextStage = std::make_shared<UITextElement>(sf::Vector2f{ columnWidth, 30.f }, sf::Vector2f{ 0.f, 0.f }, next_stage_label(), 26);
    UISkin::style_text(nextStage->get_text(), 26, UISkin::OUTLINE);
    nextStage->get_text().setOutlineThickness(0.f);
    UISkin::set_text_anchor(nextStage->get_text(), { 0.5f, 0.5f });
    nextStage->set_position({ columnX + columnWidth / 2.f, 190.f + 150.f - 24.f });

    // Upgrade button switches UI to upgrade screen
    auto upgradeButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ columnWidth, 120.f },
        sf::Vector2f{ columnX, 372.f },
        "UPGRADE"
    );
    upgradeButton->set_text_size(52);
    upgradeButton->set_style(UIButtonElement::Style::Secondary);
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
    equipButton->set_style(UIButtonElement::Style::Secondary);
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
    encyclopediaButton->set_style(UIButtonElement::Style::Secondary);
    encyclopediaButton->set_callback([this]()
        {
            m_pendingAction = [this]() {
                generate_encyclopedia_uis();
                };
        });

    // Options and Quit share the last row (Quit smaller, orange, and it asks first)
    auto optionsMenu = make_options_menu();
    auto optionsButton = make_options_button(optionsMenu, { 300.f, 96.f }, { columnX, 828.f }, 40);
    optionsButton->set_style(UIButtonElement::Style::Secondary);

    // "!" on UPGRADE when a fish can be upgraded or unlocked with the shells there are
    auto upgradeBadge = std::make_shared<UIBadge>(sf::Vector2f{ columnX + columnWidth - 14.f, 372.f + 12.f }, "!");
    upgradeBadge->set_visible(is_upgrade_available());

    auto quitButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ 230.f, 96.f },
        sf::Vector2f{ columnX + columnWidth - 230.f, 828.f },
        "QUIT"
    );
    quitButton->set_text_size(40);
    quitButton->set_fill_color(sf::Color(255, 150, 130));
    auto quitDialog = make_quit_dialog(quitButton);

    add_ui_element(bg);
    add_ui_element(titleBar);
    add_ui_element(shellsInfo);
    add_ui_element(battleButton);
    add_ui_element(upgradeButton);
    add_ui_element(equipButton);
    add_ui_element(encyclopediaButton);
    add_ui_element(nextStage);
    add_ui_element(upgradeBadge);
    add_ui_element(optionsButton);
    add_ui_element(quitButton);
    add_ui_element(optionsMenu);  // Windows last: on top while open
    add_ui_element(quitDialog);
}

/*
 * Generate the Encyclopedia: every fish and every enemy (black until owned / met), their stats and abilities
 */
void UIManager::generate_encyclopedia_uis()
{
    enter_menu_screen();

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
    backButton->set_click_sound("ui_back");
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
    enter_menu_screen();

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
    enter_menu_screen();

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
            if (m_audio) m_audio->play_sound("ui_pause");   // The Game lowers the music while paused
            if (auto menu = weakPauseMenu.lock()) menu->open();
            };
        };

    pauseMenu->set_open_callback(openPauseMenu);
    pauseMenu->set_continue_callback([this, weakPauseMenu]() {
        m_pendingAction = [this, weakPauseMenu]() {
            if (m_setGameSpeedCallback)
                m_setGameSpeedCallback(m_wasFasterBeforePause ? GameSpeedState::FASTER : GameSpeedState::NORMAL);
            if (m_audio) m_audio->play_sound("ui_unpause");
            if (auto menu = weakPauseMenu.lock()) menu->close();
            };
        });
    pauseMenu->set_is_paused_callback([this]() {
        return m_getGameSpeedCallback && m_getGameSpeedCallback() == GameSpeedState::PAUSED;
        });

    // Options: the same window as in the menus, over the pause menu
    auto optionsMenu = make_options_menu();
    std::weak_ptr<UIOptionsMenu> weakOptionsMenu = optionsMenu;
    pauseMenu->set_options_callback([weakOptionsMenu]() { if (auto menu = weakOptionsMenu.lock()) menu->open(); });

    // Retreat: asks first, then leaves the battle without rewards and goes back to the chapter map
    auto retreatDialog = std::make_shared<UIConfirmDialog>("Retreat?",
        "You'll lose this battle and earn no shells.", "RETREAT", "KEEP FIGHTING");
    std::weak_ptr<UIConfirmDialog> weakRetreatDialog = retreatDialog;
    pauseMenu->set_retreat_callback([this, weakRetreatDialog]() {
        auto dialog = weakRetreatDialog.lock();
        if (!dialog) return;
        dialog->open([this]() {
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
        });

    // Pause button (top left), its sound is the pause menu's
    auto pauseButton = std::make_shared<UIRoundButton>(100.f, sf::Vector2f{ 24.f, 24.f }, "icon_pause");
    pauseButton->set_click_sound("");
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
    speedButton->set_hotkey(sf::Keyboard::Key::F);

	auto deploymentIcons = std::make_shared<UIDeploymentIcons>(m_dataLoader, m_stage.lock());
	auto stageCash = std::make_shared<UIStageCash>(m_stage.lock());
	auto cashUp = std::make_shared<UICashUp>(m_stage.lock());
	auto baseHealthInfo = std::make_shared<UIBaseHealthInfo>(m_stage.lock());
	auto statusIcons = std::make_shared<UIStatusIcons>(m_stage.lock(), m_dataLoader); // Status icons under every entity
	auto cashPopups = std::make_shared<UICashPopups>(m_stage.lock());              // "+X$" where enemies drop cash

    // World-space widgets first, so the screen widgets are drawn over them
	add_ui_element(statusIcons);
	add_ui_element(cashPopups);
	add_ui_element(baseHealthInfo);
    add_ui_element(pauseButton);
    add_ui_element(speedButton);
	add_ui_element(deploymentIcons);
	add_ui_element(stageCash);
	add_ui_element(cashUp);
    add_ui_element(pauseMenu); // On top, and the first to get the input
    add_ui_element(retreatDialog); // Over the pause menu while it asks
    add_ui_element(optionsMenu);   // Over the pause menu while open
}

/*
 * Generate the chapter map: the map with its stages and the selected stage's panel (UIChapterMap),
 * the chapter's name in the title bar, a big BATTLE! button, back to the Fish Tank
 */
void UIManager::generate_chapter_map_uis()
{
    enter_menu_screen();

    auto chapterMap = std::make_shared<UIChapterMap>(m_dataLoader);

    // Battle button triggers stage load and switches UI
    auto battleButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ 420.f, 160.f },
        sf::Vector2f{ 1470.f, 880.f },
        "BATTLE!"
    );
    battleButton->set_text_size(66);
    battleButton->set_click_sound("");   // The battle start jingle instead; the stage's theme follows it
    battleButton->set_callback([this, chapterMap]()
        {
            m_pendingAction = [this, chapterMap]() { start_battle(chapterMap->get_selected_stage_uid()); };
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

/*
 * Starts a battle (from the map, RETRY or NEXT STAGE): the battle start jingle, the stage (its theme
 * follows the jingle), the battle screen. Runs as a deferred action (the UI is rebuilt).
 */
void UIManager::start_battle(int stageUid)
{
    auto stage = m_stage.lock();
    if (!stage || !m_dataLoader->get_stage_data(stageUid)) return;

    // The map opens on this stage next time (until it's won for the first time)
    if (auto player = m_dataLoader->get_player_data().lock())
        player->lastStage = stageUid;
    if (m_setGameSpeedCallback) m_setGameSpeedCallback(GameSpeedState::NORMAL);
    if (m_audio)
    {
        // The version ending on the tonic of the theme that follows it
        const std::string keyed = "jingle_battle_start_" + m_audio->get_music_key(m_dataLoader->get_stage_data(stageUid)->music);
        m_audio->play_jingle(m_audio->has_music(keyed) ? keyed : "jingle_battle_start");
    }
    stage->load(stageUid);   // Unloads the previous battle if any
    generate_battle_uis();
}

/*
 * A base just fell: the battle HUD goes (nothing to click while the end plays) and the speed is normal
 */
void UIManager::begin_battle_end()
{
    m_uiElements.clear();
    if (m_setGameSpeedCallback) m_setGameSpeedCallback(GameSpeedState::NORMAL);
}

/*
 * The stage after this one in its chapter (-1 if it's the last one)
 */
int UIManager::next_stage_uid(int stageUid) const
{
    auto player = m_dataLoader->get_player_data().lock();
    if (!player) return -1;
    auto chapter = m_dataLoader->get_chapter_data(player->currentChapter);
    if (!chapter) return -1;
    for (size_t i = 0; i + 1 < chapter->stages.size(); ++i)
        if (chapter->stages[i].UID == stageUid) return chapter->stages[i + 1].UID;
    return -1;
}

/*
 * Victory: the results panel over the frozen battlefield (shells counting up, first clear, new fish),
 * NEXT STAGE (or the map after a chapter's last stage), RETRY, and back to the Fish Tank
 */
void UIManager::generate_victory_uis()
{
    m_uiElements.clear();
    auto stage = m_stage.lock();
    if (!stage) return;

    const BattleResult result = stage->get_battle_result();
    std::string stageName;
    if (auto stageData = m_dataLoader->get_stage_data(result.stageUID)) stageName = stageData->name;

    const int nextStage = next_stage_uid(result.stageUID);
    UIResultsPanel::Actions actions;
    if (nextStage >= 0)
    {
        actions.primaryLabel = "NEXT STAGE";
        actions.primary = [this, nextStage]() { m_pendingAction = [this, nextStage]() { start_battle(nextStage); }; };
    }
    else
    {
        actions.primaryLabel = "MAP";
        actions.primary = [this]() { m_pendingAction = [this]() { leave_battle(); generate_chapter_map_uis(); }; };
    }
    actions.secondaryLabel = "RETRY";
    actions.secondary = [this, result]() { m_pendingAction = [this, result]() { start_battle(result.stageUID); }; };
    actions.retry = actions.secondary;
    actions.back = [this]() { m_pendingAction = [this]() { leave_battle(); generate_fish_tank_uis(); }; };

    add_ui_element(std::make_shared<UIResultsPanel>(result, stageName, m_dataLoader, actions));
}

/*
 * Defeat: the results panel (how much of the enemy base was left, a tip), RETRY, UPGRADE, back to the map
 */
void UIManager::generate_defeat_uis()
{
    m_uiElements.clear();
    auto stage = m_stage.lock();
    if (!stage) return;

    const BattleResult result = stage->get_battle_result();
    std::string stageName;
    if (auto stageData = m_dataLoader->get_stage_data(result.stageUID)) stageName = stageData->name;

    UIResultsPanel::Actions actions;
    actions.primaryLabel = "RETRY";
    actions.primary = [this, result]() { m_pendingAction = [this, result]() { start_battle(result.stageUID); }; };
    actions.retry = actions.primary;
    actions.secondaryLabel = "UPGRADE";
    actions.secondary = [this]() { m_pendingAction = [this]() { leave_battle(); generate_upgrade_menu_uis(); }; };
    actions.back = [this]() { m_pendingAction = [this]() { leave_battle(); generate_chapter_map_uis(); }; };

    add_ui_element(std::make_shared<UIResultsPanel>(result, stageName, m_dataLoader, actions));
}

/*
 * Unloads the battle before going back to the menus
 */
void UIManager::leave_battle()
{
    if (auto stage = m_stage.lock()) stage->unload();
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

void UIManager::set_fullscreen_callback(std::function<void(bool)> callback)
{
    m_fullscreenCallback = callback;
}

/*
 * "Stage 5 - Clam Leader": the stage the chapter map opens on (the last one played, else the first not cleared)
 */
std::string UIManager::next_stage_label() const
{
    auto player = m_dataLoader->get_player_data().lock();
    if (!player) return "";
    auto chapter = m_dataLoader->get_chapter_data(player->currentChapter);
    if (!chapter || chapter->stages.empty()) return "";

    int index = -1;
    for (size_t i = 0; i < chapter->stages.size() && player->lastStage >= 0; ++i)
        if (chapter->stages[i].UID == player->lastStage) index = static_cast<int>(i);
    for (size_t i = 0; i < chapter->stages.size() && index < 0; ++i)
        if (!player->has_completed_stage(chapter->stages[i].UID)) index = static_cast<int>(i);
    if (index < 0) index = static_cast<int>(chapter->stages.size()) - 1;

    auto stage = m_dataLoader->get_stage_data(chapter->stages[index].UID);
    return "Stage " + std::to_string(index + 1) + (stage ? "  -  " + stage->name : "");
}

/*
 * A fish waiting to be unlocked, or one the shells can upgrade
 */
bool UIManager::is_upgrade_available() const
{
    auto player = m_dataLoader->get_player_data().lock();
    if (!player) return false;
    if (!player->unitsWaitingToBeUnlocked.empty()) return true;
    for (const auto& [uid, unit] : player->ownedUnits)
        if (unit.level < UIUpgradeIcon::MAX_LEVEL && player->shells >= UIUpgradeSlider::calculate_upgrade_cost(*m_dataLoader, uid, unit.level))
            return true;
    return false;
}

/*
 * The Options window: audio, display, controls (UIOptionsMenu)
 */
std::shared_ptr<UIOptionsMenu> UIManager::make_options_menu()
{
    return std::make_shared<UIOptionsMenu>(m_dataLoader, m_audio, [this](bool fullscreen)
        {
            if (m_fullscreenCallback) m_fullscreenCallback(fullscreen);
        });
}

std::shared_ptr<UIButtonElement> UIManager::make_options_button(const std::shared_ptr<UIOptionsMenu>& optionsMenu,
    sf::Vector2f size, sf::Vector2f position, unsigned int textSize)
{
    auto button = std::make_shared<UIButtonElement>(size, position, "OPTIONS");
    button->set_text_size(textSize);
    std::weak_ptr<UIOptionsMenu> weakMenu = optionsMenu;
    button->set_callback([weakMenu]() { if (auto menu = weakMenu.lock()) menu->open(); });
    return button;
}

#include "UIManager.h"

#include "../core/stages/Stage.h"
#include "../core/Game.h"

#include "widgets/UIDeploymentIcons.h"
#include "widgets/UIStageCash.h"
#include "widgets/UICashUp.h"
#include "widgets/UIBaseHealthInfo.h"
#include "widgets/UIFishTank.h"
#include "widgets/UIFishParade.h"
#include "widgets/UITutorial.h"
#include "widgets/UIBattleIntro.h"
#include "widgets/UIClamRoom.h"
#include "widgets/UIClamStorage.h"
#include "widgets/UIFlyingIcons.h"
#include "widgets/UISchoolRank.h"
#include "widgets/UIToast.h"
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
#include "widgets/UIAmbience.h"
#include "widgets/UIHubMascot.h"
#include "widgets/UIInfoButton.h"
#include "widgets/UIInfoPanel.h"
#include "widgets/UIIconTile.h"
#include "widgets/UIClamBackdrop.h"
#include "widgets/UIAdOverlay.h"
#include "widgets/UIChartBackground.h"
#include "UISkin.h"
#include "../core/Platform.h"

#include "../audio/AudioManager.h"

#include <algorithm>
#include <iostream>
#include <random>

namespace
{
    const std::string MENU_MUSIC = "theme_base";     // The Fish Tank menus
    const std::string CLAM_MUSIC = "theme_clam";     // The Clam Room: the Fish Tank theme gone magic, on the same grid
    const std::string CLAM_INSIDE_MUSIC = "theme_clam_inside";   // The Clam Storage: the Clam Room's, muffled and calmer
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

namespace
{
    const float HOLD_UNTIL_REPLACED = 3600.f;   // A battle start's cover: never sweeps away by itself
    const float REVEAL_SECONDS = 0.6f;     // The wave's sweep up the screen
    const float REVEAL_BAND = 300.f;       // Its height

    // How far the wave has gone (0 to 1) after t of the sweep: slow, fast, slow
    float reveal_ease(float t) { return 0.5f - 0.5f * std::cos(3.14159265f * std::clamp(t, 0.f, 1.f)); }
}

/*
 * A new menu screen comes in with a wave: the screen being left is kept (drawn above the wave, no longer updated)
 * while the new one is built, and appears under the wave as it sweeps up
 */
void UIManager::start_screen_reveal()
{
    m_oldElements = std::move(m_uiElements);
    m_uiElements.clear();
    m_revealTime = 0.f;
    m_needsEntrance = true;
}

void UIManager::cancel_screen_reveal()
{
    m_oldElements.clear();
    m_revealTime = -1.f;
}

// From fully under the screen (the old screen) to fully over it (the new one)
std::pair<float, float> UIManager::reveal_band() const
{
    const float travel = 1080.f + REVEAL_BAND + 2.f * UISkin::WATER_WAVE;
    const float top = 1080.f + UISkin::WATER_WAVE - travel * reveal_ease(m_revealTime / REVEAL_SECONDS);
    return { top, top + REVEAL_BAND };
}

/*
 * Clears the screen for a Fish Tank menu: the Fish Tank theme (it keeps playing from one menu to the next)
 * and a swish of water between the screens (the wave)
 */
void UIManager::enter_menu_screen()
{
    start_screen_reveal();
    m_hasSideFrame = true;
    if (m_audio)
    {
        // Back from the Clam Room, its theme melts into the Fish Tank's where it is (both share their grid)
        if (m_audio->get_current_music() == CLAM_MUSIC || m_audio->get_current_music() == CLAM_INSIDE_MUSIC) m_audio->play_music_in_sync(MENU_MUSIC);
        else m_audio->play_music(MENU_MUSIC);
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
        "assets/images/textures/backgrounds/menus/title.png"   // The Fish Tank room, calm (no porthole: the logo stands on it)
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
    startButton->set_attention(true);
    m_needsEntrance = true;   // The buttons pop in (no bubbles: the game just opened)

    auto quitButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ 240.f, 90.f },
        sf::Vector2f{ 30.f, 1080.f - 30.f - 90.f },
        "QUIT"
    );
    quitButton->set_text_size(38);
    quitButton->set_fill_color(sf::Color(255, 150, 130));
    auto quitDialog = make_quit_dialog(quitButton);

    auto optionsMenu = make_options_menu();
    quitButton->set_visible(!Platform::IS_MOBILE);   // Phones don't quit apps (the back key leaves)
    auto optionsButton = make_options_button(optionsMenu, { 280.f, 90.f }, { Platform::IS_MOBILE ? 30.f : 290.f, 1080.f - 30.f - 90.f }, 38);
    optionsButton->set_style(UIButtonElement::Style::Secondary);   // START is the only yellow one

    add_ui_element(bg);
    add_ui_element(std::make_shared<UIFishParade>(m_dataLoader));   // The cast marches along the floor
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
    // A screen just built: each button pops in as the wave uncovers it (no wave, the title: one after the other)
    if (m_needsEntrance)
    {
        m_needsEntrance = false;
        float delay = 0.12f;
        for (auto& element : m_uiElements)
            if (auto button = std::dynamic_pointer_cast<UIButtonElement>(element); button && button->is_visible())
            {
                if (m_revealTime >= 0.f)
                {
                    // When the wave's bottom passes the button's middle (inverse of reveal_ease)
                    const sf::FloatRect bounds = button->get_bounds();
                    const float middle = bounds.position.y + bounds.size.y / 2.f;
                    const float travel = 1080.f + REVEAL_BAND + 2.f * UISkin::WATER_WAVE;
                    const float progress = std::clamp((1080.f + UISkin::WATER_WAVE + REVEAL_BAND - middle) / travel, 0.f, 1.f);
                    button->set_entrance(std::acos(1.f - 2.f * progress) / 3.14159265f * REVEAL_SECONDS - m_revealTime - 0.03f);
                }
                else
                {
                    button->set_entrance(delay);
                    delay += 0.045f;
                }
            }
    }
    if (m_revealTime >= 0.f)
    {
        m_revealTime += deltaTime;
        if (m_revealTime >= REVEAL_SECONDS) cancel_screen_reveal();   // The wave is gone, and the old screen with it
    }

    for (auto& element : m_uiElements)
    {
        element->update(deltaTime);
    }

    // A battle starting: once the water covers the screen and the background has been read, load the rest
    auto loadingStage = m_stage.lock();
    if (m_pendingBattleStage >= 0 && m_battleCover && m_battleCover->has_covered_screen() && (!loadingStage || loadingStage->is_prepared()))
        finish_battle_start();

    // An action an animation asked for when it ended (the zoom into the Clam Storage): after the loop, since it can
    // change the screen
    if (m_pendingAction)
    {
        auto action = m_pendingAction;
        m_pendingAction = nullptr;
        action();
    }
}

// Render all UI elements
// Handles world-space vs screen-space rendering by switching the view as needed
void UIManager::render_uis(sf::RenderWindow& window, sf::View& uiView, sf::View& worldView)
{
    if (m_revealTime < 0.f)
    {
        render_list(window, m_uiElements, uiView, worldView, m_hasSideFrame);
        return;
    }

    // The wave sweeping up: the old screen above it, the new one below it (each clipped to its part of the window
    // with a scissor rectangle, down to the wave's lowest dips), the water over both
    const auto [top, bottom] = reveal_band();
    const float height = static_cast<float>(std::max(1u, window.getSize().y));
    const auto row = [&](float y) { return std::clamp(static_cast<float>(window.mapCoordsToPixel({ 0.f, y }, uiView).y) / height, 0.f, 1.f); };
    const float oldEnd = row(top + UISkin::WATER_WAVE), newStart = row(bottom - UISkin::WATER_WAVE);
    if (oldEnd > 0.001f)
    {
        sf::View above = uiView;
        above.setScissor({ { 0.f, 0.f }, { 1.f, oldEnd } });
        render_list(window, m_oldElements, above, worldView, false);
    }
    if (newStart < 0.999f)
    {
        sf::View below = uiView;
        below.setScissor({ { 0.f, newStart }, { 1.f, 1.f - newStart } });
        render_list(window, m_uiElements, below, worldView, false);
    }
    window.setView(uiView);
    UISkin::draw_water_band(window, top, bottom, m_revealTime);
    if (m_hasSideFrame) UISkin::draw_side_frame(window);   // The frame stays still while the wave passes
}

// Draws elements in order: the world-space ones in the world view, the others in uiView (which can carry a scissor)
void UIManager::render_list(sf::RenderWindow& window, std::vector<std::shared_ptr<UIElement>>& elements, const sf::View& uiView, sf::View& worldView,
    bool withFrame)
{
    window.setView(uiView);
    bool isFramed = !withFrame;   // The side frame: over the screen, under an open window (dimmed with the rest)
    for (auto& element : elements)
    {
        if (!isFramed && !element->isWorldSpaceUi && element->is_visible() && element->is_blocking_input())
        {
            UISkin::draw_side_frame(window);
            isFramed = true;
        }
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
    if (!isFramed) UISkin::draw_side_frame(window);
}

/*
 * Darkens what the screen shows outside the 1920 x 1080 layout (a phone's sides, a tablet's top and bottom),
 * like the modal windows darken the layout itself
 */
void UIManager::dim_letterbox_bars(sf::RenderWindow& window, const sf::View& uiView) const
{
    const sf::FloatRect screen = UISkin::screen_rect();
    const float right = screen.position.x + screen.size.x, bottom = screen.position.y + screen.size.y;
    window.setView(uiView);
    sf::RectangleShape bar;
    bar.setFillColor(sf::Color(0, 0, 0, 150));
    const sf::FloatRect bars[] = {
        { screen.position, { -screen.position.x, screen.size.y } },                   // Left
        { { 1920.f, screen.position.y }, { right - 1920.f, screen.size.y } },          // Right
        { { 0.f, screen.position.y }, { 1920.f, -screen.position.y } },               // Top
        { { 0.f, 1080.f }, { 1920.f, bottom - 1080.f } }                              // Bottom
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
    if (m_pendingBattleStage >= 0) return;   // A battle is starting: the screen under the water takes no input
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
    battleButton->set_attention(true);
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

    // The main actions stay big text buttons (BATTLE! above, yellow): UPGRADE and EQUIP
    const float stackHeight = 104.f;
    auto upgradeButton = std::make_shared<UIButtonElement>(sf::Vector2f{ columnWidth, stackHeight }, sf::Vector2f{ columnX, 366.f }, "UPGRADE");
    upgradeButton->set_text_size(50);
    upgradeButton->set_style(UIButtonElement::Style::Secondary);
    upgradeButton->set_callback([this]() { m_pendingAction = [this]() { generate_upgrade_menu_uis(); }; });

    auto equipButton = std::make_shared<UIButtonElement>(sf::Vector2f{ columnWidth, stackHeight }, sf::Vector2f{ columnX, 494.f }, "EQUIP");
    equipButton->set_text_size(50);
    equipButton->set_style(UIButtonElement::Style::Secondary);
    equipButton->set_callback([this]() { m_pendingAction = [this]() { generate_equip_menu_uis(); }; });

    // The rest are icon tiles in a row: the encyclopedia, the options, and on PC quit (phones don't quit apps).
    // Two tiles share the row on a phone: wider, easier to tap
    const int tileCount = Platform::IS_MOBILE ? 2 : 3;
    const float tileGap = 25.f, tileY = 628.f, tileHeight = 170.f;
    const float tileWidth = (columnWidth - tileGap * static_cast<float>(tileCount - 1)) / static_cast<float>(tileCount);
    const auto tile_x = [&](int index) { return columnX + static_cast<float>(index) * (tileWidth + tileGap); };

    auto encyclopediaButton = std::make_shared<UIIconTile>(sf::Vector2f{ tileWidth, tileHeight }, sf::Vector2f{ tile_x(0), tileY },
        "ENCYCLOPEDIA", UIIconTile::Icon::BOOK);
    encyclopediaButton->set_callback([this]() { m_pendingAction = [this]() { generate_encyclopedia_uis(); }; });

    auto optionsMenu = make_options_menu();
    auto optionsButton = std::make_shared<UIIconTile>(sf::Vector2f{ tileWidth, tileHeight }, sf::Vector2f{ tile_x(1), tileY },
        "OPTIONS", UIIconTile::Icon::GEAR);
    std::weak_ptr<UIOptionsMenu> weakOptions = optionsMenu;
    optionsButton->set_callback([weakOptions]() { if (auto menu = weakOptions.lock()) menu->open(); });

    // "!" on UPGRADE when a fish can be upgraded or unlocked with the shells there are
    auto upgradeBadge = std::make_shared<UIBadge>(sf::Vector2f{ columnX + columnWidth - 14.f, 366.f + 12.f }, "!");
    upgradeBadge->set_visible(is_upgrade_available());

    auto quitButton = std::make_shared<UIIconTile>(sf::Vector2f{ tileWidth, tileHeight }, sf::Vector2f{ tile_x(2), tileY },
        "QUIT", UIIconTile::Icon::POWER);
    quitButton->set_visible(!Platform::IS_MOBILE);
    auto quitDialog = make_quit_dialog(quitButton);

    add_ui_element(bg);
    add_ui_element(std::make_shared<UIAmbience>());
    add_ui_element(std::make_shared<UIFishTank>(m_dataLoader));   // The lineup swims in the porthole
    add_ui_element(titleBar);
    add_ui_element(shellsInfo);
    add_ui_element(battleButton);
    add_ui_element(upgradeButton);
    add_ui_element(equipButton);
    add_ui_element(encyclopediaButton);
    add_ui_element(nextStage);
    add_ui_element(upgradeBadge);

    // The Clam Room: a tile with the clam under the porthole. Locked (greyed, the stage that opens it written under
    // it) until the stage with the "clamRoom" reward is cleared
    const bool isClamRoomOpen = m_dataLoader->is_clam_room_open();
    const sf::FloatRect clamTile{ { 1388.f - 120.f, 826.f }, { 240.f, 168.f } };
    auto clamButton = std::make_shared<UIIconTile>(clamTile.size, clamTile.position, "CLAM ROOM", UIIconTile::Icon::TEXTURE,
        "assets/images/textures/ui/clam_closed.png");
    clamButton->set_enabled(isClamRoomOpen);
    clamButton->set_callback([this]() { m_pendingAction = [this]() { generate_clam_room_uis(); }; });
    add_ui_element(clamButton);
    if (!isClamRoomOpen)
    {
        auto stage = m_dataLoader->get_stage_data(m_dataLoader->get_clam_room_stage());
        auto lockLabel = std::make_shared<UITextElement>(sf::Vector2f{ 400.f, 30.f }, sf::Vector2f{ 0.f, 0.f },
            stage ? "Clear \"" + stage->name + "\" to open" : "Locked", 26);
        UISkin::style_text(lockLabel->get_text(), 26);
        UISkin::set_text_anchor(lockLabel->get_text(), { 0.5f, 0.5f });
        lockLabel->set_position({ clamTile.position.x + clamTile.size.x / 2.f, clamTile.position.y + clamTile.size.y + 30.f });
        add_ui_element(lockLabel);
    }
    else if (auto player = m_dataLoader->get_player_data().lock(); player && (player->shinyScales > 0 || !player->clamStorage.empty()))
        add_ui_element(std::make_shared<UIBadge>(sf::Vector2f{ clamTile.position.x + clamTile.size.x - 36.f, clamTile.position.y + 36.f }, "!"));   // On the clam's shoulder

    // The School Rank (top right) with its "i", and its milestones reached since last time
    add_ui_element(std::make_shared<UISchoolRank>(m_dataLoader));
    auto rankInfo = add_info_button({ 1330.f - 40.f, 170.f }, "School Rank",
        "Your school's strength: every fish's level and extra levels, added up. It goes up each time you upgrade a fish "
        "or use a fish from the Clam Storage.\n\n"
        "Milestones give Shiny Scales, to open the giant clam: 1 at ranks 10, 25 and 50, 7 at rank 77, 1 at 100 and 125, "
        "then 1 more every 25 ranks. The bar shows how far the next one is.");
    if (auto player = m_dataLoader->get_player_data().lock())
    {
        const int before = player->shinyScales;
        UIToast::announce_school_rank_rewards(*player);
        if (player->shinyScales != before) m_dataLoader->save_progress();
    }
    add_ui_element(std::make_shared<UIHubMascot>(m_dataLoader, hub_tips()));   // The Fish, by the floor, with tips
    add_ui_element(optionsButton);
    add_ui_element(quitButton);
    add_ui_element(std::make_shared<UIToast>());
    add_ui_element(optionsMenu);  // Windows last: on top while open
    add_ui_element(quitDialog);
    add_ui_element(rankInfo);
}

/*
 * Generate the Encyclopedia: every fish and every enemy (black until owned / met), their stats and abilities
 */
/*
 * The Clam Room's and the Clam Storage's title bar: the shells, the Shiny Scales, the Fish Scales, and the "+"
 * between them that crafts a Shiny Scale from 10 Fish Scales (it asks first, and says how to get Fish Scales).
 * The craft dialog is returned: add it last (it's a window)
 */
UIManager::ClamCounters UIManager::add_clam_counters()
{
    ClamCounters counters;
    counters.shells = std::make_shared<UIPlayerShellsInfo>(m_dataLoader);
    counters.shinyScales = std::make_shared<UIPlayerShellsInfo>(m_dataLoader, UIPlayerShellsInfo::Currency::SHINY_SCALES);
    counters.fishScales = std::make_shared<UIPlayerShellsInfo>(m_dataLoader, UIPlayerShellsInfo::Currency::FISH_SCALES);
    add_ui_element(counters.shells);
    add_ui_element(counters.shinyScales);
    add_ui_element(counters.fishScales);

    auto craftDialog = std::make_shared<UIConfirmDialog>("", "", "CRAFT", "CANCEL");
    craftDialog->set_confirm_tint(sf::Color::White);   // A good action: yellow
    craftDialog->set_confirm_sound("");
    counters.craftDialog = craftDialog;

    // Inside the right end of the Shiny Scales pill (its right edge stays at 1920 - 364 whatever its width)
    counters.shinyScales->set_trailing_room(58.f);
    auto plus = std::make_shared<UIButtonElement>(sf::Vector2f{ 50.f, 50.f }, sf::Vector2f{ 1920.f - 364.f - 12.f - 50.f, 30.f }, "+");
    plus->set_text_size(40);
    std::weak_ptr<UIConfirmDialog> weakDialog = craftDialog;
    plus->set_callback([this, weakDialog]()
        {
            auto dialog = weakDialog.lock();
            auto player = m_dataLoader->get_player_data().lock();
            if (!dialog || !player) return;
            const int need = PlayerData::FISH_SCALES_PER_SHINY;
            dialog->set_texts("Craft a Shiny Scale?", std::to_string(need) + " Fish Scales make one (you have " + std::to_string(player->fishScales)
                + "). Sell fish in the Clam Storage for more.", "CRAFT");
            dialog->open([this]()
                {
                    auto player = m_dataLoader->get_player_data().lock();
                    if (!player || !player->craft_shiny_scale()) { if (m_audio) m_audio->play_sound("ui_error"); return; }
                    if (m_audio) m_audio->play_sound("purchase");
                    m_dataLoader->save_progress();
                });
        });
    add_ui_element(plus);
    return counters;
}

/*
 * The Clam Room: the dim purple room, the giant clam (UIClamRoom), the shells, the Shiny Scales and the Fish Scales
 * (with the craft "+"), STORAGE (dives into the clam: the Clam Storage), back to the Fish Tank. Its theme fades in
 * over the Fish Tank's (or the Clam Storage's) at the same place in the music. fromStorage: the view zooms out of
 * the open clam first. The first visit gives one Shiny Scale for a first roll
 */
void UIManager::generate_clam_room_uis(bool fromStorage)
{
    if (!fromStorage) start_screen_reveal();   // From the storage, the view zooms out of the clam instead
    else cancel_screen_reveal();
    m_uiElements.clear();
    m_hasSideFrame = true;
    if (m_audio)
    {
        m_audio->play_music_in_sync(CLAM_MUSIC);
        if (!fromStorage) m_audio->play_sound("ui_transition");
    }
    if (auto player = m_dataLoader->get_player_data().lock(); player && player->give_clam_room_gift())
    {
        UIToast::show("A WELCOME GIFT!", "1 Shiny Scale for your first clam", "icon_shiny_scale");
        m_dataLoader->save_progress();
    }

    auto bg = std::make_shared<UIClamBackdrop>();   // Plain: the clam is the show
    auto room = std::make_shared<UIClamRoom>(m_dataLoader);
    room->set_on_storage([this]() { m_pendingAction = [this]() { generate_clam_storage_uis(); }; });
    if (fromStorage) room->enter_from_storage();
    std::weak_ptr<UIClamRoom> weakRoom = room;
    auto backButton = make_back_button([this, weakRoom]()
        {
            if (auto room = weakRoom.lock(); room && room->is_busy()) return;   // Not in the middle of an opening
            generate_fish_tank_uis();
        });

    add_ui_element(bg);
    add_ui_element(room);
    auto clamTitle = std::make_shared<UITitleBar>("Clam Room", UITitleBar::Material::VIOLET);
    add_ui_element(clamTitle);
    auto clamInfo = add_info_button(clamTitle->get_info_position(), "The giant clam",
        "Feed the clam Shiny Scales: each one gives a random fish. OPEN uses 1 scale, OPEN x10 uses 10.\n\nEvery fish goes to the Clam Storage (the chest): use it for a fish you don't have yet, or to give a fish you have an extra level; or sell it.\n\nShiny Scales come from School Rank milestones. 10 Fish Scales make one: tap the + in the Shiny Scales counter.");
    const ClamCounters counters = add_clam_counters();
    room->set_scales_counter(counters.shinyScales);   // The scales fly out of it
    for (const auto& pill : { counters.shells, counters.shinyScales, counters.fishScales }) pill->set_tint(sf::Color(206, 186, 255));   // Violet room

    // Phones: the FREE offer plays a rewarded ad (UIAdOverlay), then gives a Shiny Scale (a few a day)
    auto adOverlay = std::make_shared<UIAdOverlay>();
    std::weak_ptr<UIAdOverlay> weakAd = adOverlay;
    room->set_on_watch_ad([this, weakAd]()
        {
            auto overlay = weakAd.lock();
            if (!overlay || overlay->is_playing()) return;
            overlay->play([this]()
                {
                    auto player = m_dataLoader->get_player_data().lock();
                    if (!player || !player->watch_ad()) return;
                    m_dataLoader->save_progress();
                    if (m_audio) m_audio->play_sound("purchase");
                    UIToast::show("+1 SHINY SCALE", "Thanks for watching!", "icon_shiny_scale");
                });
        });
    add_ui_element(std::make_shared<UIClamRoomOverlay>(room));   // What flies over the title bar
    add_ui_element(backButton);
    add_ui_element(std::make_shared<UIToast>());
    add_ui_element(counters.craftDialog);
    add_ui_element(clamInfo);
    add_ui_element(adOverlay);   // Over everything while an ad plays
}

/*
 * The Clam Storage, inside the clam (dark pink, UIClamStorage draws it all): the fish out of the clam, picked to be
 * used or sold, the counters they fly into, back to the Clam Room (out of the clam). Its theme is the Clam Room's
 * muffled and calmer, swapped at the same place in the music
 */
void UIManager::generate_clam_storage_uis()
{
    cancel_screen_reveal();   // The dive into the clam is its transition
    m_hasSideFrame = true;
    m_uiElements.clear();
    if (m_audio) m_audio->play_music_in_sync(CLAM_INSIDE_MUSIC);

    auto storage = std::make_shared<UIClamStorage>(m_dataLoader);
    auto flying = std::make_shared<UIFlyingIcons>();
    auto sellDialog = std::make_shared<UIConfirmDialog>("", "", "SELL", "CANCEL");
    sellDialog->set_confirm_sound("");
    std::weak_ptr<UIFlyingIcons> weakFlying = flying;
    auto backButton = make_back_button([this, weakFlying]()
        {
            if (auto flying = weakFlying.lock()) flying->land_all();
            generate_clam_room_uis(true);
        });

    add_ui_element(storage);
    auto storageTitle = std::make_shared<UITitleBar>("Clam Storage", UITitleBar::Material::NACRE);   // Inside the clam
    add_ui_element(storageTitle);
    auto storageInfo = add_info_button(storageTitle->get_info_position(), "The Clam Storage",
        "The fish out of the clam wait here. Tap one to pick it; tap others of the same kind to add them, or tick Select all.\n\nUSE: a fish you don't have yet joins your tank, and every other one gives that fish +1 extra level, up to +10.\n\nSELL: for shells, or for 1 Fish Scale each. 10 Fish Scales make a Shiny Scale (the + in its counter).");
    const ClamCounters counters = add_clam_counters();
    for (const auto& pill : { counters.shells, counters.shinyScales, counters.fishScales }) pill->set_tint(sf::Color(255, 168, 214));   // Plum, in the clam
    storage->set_counters(counters.shells, counters.fishScales);
    storage->set_flying_icons(flying);
    storage->set_confirm_dialog(sellDialog);
    add_ui_element(flying);   // Over the title bar: into the pills
    add_ui_element(backButton);
    add_ui_element(std::make_shared<UIToast>());
    add_ui_element(sellDialog);
    add_ui_element(counters.craftDialog);
    add_ui_element(storageInfo);
}

void UIManager::generate_encyclopedia_uis()
{
    enter_menu_screen();

    auto bg = std::make_shared<UIChartBackground>();   // The field guide's own place: a research chart
    auto encyclopedia = std::make_shared<UIEncyclopedia>(m_dataLoader);
    auto titleBar = std::make_shared<UITitleBar>("Encyclopedia");
    auto shellsInfo = std::make_shared<UIPlayerShellsInfo>(m_dataLoader);
    auto backButton = make_back_button([this]() { generate_fish_tank_uis(); });

    add_ui_element(bg);
    add_ui_element(std::make_shared<UIAmbience>());
    add_ui_element(titleBar);
    add_ui_element(shellsInfo);
    add_ui_element(backButton);
    auto info = add_info_button(titleBar->get_info_position(), "Encyclopedia",
        "Every fish and every enemy of the game. The ones you haven't got or met yet are black shapes.\n\nPick one to see its stats and its abilities; the buttons on top sort them.");
    add_ui_element(encyclopedia);   // Its ability bubbles go over everything (but its "i" window)
    add_ui_element(info);
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
    add_ui_element(std::make_shared<UIAmbience>());
    add_ui_element(upgradeSlider);
    add_ui_element(titleBar);
    add_ui_element(shellsInfo);
    add_ui_element(backButton);
    add_ui_element(std::make_shared<UIToast>());   // School Rank milestones reached by upgrading
    add_ui_element(add_info_button(titleBar->get_info_position(), "Upgrading",
        "Spend shells to raise a fish's level: more health and more attack. Each level costs more than the last; the price is on the card. A fish can reach level 30.\n\nExtra levels (the purple +1, +2...) come from the Clam Room: using a fish you already have from the Clam Storage gives it +1, up to +10, on top of its level.\n\nNew fish won in battle wait here to be unlocked."));
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
    add_ui_element(std::make_shared<UIAmbience>());
    add_ui_element(equipSlider);
    add_ui_element(titleBar);
    add_ui_element(shellsInfo);
    add_ui_element(backButton);
    add_ui_element(add_info_button(titleBar->get_info_position(), "Your lineup",
        "Your lineup: the fish you can send into battle, up to 10.\n\nTap a fish (or drag it) to add it. Drag the fish in the lineup to change their order: their slot is their number key in battle. Drag one out of the lineup to remove it (right click works too on PC).\n\nThe price on each fish is what sending it costs in battle."));
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
    cancel_screen_reveal();   // UIBattleIntro is its transition
    m_hasSideFrame = false;    // The battlefield fills the screen
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
    speedButton->set_state_label([this]() {
        return m_getGameSpeedCallback && m_getGameSpeedCallback() == GameSpeedState::FASTER ? std::string("x2") : std::string("x1");
        });

	auto deploymentIcons = std::make_shared<UIDeploymentIcons>(m_dataLoader, m_stage.lock());
	auto stageCash = std::make_shared<UIStageCash>(m_stage.lock());
	auto cashUp = std::make_shared<UICashUp>(m_stage.lock());
	auto baseHealthInfo = std::make_shared<UIBaseHealthInfo>(m_stage.lock());
	auto statusIcons = std::make_shared<UIStatusIcons>(m_stage.lock(), m_dataLoader); // Status icons under every entity
	if (auto stage = m_stage.lock())
	{
		// The stage draws them at each entity's layer (the widget may go first: weak)
		std::weak_ptr<UIStatusIcons> weakIcons = statusIcons;
		stage->set_entity_overlay([weakIcons](sf::RenderWindow& window, const BattleEntity& entity, bool isUnit)
			{
				if (auto icons = weakIcons.lock()) icons->draw_row(window, entity, isUnit);
			});
	}
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
    if (auto stage = m_stage.lock(); stage && UITutorial::is_wanted(*m_dataLoader, stage->get_current_uid()))
        add_ui_element(std::make_shared<UITutorial>(stage, cashUp));   // The first battle's hints, over the HUD
    if (m_battleIntroSeconds > 0.f)   // GET READY FOR BATTLE! while the jingle plays (once per battle start)
        add_ui_element(std::make_shared<UIBattleIntro>(stageName, m_battleIntroSeconds,
            std::max(UIBattleIntro::COVER_SECONDS, m_jingleClock.getElapsedTime().asSeconds())));   // Where the jingle is
    m_battleIntroSeconds = 0.f;
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
    battleButton->set_attention(true);
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
    add_ui_element(add_info_button(titleBar->get_info_position(), "The chapter map",
        "Pick a stage on the map, then BATTLE!.\n\nCleared stages are green, the next one is yellow; the grey ones open as you clear the stages before them. The panel shows the enemy base's health, the shells a win brings, and the enemies you'll meet."));
}

/*
 * Starts a battle (from the map, RETRY or NEXT STAGE): the battle start jingle, the stage (its theme
 * follows the jingle), the battle screen. Runs as a deferred action (the UI is rebuilt).
 */
void UIManager::start_battle(int stageUid)
{
    auto stage = m_stage.lock();
    if (!stage || !m_dataLoader->get_stage_data(stageUid) || m_pendingBattleStage >= 0) return;

    // The map opens on this stage next time (until it's won for the first time)
    if (auto player = m_dataLoader->get_player_data().lock())
        player->lastStage = stageUid;
    if (m_setGameSpeedCallback) m_setGameSpeedCallback(GameSpeedState::NORMAL);
    m_battleIntroSeconds = 2.4f;
    if (m_audio)
    {
        // The version ending on the tonic of the theme that follows it
        const std::string keyed = "jingle_battle_start_" + m_audio->get_music_key(m_dataLoader->get_stage_data(stageUid)->music);
        const std::string jingle = m_audio->has_music(keyed) ? keyed : "jingle_battle_start";
        m_audio->play_jingle(jingle);
        m_battleIntroSeconds = m_audio->get_music_length(jingle);   // The battle start transition lasts as long
    }
    m_jingleClock.restart();

    // With the jingle: the water rises over this screen right away (the stage loads under it, finish_battle_start)
    // It holds covered (its bubbles rising) as long as loading takes: the battle screen's own intro sweeps the water away
    m_battleCover = std::make_shared<UIBattleIntro>(m_dataLoader->get_stage_data(stageUid)->name, HOLD_UNTIL_REPLACED);
    add_ui_element(m_battleCover);
    m_pendingBattleStage = stageUid;
    Platform::log("Battle start: the water rises");
    stage->prepare(stageUid);   // The slow reading happens on a worker thread while the water rises and the bubbles go up
}

/*
 * The screen is covered (the water and the title are on screen): the stage loads (it takes a moment, nothing moves
 * meanwhile but nothing needs to), then the battle screen, whose intro starts at the jingle's current moment
 */
void UIManager::finish_battle_start()
{
    const int stageUid = m_pendingBattleStage;
    m_pendingBattleStage = -1;
    m_battleCover.reset();
    auto stage = m_stage.lock();
    if (!stage) return;
    Platform::log("Battle start: covered and read, loading");
    sf::Clock clock;
    stage->load(stageUid);   // Unloads the previous battle if any
    generate_battle_uis();
    Platform::log("Battle start: loaded in " + std::to_string(clock.getElapsedTime().asMilliseconds()) + " ms");
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
 * Victory: the results panel over the frozen battlefield (shells counting up, first clear, new fish),
 * RETURN TO MAP (it opens on the newest stage after a first clear), RETRY, and back to the Fish Tank
 */
void UIManager::generate_victory_uis()
{
    m_uiElements.clear();
    auto stage = m_stage.lock();
    if (!stage) return;

    const BattleResult result = stage->get_battle_result();
    std::string stageName;
    if (auto stageData = m_dataLoader->get_stage_data(result.stageUID)) stageName = stageData->name;

    // Back to the map rather than straight into the next stage: the player sees the path move on, and can upgrade first
    UIResultsPanel::Actions actions;
    actions.primaryLabel = "RETURN TO MAP";
    actions.primary = [this]() { m_pendingAction = [this]() { leave_battle(); generate_chapter_map_uis(); }; };
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
 * What the Fish says in the Fish Tank: what's worth doing now first, then general tips
 */
std::vector<std::string> UIManager::hub_tips() const
{
    std::vector<std::string> tips;
    auto player = m_dataLoader->get_player_data().lock();
    if (!player) return tips;

    // Short: two or three lines in the bubble
    if (m_dataLoader->is_clam_room_open() && player->shinyScales > 0)
        tips.push_back("The clam looks hungry! We have " + std::to_string(player->shinyScales)
            + (player->shinyScales == 1 ? " Shiny Scale." : " Shiny Scales."));
    int stored = 0;
    for (const auto& [uid, count] : player->clamStorage) stored += count;
    if (stored > 0)
        tips.push_back(std::to_string(stored) + (stored == 1 ? " fish waits" : " fish wait") + " in the Clam Storage!");
    if (is_upgrade_available())
        tips.push_back("We can afford an upgrade. Try UPGRADE!");
    const int rank = player->get_school_rank();
    for (int i = 0; i < 200; ++i)
        if (const auto milestone = PlayerData::school_rank_milestone(i); milestone.power > rank)
        {
            tips.push_back("School Rank " + std::to_string(milestone.power) + " gives " + std::to_string(milestone.shinyScales)
                + (milestone.shinyScales == 1 ? " Shiny Scale!" : " Shiny Scales!"));
            break;
        }
    if (const std::string next = next_stage_label(); !next.empty())
        tips.push_back("Next: " + next + ". Ready?");

    tips.push_back("Tap a fish in the porthole to say hi!");
    tips.push_back("In battle, upgrade your wallet early!");
    tips.push_back("Cheap fish stall, strong fish hit hard.");
    tips.push_back("Lost? Upgrade a few fish and try again!");
    tips.push_back("The Encyclopedia knows every enemy we've met.");
    return tips;
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
 * An "i" that opens a help window (contextual explanations, like a feature's rules): the button is added now, the
 * window is returned to be added last (a window: on top while open)
 */
std::shared_ptr<UIInfoPanel> UIManager::add_info_button(sf::Vector2f center, const std::string& title, const std::string& text)
{
    auto panel = std::make_shared<UIInfoPanel>(title, text);
    auto button = std::make_shared<UIInfoButton>(center);
    std::weak_ptr<UIInfoPanel> weakPanel = panel;
    button->set_callback([weakPanel]() { if (auto panel = weakPanel.lock()) panel->open(); });
    add_ui_element(button);
    return panel;
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

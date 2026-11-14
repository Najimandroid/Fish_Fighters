#pragma once

#include "widgets/UIElement.h"
#include "../core/DataLoader.h"

#include <SFML/Graphics.hpp>
#include <vector>
#include <functional>

class Stage;
class UIRoundButton;
class UIButtonElement;
class UIConfirmDialog;
class UIPlayerShellsInfo;
class UIOptionsMenu;
class UIBattleIntro;
class UIInfoPanel;
class AudioManager;
enum class GameSpeedState;

/*
 * UIManager class
 * ----------------
 * UIManager handles all User Interface (UI) elements in the game.
 * Responsibilities include:
 *   - Initializing UI elements with references to game data and stage
 *   - Updating and rendering all UI elements
 *   - Handling input events (mouse clicks, drags, etc.)
 *   - Generating different UI screens (Fish Tank, Upgrades, Equip, Battle)
 *
 * UI elements can be either:
 *   - Screen-space or
 *   - World-space (attached to the game world, e.g., UIBaseHealthInfo)
 *
 * Example usage:
 *   auto uiManager = std::make_shared<UIManager>();
 *   uiManager->init(dataLoader, stage);
 *   uiManager->generate_fish_tank_uis();
 *   uiManager->update_uis(deltaTime);
 *   uiManager->render_uis(window, uiCamera, worldCamera);
 */

class UIManager
{
public:

    UIManager() = default;

    // Initialize UIManager with game data, stage and audio references (the widgets' sounds go to the audio)
    void init(std::shared_ptr<DataLoader> dataLoader, std::shared_ptr<Stage> stage, std::shared_ptr<AudioManager> audio);

    // Add a single UI element to the manager
    void add_ui_element(std::shared_ptr<UIElement> element);

    // Update all UI elements
    void update_uis(float deltaTime);

    // Render all UI elements, handling screen-space vs world-space rendering
    void render_uis(sf::RenderWindow& window, sf::View& uiView, sf::View& worldView);

    // Handle input events for all UI elements
    void handle_ui_events(const sf::Event& event, const sf::RenderWindow& window);

    // Generate specific UI layouts/screens
    void generate_title_uis();      // Title screen (the title theme), START goes to the Fish Tank
    void generate_fish_tank_uis();
    void generate_upgrade_menu_uis();
    void generate_equip_menu_uis();
    void generate_encyclopedia_uis();
    void generate_clam_room_uis(bool fromStorage = false);   // The gacha: Shiny Scales into the giant clam, fish out
    void generate_clam_storage_uis();   // The fish out of the clam, to use or sell
    void generate_battle_uis();
    void generate_chapter_map_uis();
    void generate_victory_uis();
    void generate_defeat_uis();

    // A base just fell (Stage): the battle HUD goes and the speed is back to normal while the end plays
    void begin_battle_end();

    // Returns true if a given world position overlaps any UI element
    bool is_mouse_over_ui(const sf::Vector2i& worldPosition) const;

	// Callbacks for getting and setting game speed
    void set_game_speed_callback(std::function<void(GameSpeedState)> callback);
    void set_get_game_speed_callback(std::function<GameSpeedState()> callback);
    void set_quit_game_callback(std::function<void()> callback);

    // Switches between fullscreen (true) and a window (Options > Display); the Game applies it
    void set_fullscreen_callback(std::function<void(bool)> callback);

private:
    // The round back button of the menus (bottom left): runs onBack as a deferred action
    std::shared_ptr<UIRoundButton> make_back_button(std::function<void()> onBack);

    // Starts a battle: the jingle plays and the water rises over the screen the player is on at once (UIBattleIntro);
    // the stage loads once a frame with the screen fully covered is up (finish_battle_start), so loading never looks
    // like a frozen game, and the battle's intro carries on from where the jingle is. Run it as a deferred action
    void start_battle(int stageUid);
    void finish_battle_start();
    std::shared_ptr<UIBattleIntro> m_battleCover;   // The water over the screen being left
    int m_pendingBattleStage = -1;                  // The stage loading under it (-1: none)
    sf::Clock m_jingleClock;                        // Since the jingle started

    // Unloads the battle before going back to a menu
    void leave_battle();

    // The hub: the stage the map opens on ("Stage 5 - Clam Leader"), and whether something can be upgraded
    std::string next_stage_label() const;
    bool is_upgrade_available() const;

    // What the Fish (UIHubMascot) says in the Fish Tank: what's worth doing now first, then general tips
    std::vector<std::string> hub_tips() const;


    // Darkens the bars around the 16:9 UI area while a modal window is open (screens that aren't 16:9)
    void dim_letterbox_bars(sf::RenderWindow& window, const sf::View& uiView) const;

    // "Quit Fish Fighters?" dialog that quitButton opens (add it last, so it's on top)
    std::shared_ptr<UIConfirmDialog> make_quit_dialog(const std::shared_ptr<UIButtonElement>& quitButton);

    // The Clam Room's and Clam Storage's counters (shells, Shiny Scales, Fish Scales) and the craft "+": adds them,
    // returns them with the craft dialog (add it last)
    struct ClamCounters
    {
        std::shared_ptr<UIPlayerShellsInfo> shells, shinyScales, fishScales;
        std::shared_ptr<UIConfirmDialog> craftDialog;
    };
    ClamCounters add_clam_counters();

    // An "i" at center that explains something (its window is returned: add it last, it's a window)
    std::shared_ptr<UIInfoPanel> add_info_button(sf::Vector2f center, const std::string& title, const std::string& text);

    // The menus have the side frame on a screen wider than 16:9 (UISkin::draw_side_frame); the title screen and the
    // battle don't
    bool m_hasSideFrame = false;

    // The Options window (add it after the screen's other elements) and a button that opens it
    std::shared_ptr<UIOptionsMenu> make_options_menu();
    std::shared_ptr<UIButtonElement> make_options_button(const std::shared_ptr<UIOptionsMenu>& optionsMenu,
        sf::Vector2f size, sf::Vector2f position, unsigned int textSize);

    // Starts a Fish Tank menu screen: clears the UI, plays the Fish Tank theme (kept if it already plays)
    // and the transition sound
    void enter_menu_screen();

    std::shared_ptr<DataLoader> m_dataLoader = nullptr;         // Reference to game data
    std::weak_ptr<Stage> m_stage;                               // Reference to current game stage
    std::shared_ptr<AudioManager> m_audio;                      // Music and sounds

    std::vector<std::shared_ptr<UIElement>> m_uiElements;       // All active UI elements

    std::function<void()> m_pendingAction;                      // Deferred action triggered by UI

    // A new menu screen comes in with a wave: a band of water (UISkin::draw_water_band) sweeps up the screen, the
    // old screen still drawn above it, the new one below it; each button pops in as the wave uncovers it.
    // start_screen_reveal keeps the old screen's elements (drawn, not updated) until the wave is gone
    std::vector<std::shared_ptr<UIElement>> m_oldElements;
    float m_revealTime = -1.f;                                  // -1: no wave
    bool m_needsEntrance = false;
    void start_screen_reveal();
    void cancel_screen_reveal();                                // A screen with its own transition (battle, storage)
    std::pair<float, float> reveal_band() const;                // The wave's top and bottom (UI y) now
    void render_list(sf::RenderWindow& window, std::vector<std::shared_ptr<UIElement>>& elements, const sf::View& uiView, sf::View& worldView,
        bool withFrame);

	std::function<void(GameSpeedState)> m_setGameSpeedCallback; // Callback to set game speed state
    std::function<GameSpeedState()> m_getGameSpeedCallback;     // Callback to get current game speed state
    std::function<void()> m_quitGameCallback;                   // Callback to quit game
    std::function<void(bool)> m_fullscreenCallback;             // Callback to change the window mode

    bool m_wasFasterBeforePause = false;                        // "Continue" goes back to this speed
    float m_battleIntroSeconds = 0.f;                           // The battle start jingle's length (UIBattleIntro), set by start_battle
};

#include "UIManager.h"

#include "../stages/Stage.h"

#include "UIDeploymentIcons.h"
#include "UIStageCash.h"
#include "UICashUp.h"
#include "UIBaseHealthInfo.h"
#include "UIUpgradeSlider.h"
#include "UIEquipSlider.h"
#include "UIPlayerShellsInfo.h"

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
    for (auto& uiElement : m_uiElements)
    {
        uiElement->handle_event(event, window);
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
 * Generate Fish Tank UI screen:
 *   - Background, title, battle/upgrade/equip buttons
 *   - Each button sets a deferred callback using m_pendingAction
 *   - Player shells display
 */
void UIManager::generate_fish_tank_uis()
{
    m_uiElements.clear();

    auto bg = std::make_shared<UITextureElement>(
        sf::Vector2f{ 1920.f, 1080.f },
        sf::Vector2f{ 0.f, 0.f },
        "assets/images/textures/backgrounds/fish_tank.png"
    );

    auto title = std::make_shared<UITextElement>(
        sf::Vector2f{ 0.f, 0.f },
        sf::Vector2f{ 50.f, 50.f },
        "Fish Tank",
        48
    );

    // Battle button triggers stage load and switches UI
    auto battleButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ 200.f, 80.f },
        sf::Vector2f{ 100.f, 300.f },
        "BATTLE"
    );
    battleButton->set_callback([this]()
        {
            m_pendingAction = [this]() {
                if (auto stage = m_stage.lock())
                {
                    stage->load(1);
                    generate_battle_uis();
                }
                };
        });

    // Upgrade button switches UI to upgrade screen
    auto upgradeButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ 200.f, 80.f },
        sf::Vector2f{ 100.f, 400.f },
        "UPGRADE"
    );
    upgradeButton->set_callback([this]()
        {
            m_pendingAction = [this]() {
                generate_upgrade_menu_uis();
                };
        });

    // Equip button switches UI to equip screen
    auto equipButton = std::make_shared<UIButtonElement>(
        sf::Vector2f{ 200.f, 80.f },
        sf::Vector2f{ 100.f, 500.f },
        "EQUIP"
    );
    equipButton->set_callback([this]()
        {
            m_pendingAction = [this]() {
                generate_equip_menu_uis();
                };
        });

    auto shellsInfo = std::make_shared<UIPlayerShellsInfo>(m_dataLoader);

    add_ui_element(bg);
    add_ui_element(title);
    add_ui_element(battleButton);
    add_ui_element(upgradeButton);
    add_ui_element(equipButton);
    add_ui_element(shellsInfo);
}

//
// TODO: Document the rest of the file
//

void UIManager::generate_upgrade_menu_uis()
{
    m_uiElements.clear();

    //background
    auto bg = std::make_shared<UITextureElement>(
        sf::Vector2f{ 1920.f, 1080.f },
        sf::Vector2f{ 0.f, 0.f },
        "assets/images/textures/backgrounds/fish_tank.png"
    );

    //title text "Upgrades"
    auto title = std::make_shared<UITextElement>(
        sf::Vector2f{ 0.f, 0.f },
        sf::Vector2f{ 50.f, 50.f },
        "Upgrades",
        48
    );

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
                    generate_fish_tank_uis();
                }
                };
        });

    //slider
    auto upgradeSlider = std::make_shared<UIUpgradeSlider>(m_dataLoader);
	auto shellsInfo = std::make_shared<UIPlayerShellsInfo>(m_dataLoader);

    add_ui_element(bg);
    add_ui_element(title);
    add_ui_element(upgradeSlider);
    add_ui_element(shellsInfo);
    add_ui_element(menuButton);
}

void UIManager::generate_equip_menu_uis()
{
    m_uiElements.clear();

    //background
    auto bg = std::make_shared<UITextureElement>(
        sf::Vector2f{ 1920.f, 1080.f },
        sf::Vector2f{ 0.f, 0.f },
        "assets/images/textures/backgrounds/fish_tank.png"
    );

    //title text "Equip Team"
    auto title = std::make_shared<UITextElement>(
        sf::Vector2f{ 0.f, 0.f },
        sf::Vector2f{ 50.f, 50.f },
        "Equip Team",
        48
    );

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
                    generate_fish_tank_uis();
                }
                };
        });

    //slider
    auto equipSlider = std::make_shared<UIEquipSlider>(m_dataLoader);
    auto shellsInfo = std::make_shared<UIPlayerShellsInfo>(m_dataLoader);

    add_ui_element(bg);
    add_ui_element(title);
    add_ui_element(equipSlider);
    add_ui_element(shellsInfo);
    add_ui_element(menuButton);
}

void UIManager::generate_battle_uis()
{
	m_uiElements.clear();

	auto deploymentIcons = std::make_shared<UIDeploymentIcons>(m_dataLoader, m_stage.lock());
	auto stageCash = std::make_shared<UIStageCash>(m_stage.lock());
	auto cashUp = std::make_shared<UICashUp>(m_stage.lock());
	auto baseHealthInfo = std::make_shared<UIBaseHealthInfo>(m_stage.lock());

	add_ui_element(deploymentIcons);
	add_ui_element(stageCash);
	add_ui_element(cashUp);
	add_ui_element(baseHealthInfo);
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
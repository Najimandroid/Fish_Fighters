#include "UICashUp.h"
#include "../UISkin.h"
#include "../../core/stages/Stage.h"

namespace
{
    const float SIZE = 230.f;
    const float MARGIN = 24.f;                         // From the bottom left corner of the screen
    const float FRAME_SCALE = 1.25f;                   // Corners of the slot images on screen
    const float FRAME_THICKNESS = 6.f * FRAME_SCALE;   // Outline of slot.png (6 px in the image)
    const unsigned int LEVEL_SIZE = 30;
    const unsigned int COST_SIZE = 42;
    const float TEXT_PADDING = 16.f;
}

/*
 * Sets up the button size, position, icon, texts, and callback for handling upgrades.
 */
UICashUp::UICashUp(std::shared_ptr<Stage> stage) :
    UIButtonElement({ SIZE, SIZE }, { MARGIN, 1080.f - MARGIN - SIZE }), // Initialize base UIButtonElement
    m_stage(stage),
    m_icon(m_texture), // Initialize sprite with texture
    m_levelText(UISkin::heavy_font())
{
    set_fires_on_press(true);   // Real-time play: works the moment it's pressed
    // Load both icon textures once (update() only switches between them)
    auto success = m_texture.loadFromFile("assets/images/textures/icons/uis/cash_up.png");
    success = m_disabledTexture.loadFromFile("assets/images/textures/icons/uis/cash_up_disabled.png");
    m_texture.setSmooth(true);
    m_disabledTexture.setSmooth(true);
    m_icon.setTexture(m_texture, true);

    // Texts: level in white, cost in yellow digits
    UISkin::style_text(m_levelText, LEVEL_SIZE);
    UISkin::style_text(m_text, COST_SIZE, UISkin::NUMBER);

    // Callback executed when button is clicked (with its own sounds: "cash_up", or the error when it can't)
    set_click_sound("");
    set_callback(
        [this]()
        {
            // Prevent upgrading beyond max level
            if (m_level >= m_maxLevel) { play_sound("ui_error"); return; }

            // Lock stage reference
            if (auto stage = m_stage.lock())
            {
                // Attempt upgrade; if successful, increment level and increase cost
                if (stage->upgrade_cash(m_level, m_cost))
                {
                    m_level++;
                    m_cost *= 1.5f;
                    play_sound("cash_up");
                }
                else play_sound("ui_error");
            }
        }
    );
}

/*
 * Updates the button state and texts each frame:
 * - Changes icon and cost color based on affordability
 * - Updates the level and the cost (or MAX)
 */
void UICashUp::update(float deltaTime)
{
    // Check if upgrade is affordable or maxed
    auto stage = m_stage.lock();
    m_isAffordable = m_level >= m_maxLevel || (stage && stage->get_cash() >= m_cost);

    m_icon.setTexture(m_isAffordable ? m_texture : m_disabledTexture);

    m_levelText.setString("LEVEL " + std::to_string(m_level));

    if (m_level >= m_maxLevel)
    {
        m_text.setString("MAX");
        UISkin::set_text_color(m_text, UISkin::TEXT);
    }
    else
    {
        m_text.setString(std::to_string(m_cost) + "$");
        UISkin::set_text_color(m_text, m_isAffordable ? UISkin::NUMBER : UISkin::NUMBER_DISABLED);
    }
}

/*
 * Draws the slot, the icon inside it, the frame over the icon edges, then the level and the cost.
 * Everything follows the hover / press size of the button.
 */
void UICashUp::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;

    const sf::FloatRect rect = get_visual_rect();
    const float scale = rect.size.x / SIZE;
    const float inset = FRAME_THICKNESS * scale;

    UISkin::draw_nine_slice(window, UISkin::SLOT, rect, FRAME_SCALE * scale);

    const sf::Vector2f textureSize(m_icon.getTexture().getSize());
    m_icon.setScale({ (rect.size.x - 2.f * inset) / textureSize.x, (rect.size.y - 2.f * inset) / textureSize.y });
    m_icon.setPosition(rect.position + sf::Vector2f{ inset, inset });
    window.draw(m_icon);

    UISkin::draw_nine_slice(window, UISkin::SLOT_FRAME, rect, FRAME_SCALE * scale);

    // Level at the top center
    UISkin::set_text_anchor(m_levelText, { 0.5f, 0.f });
    m_levelText.setScale({ scale, scale });
    m_levelText.setPosition({ rect.position.x + rect.size.x / 2.f, rect.position.y + TEXT_PADDING * scale });
    window.draw(m_levelText);

    // Cost at the bottom right
    UISkin::set_text_anchor(m_text, { 1.f, 1.f });
    m_text.setScale({ scale, scale });
    m_text.setPosition(rect.position + rect.size - sf::Vector2f{ TEXT_PADDING, TEXT_PADDING } * scale);
    window.draw(m_text);
}

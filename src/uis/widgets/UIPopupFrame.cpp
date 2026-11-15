#include "UIPopupFrame.h"
#include "../UISkin.h"

namespace
{
    const float RADIUS = 30.f;
    const float CLOSE_SIZE = 96.f;
    const unsigned int TITLE_SIZE = 54;
    const sf::Color BODY{ 26, 42, 64 };
    const sf::Color BAND_TOP{ 238, 245, 251 }, BAND_BOTTOM{ 206, 221, 236 };   // Foam white, a touch of blue lower down
    const sf::Color FADED_FISH{ 70, 96, 130, 90 };                                // The face drawn faint on the band
}

UIPopupFrame::UIPopupFrame(const sf::FloatRect& rect, const std::string& title, float footerHeight) :
    m_rect(rect),
    m_footerHeight(footerHeight),
    m_title(UISkin::heavy_font(), title),
    m_close(CLOSE_SIZE, { rect.position.x + rect.size.x - CLOSE_SIZE * 0.68f, rect.position.y - CLOSE_SIZE * 0.32f }, "icon_close")
{
    UISkin::style_text(m_title, TITLE_SIZE, UISkin::TEXT);
    set_title(title);
    m_close.set_click_sound("ui_back");
}

void UIPopupFrame::set_title(const std::string& title)
{
    m_title.setString(title);
    UISkin::set_text_anchor(m_title, { 0.5f, 0.5f });
    m_title.setPosition({ m_rect.position.x + m_rect.size.x / 2.f, m_rect.position.y + HEADER / 2.f - 4.f });
}

void UIPopupFrame::set_on_close(std::function<void()> onClose)
{
    m_close.set_callback(std::move(onClose));
}

void UIPopupFrame::set_face(const std::string& textureName)
{
    m_face = textureName;
}

void UIPopupFrame::set_close_sound(const std::string& sound)
{
    m_close.set_click_sound(sound);
}

sf::FloatRect UIPopupFrame::get_body() const
{
    return { { m_rect.position.x, m_rect.position.y + HEADER }, { m_rect.size.x, m_rect.size.y - HEADER - m_footerHeight } };
}

sf::FloatRect UIPopupFrame::get_footer() const
{
    if (m_footerHeight <= 0.f) return {};
    return { { m_rect.position.x, m_rect.position.y + m_rect.size.y - m_footerHeight }, { m_rect.size.x, m_footerHeight } };
}

void UIPopupFrame::update(float deltaTime)
{
    m_close.update(deltaTime);
}

void UIPopupFrame::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    m_close.handle_event(event, window);
}

/*
 * Its shadow, the dark body with the dark outline, the light bands (square where they meet the body), the faded Fish,
 * the title, the X over the corner
 */
void UIPopupFrame::render(sf::RenderWindow& window)
{
    sf::ConvexShape shape = UISkin::rounded_rect({ m_rect.position + sf::Vector2f{ UISkin::SHADOW_SHIFT, UISkin::SHADOW_DROP } * 1.6f, m_rect.size }, RADIUS);
    shape.setFillColor(sf::Color(0, 0, 0, 110));
    window.draw(shape);

    shape = UISkin::rounded_rect(m_rect, RADIUS);
    shape.setFillColor(BODY);
    shape.setOutlineThickness(6.f);
    shape.setOutlineColor(UISkin::OUTLINE);
    window.draw(shape);

    // A light band (rounded on the popup's own corner side, square on the body's side), a dark line where it meets
    // the body
    const auto band = [&](float top, float height, bool isHeader)
        {
            sf::ConvexShape round = UISkin::rounded_rect({ { m_rect.position.x, top }, { m_rect.size.x, height } }, RADIUS);
            round.setFillColor(BAND_TOP);
            window.draw(round);
            sf::VertexArray shade(sf::PrimitiveType::TriangleStrip, 4);
            const float shadeTop = isHeader ? top + height * 0.45f : top, shadeBottom = isHeader ? top + height : top + height * 0.55f;
            const sf::Color upper = isHeader ? BAND_TOP : BAND_BOTTOM, lower = isHeader ? BAND_BOTTOM : BAND_TOP;
            shade[0] = sf::Vertex{ { m_rect.position.x, shadeTop }, upper };
            shade[1] = sf::Vertex{ { m_rect.position.x + m_rect.size.x, shadeTop }, upper };
            shade[2] = sf::Vertex{ { m_rect.position.x, shadeBottom }, lower };
            shade[3] = sf::Vertex{ { m_rect.position.x + m_rect.size.x, shadeBottom }, lower };
            window.draw(shade);
            sf::RectangleShape line({ m_rect.size.x, 5.f });
            line.setPosition({ m_rect.position.x, isHeader ? top + height - 2.5f : top - 2.5f });
            line.setFillColor(UISkin::OUTLINE);
            window.draw(line);
        };
    band(m_rect.position.y, HEADER, true);
    if (m_footerHeight > 0.f) band(m_rect.position.y + m_rect.size.y - m_footerHeight, m_footerHeight, false);

    // The Fish's face, faint, on the header's left (like the cat on The Battle Cats' popups)
    UISkin::draw_fitted(window, UISkin::texture(m_face),
        { { m_rect.position.x + 26.f, m_rect.position.y + 6.f }, { HEADER - 12.f, HEADER - 12.f } }, FADED_FISH);

    window.draw(m_title);
    m_close.render(window);
}

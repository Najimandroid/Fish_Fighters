#include "UIChartBackground.h"
#include "../UISkin.h"

#include <cmath>

namespace
{
    const sf::Color TOP{ 26, 84, 100 }, BOTTOM{ 14, 50, 66 };
    const float CELL = 48.f;          // The fine grid
    const int MAJOR_EVERY = 5;        // A stronger line every few cells
}

UIChartBackground::UIChartBackground() :
    UIElement({ 1920.f, 1080.f }, { 0.f, 0.f })
{
}

void UIChartBackground::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    const sf::FloatRect screen = UISkin::screen_rect();
    const float left = screen.position.x, right = left + screen.size.x, top = screen.position.y, bottom = top + screen.size.y;

    sf::VertexArray paper(sf::PrimitiveType::TriangleStrip, 4);
    paper[0] = sf::Vertex{ { left, top }, TOP };
    paper[1] = sf::Vertex{ { right, top }, TOP };
    paper[2] = sf::Vertex{ { left, bottom }, BOTTOM };
    paper[3] = sf::Vertex{ { right, bottom }, BOTTOM };
    window.draw(paper);

    // The grid, aligned on the layout (0, 0) so it doesn't shift between screen shapes
    sf::VertexArray lines(sf::PrimitiveType::Lines);
    const auto add_line = [&lines](sf::Vector2f from, sf::Vector2f to, bool isMajor)
        {
            const sf::Color color = isMajor ? sf::Color(150, 230, 240, 44) : sf::Color(150, 230, 240, 18);
            lines.append({ from, color });
            lines.append({ to, color });
        };
    const int firstColumn = static_cast<int>(std::floor(left / CELL)), lastColumn = static_cast<int>(std::ceil(right / CELL));
    for (int column = firstColumn; column <= lastColumn; ++column)
        add_line({ column * CELL, top }, { column * CELL, bottom }, column % MAJOR_EVERY == 0);
    const int firstRow = static_cast<int>(std::floor(top / CELL)), lastRow = static_cast<int>(std::ceil(bottom / CELL));
    for (int row = firstRow; row <= lastRow; ++row)
        add_line({ left, row * CELL }, { right, row * CELL }, row % MAJOR_EVERY == 0);
    window.draw(lines);
}

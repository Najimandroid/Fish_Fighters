#pragma once

#include <SFML/System/Vector2.hpp>

#include <cmath>

/*
 * UISwipe
 * -------
 * A horizontal swipe (a finger, or the mouse dragged): the carousels (Upgrade, Equip) move by one card when the
 * press slides far enough sideways; a short or mostly vertical move stays a tap (or a drag of a card down).
 *
 * Example usage:
 *   pressed:  m_swipe.press(point);
 *   moved:    m_swipe.move(point);
 *   released: if (const int direction = m_swipe.release(point)) move_to_index(m_currentIndex - direction);
 */
struct UISwipe
{
    static constexpr float DISTANCE = 70.f;   // Sideways, in UI pixels, to count as a swipe

    bool isDown = false;
    sf::Vector2f start, last;

    void press(sf::Vector2f point) { isDown = true; start = last = point; }
    void move(sf::Vector2f point) { if (isDown) last = point; }
    void cancel() { isDown = false; }

    // Sliding sideways (more sideways than up or down) and far enough
    bool is_swiping() const
    {
        const sf::Vector2f delta = last - start;
        return isDown && std::abs(delta.x) > DISTANCE && std::abs(delta.x) > 1.4f * std::abs(delta.y);
    }

    // On release: -1 slid left (the next card), +1 slid right (the previous one), 0 a tap
    int release(sf::Vector2f point)
    {
        last = point;
        const int direction = is_swiping() ? (last.x < start.x ? -1 : 1) : 0;
        isDown = false;
        return direction;
    }
};

#pragma once

#include <SFML/System/Clock.hpp>
#include <SFML/System/Vector2.hpp>

#include <cmath>

/*
 * UISwipe
 * -------
 * A horizontal swipe (a finger, or the mouse dragged): the carousels (Upgrade, Equip) and the chapter map's strip
 * follow it (last - start) once it slides sideways, then settle on the nearest card or stage; past DISTANCE, a quick
 * flick moves at least one. Let go faster than GLIDE_SPEED, they glide on (UIGlide). A short or mostly vertical move
 * stays a tap (or a drag of a card down).
 *
 * Example usage:
 *   pressed:  m_swipe.press(point);
 *   moved:    m_swipe.move(point); follow(m_swipe.last.x - m_swipe.start.x);
 *   released: m_swipe.release(point); if (std::abs(m_swipe.velocity.x) > UISwipe::GLIDE_SPEED) glide...
 */
struct UISwipe
{
    static constexpr float DISTANCE = 70.f;       // Sideways, in UI pixels, to count as a swipe
    static constexpr float GLIDE_SPEED = 400.f;   // Let go faster than this (UI pixels per second): it glides on

    bool isDown = false;
    sf::Vector2f start, last;
    sf::Vector2f velocity;   // The finger's speed, UI pixels per second (smoothed); none once it stopped and lifted

    void press(sf::Vector2f point)
    {
        isDown = true;
        start = last = m_sampleFrom = point;
        velocity = {};
        m_sampleClock.restart();
    }

    void move(sf::Vector2f point)
    {
        if (!isDown) return;
        last = point;
        // Several moves can come in one frame: the speed is measured over a few milliseconds at least
        const float elapsed = m_sampleClock.getElapsedTime().asSeconds();
        if (elapsed >= 0.012f)
        {
            velocity = velocity * 0.25f + (point - m_sampleFrom) * (0.75f / elapsed);
            m_sampleFrom = point;
            m_sampleClock.restart();
        }
    }

    void cancel() { isDown = false; }

    // Sliding sideways (more sideways than up or down) and far enough
    bool is_swiping() const
    {
        const sf::Vector2f delta = last - start;
        return isDown && std::abs(delta.x) > DISTANCE && std::abs(delta.x) > 1.4f * std::abs(delta.y);
    }

    // On release: -1 slid left (the next card), +1 slid right (the previous one), 0 a tap. A finger that stopped
    // before lifting has no speed left
    int release(sf::Vector2f point)
    {
        last = point;
        const int direction = is_swiping() ? (last.x < start.x ? -1 : 1) : 0;
        if (m_sampleClock.getElapsedTime().asSeconds() > 0.06f) velocity = {};
        isDown = false;
        return direction;
    }

private:
    sf::Vector2f m_sampleFrom;
    sf::Clock m_sampleClock;
};

/*
 * UIGlide
 * -------
 * A slide let go fast keeps going and slows down on its own, like a flicked wheel, and lands exactly on a card or a
 * stage: the target is picked from where that speed would carry it (projected), then it eases there. A tap while it
 * glides catches it (the widget stops it and settles on the nearest).
 *
 * Positions are in drag pixels (the same as the finger's last.x - start.x).
 */
struct UIGlide
{
    static constexpr float DECAY = 4.f;   // Per second: how quickly it slows down (it travels speed / DECAY)

    bool isActive = false;
    float position = 0.f;
    float target = 0.f;

    // Where a slide let go there at that speed would stop on its own
    static float projected(float from, float speed) { return from + speed / DECAY; }

    // The card (or stage) a slide let go at that speed lands on, counted like the carousels (index = from - drag /
    // spacing): the nearest to where it would stop, and at least one past the nearest now, since it was flung.
    // Not clamped: the caller keeps it among its cards
    static int landing(float fromIndex, float dragX, float speed, float spacing)
    {
        const int nearest = static_cast<int>(std::lround(fromIndex - dragX / spacing));
        int target = static_cast<int>(std::lround(fromIndex - projected(dragX, speed) / spacing));
        if (target == nearest) target += speed < 0.f ? 1 : -1;
        return target;
    }

    void start(float from, float to)
    {
        isActive = true;
        position = from;
        target = to;
    }

    // Moves on; false once it has arrived
    bool step(float deltaTime)
    {
        position += (target - position) * (1.f - std::exp(-DECAY * deltaTime));
        if (std::abs(target - position) < 1.5f)
        {
            position = target;
            isActive = false;
        }
        return isActive;
    }
};

#pragma once

#include "UIElement.h"

#include <deque>
#include <string>

class PlayerData;

/*
 * UIToast
 * -------
 * Good news popping in under the title bar for a moment ("SHOAL POWER 50!  +1 Shiny Scale"), one after the other.
 * UIToast::show() can be called from anywhere (the queue is shared): the toast element of the current screen shows
 * them, so add one (last) to the screens where news happen. Nothing to click.
 */
class UIToast : public UIElement
{
public:
    UIToast();

    // Queues news: a title and a line under it, with a UISkin icon (empty: none)
    static void show(const std::string& title, const std::string& line, const std::string& icon = "");

    // Gives the Shiny Scales of the Shoal Power milestones reached (PlayerData::claim_shoal_rewards) and announces
    // them. Call it after anything that raises a fish's level
    static void announce_shoal_rewards(PlayerData& player);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;

private:
    struct News { std::string title, line, icon; };
    static std::deque<News>& queue();

    float m_time = 0.f;          // How long the front one has been shown
    bool m_hasSounded = false;
};

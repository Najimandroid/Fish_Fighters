#pragma once

#include "UIElement.h"

#include <deque>
#include <string>

class PlayerData;

/*
 * UIToast
 * -------
 * Good news popping in under the title bar for a moment ("SCHOOL RANK 50!  +1 Shiny Scale"), one after the other.
 * UIToast::show() can be called from anywhere (the queue is shared): the toast element of the current screen shows
 * them, so add one (last) to the screens where news happen. Nothing to click.
 */
class UIToast : public UIElement
{
public:
    UIToast();

    // Queues news: a title and a line under it, with a UISkin icon (empty: none)
    static void show(const std::string& title, const std::string& line, const std::string& icon = "");

    // Gives the Shiny Scales of the School Rank milestones reached (PlayerData::claim_school_rank_rewards) and announces
    // them. Call it after anything that raises a fish's level
    static void announce_school_rank_rewards(PlayerData& player);

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;

private:
    struct News { std::string title, line, icon; };

    // Shared by every screen's toast: the news, how long the front one has been shown, if its sound played.
    // Changing screens doesn't start it again: the next screen's toast goes on where it was
    struct State { std::deque<News> news; float time = 0.f; bool hasSounded = false; };
    static State& state();
};

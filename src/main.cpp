#include "core/Game.h"

int main()
{
    // Create the main game instance
    Game game;

    // Start the main game loop
    // This will handle events, updates, and rendering continuously
    game.run_game_loop();

    // Clean up resources and terminate subsystems (e.g., DataLoader)
    game.terminate();

    return 0;
}

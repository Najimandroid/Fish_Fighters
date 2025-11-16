#include "core/Game.h"

/*
* Fish Fighters
* -------------
* By Najimandroid
* ---------------
* TODO: Write game description here and a few words for the code readers
*/
int main()
{
    Game game;

    // Start the main game loop
    // This will handle events, updates, and rendering
    game.run_game_loop();

    // Clean up resources and terminate subsystems (e.g., DataLoader)
    game.terminate();

	// Thanks for playing!
	// ~Najimandroid

    return 0;
}

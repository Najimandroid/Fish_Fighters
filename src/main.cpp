#include "core/Game.h"

/*
* Fish Fighters
* -------------
* By Najimandroid
* ---------------
* Fish Fighters is a horizontal tower defense game inspired by The Battle Cats.
*
* This project was made for fun and as a way to practice game programming in C++
* and project architecture.
* It is written in C++20 and uses SFML 3.0.
* 
* ---------------
* Project creation: July 2025
*/
int main()
{
    Game game;

    // Start the main game loop
    game.run_game_loop();

    // Clean up resources and terminate subsystems (e.g., DataLoader)
    game.terminate();

	// Thanks for playing!
	// ~Najimandroid

    return 0;
}
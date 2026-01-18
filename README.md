# Fish Fighters

Fish Fighters is a personal C++ game project developed without using a game engine.
It focuses on clean architecture, modular systems, and data-driven design.

This repository exists primarily as a code showcase.
A full presentation of the project — including gameplay, technical breakdown, and design decisions — is available on my portfolio.

👉 **Project page:**  
https://najimandroid.github.io/projects/project-fishfighters.html

---

## About the project

- Genre: Horizontal tower defense  
- Technology: C++ (custom engine, SFML)  
- Scope: Personal learning project  
- Status: Work in progress  

The project was designed to deepen my understanding of game architecture, state machines, and system-oriented design in C++.

---

## Building the project

This project uses CMake and is configured to build automatically.

### Requirements
- Git
- CMake 3.15 or newer
- A C++20 compatible compiler
- Windows (currently the only supported platform)

### Build steps (Windows)

Clone the project:
```bash
git clone https://github.com/Najimandroid/Fish_Fighters.git
```
#
Make sure to be located in the project directory, then:
```bash
cmake -S . -B build
cmake --build build --config Release
```
Note: the .exe file will be located inside the build/Release folder.
---

For more details, screenshots, and explanations, please refer to the portfolio page linked above.





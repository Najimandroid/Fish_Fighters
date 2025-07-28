cmake_minimum_required(VERSION 3.15)
project(Fish_Fighters)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SFML
set(SFML_DIR "${CMAKE_SOURCE_DIR}/external/SFML-3.0.0/lib/cmake/SFML")
find_package(SFML 3 REQUIRED COMPONENTS Graphics Audio Window System)

# ---------------------------------------------------------------------
# Source files grouped par cible
# ---------------------------------------------------------------------

# ImGui
set(IMGUI_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
)

# ImGui-SFML
set(IMGUI_SFML_SOURCES
    external/imgui-sfml/imgui-SFML.cpp
)

# Application sources
set(PROJECT_SOURCES
    src/main.cpp
    src/core/Game.cpp
    src/core/DataLoader.cpp
    src/core/BattleEntity.cpp
    src/core/BattleEnemy.cpp
    src/core/BattleBase.cpp
    src/core/stages/Stage.cpp
    # ← ajoute ici les futurs fichiers, par ex.:
    # src/utils/NewSystem.cpp
)

# ---------------------------------------------------------------------
# ImGui Library
# ---------------------------------------------------------------------
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC external/imgui)

# ImGui-SFML Library
add_library(imgui-sfml STATIC ${IMGUI_SFML_SOURCES})
target_include_directories(imgui-sfml PUBLIC external/imgui-sfml external/imgui)
target_link_libraries(imgui-sfml PUBLIC imgui SFML::Graphics SFML::Window SFML::System)

# ---------------------------------------------------------------------
# Main executable
# ---------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    external/imgui
    external/imgui-sfml
    external/tweeny/include
    external/SFML-3.0.0/include
    src/
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui-sfml
    SFML::Graphics
    SFML::Audio
    SFML::Window
    SFML::System
    opengl32.lib
)

# ---------------------------------------------------------------------
# Debug macro
# ---------------------------------------------------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_MODE)
endif()

# ---------------------------------------------------------------------
# DLLs copy configuration
# ---------------------------------------------------------------------
set(SFML_DLL_DIR "${CMAKE_SOURCE_DIR}/external/SFML-3.0.0/bin")

set(SFML_DLLS_DEBUG
    "${SFML_DLL_DIR}/sfml-graphics-d-3.dll"
    "${SFML_DLL_DIR}/sfml-window-d-3.dll"
    "${SFML_DLL_DIR}/sfml-audio-d-3.dll"
    "${SFML_DLL_DIR}/sfml-system-d-3.dll"
)

set(SFML_DLLS_RELEASE
    "${SFML_DLL_DIR}/sfml-graphics-3.dll"
    "${SFML_DLL_DIR}/sfml-window-3.dll"
    "${SFML_DLL_DIR}/sfml-audio-3.dll"
    "${SFML_DLL_DIR}/sfml-system-3.dll"
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    foreach(dll ${SFML_DLLS_DEBUG})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
    endforeach()
else()
    foreach(dll ${SFML_DLLS_RELEASE})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
    endforeach()
endif()

# ---------------------------------------------------------------------
# Copy assets folder to output
# ---------------------------------------------------------------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)

# ---------------------------------------------------------------------
# Copy game_data folder to output
# ---------------------------------------------------------------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/game_data
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/game_data
)

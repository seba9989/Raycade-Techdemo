cmake_minimum_required(VERSION 3.24)
project(gra LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Silnik Raycade pobierany z GitHuba. Żeby pracować na lokalnej kopii silnika (zmiany w silniku
# widać od razu w grze), wskaż jej katalog:
#   cmake -S . -B build -DFETCHCONTENT_SOURCE_DIR_RAYCADE=$HOME/Code/Raycade
# raylib, nlohmann_json i Box2D Raycade bierze z systemu (devenv/Nix) albo pobiera sam.
include(FetchContent)
FetchContent_Declare(
    raycade
    GIT_REPOSITORY https://github.com/seba9989/Raycade.git
    GIT_TAG dev
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raycade) # demo silnika nie jest budowane (to nie główny projekt)

# Sceny (makro SCENE) muszą być w pliku wykonywalnym, a nie w bibliotece statycznej - linker
# pominąłby pliki .o, do których nic się nie odwołuje, razem z rejestracją sceny.
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_executable(gra ${GAME_SOURCES})
target_include_directories(gra PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(gra PRIVATE raycade::raycade raycade_warnings)
set_target_properties(gra PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Zasoby kopiowane obok pliku wykonywalnego - Engine::AssetPath() i Assets szukają ich w assets/
add_custom_command(TARGET gra POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:gra>/assets)

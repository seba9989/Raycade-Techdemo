{ pkgs, ... }:

{
  # Narzędzia dostępne w powłoce projektu
  packages = with pkgs; [
    # raylib, nlohmann_json i Box2D pobiera CMake silnika (FetchContent) w przypiętych wersjach.
    # Do kompilacji raylib (GLFW) potrzebne są nagłówki X11 i OpenGL:
    libx11
    libxrandr
    libxinerama
    libxcursor
    libxi
    libxext
    libGL
    cmake
    ninja
    pkg-config
    gdb
    clang-tools     # clangd, clang-format, clang-tidy

    zed-editor
    nixd
    cmake-language-server
  ];

  languages.cplusplus.enable = true;

  # Skróty: `build`, `run`, `fmt`, `clean`, `edit`
  # Lokalna kopia silnika: RAYCADE_DIR=~/Code/Raycade build
  scripts.build.exec = ''
    cmake -S "$DEVENV_ROOT" -B "$DEVENV_ROOT/build" -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      ''${RAYCADE_DIR:+-DFETCHCONTENT_SOURCE_DIR_RAYCADE="$RAYCADE_DIR"}
    cmake --build "$DEVENV_ROOT/build"
    ln -sf build/compile_commands.json "$DEVENV_ROOT/compile_commands.json"
  '';

  scripts.run.exec = ''
    build && "$DEVENV_ROOT/build/gra" "$@"
  '';

  scripts.fmt.exec = ''
    find "$DEVENV_ROOT/src" -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i
  '';

  git-hooks.hooks.clang-format = {
    enable = true;
    types_or = [ "c" "c++" ];
  };

  scripts.edit.exec = ''
    zeditor "$DEVENV_ROOT"
  '';

  scripts.clean.exec = ''
    rm -rf "$DEVENV_ROOT/build" "$DEVENV_ROOT/compile_commands.json"
  '';

  enterShell = ''
    echo "Gra na Raycade"
    echo "  build  - konfiguracja i kompilacja (RAYCADE_DIR=... - lokalny silnik)"
    echo "  run    - kompilacja i uruchomienie"
    echo "  fmt    - formatowanie kodu"
    echo "  edit   - otwarcie projektu w Zedzie"
    echo "  clean  - usunięcie katalogu build"
  '';
}

![title](./resources/title.png?raw=true "Title")
# notepadgame

## Table of Contents

- [About](#about)
- [Getting Started](#getting_started)
- [Thanks](#thanks)

## About <a name = "about"></a>

`notepadgame` is an example of an open-world 2D top-down game that runs within the standard Windows notepad.exe application. The game engine uses DLL injection to modify the notepad.exe process at startup and make the following changes:

- Replaces the default Win32 EditControl with Scintilla, which is used in notepad++
- Updates the notepad.exe message processing mechanism from GetMessageW to PeekMessageW, providing a modern way to implement a fixed-time-step game loop
- Hooks the standard keyboard and mouse inputs to allow for player interaction
- Removes Notepad's main menu and status bar, and changes the window title to fit the game's context

<video src=./resources/example.mp4>.

The game in Notepad is based on an entity-component-system and supports:
- an open world
- a night-day cycle
- UTF-8 symbols for shapes
- multi-symbol big shapes
- collision detection based on a Quadtree algorithm
- pseudo non-uniform movement for projectiles that explode into fragments upon impact

## Getting Started <a name = "getting_started"></a>

This project only works on Windows and has only been tested on x64 platforms. It uses C++20. To build from source:
### Prerequisites

he project uses vcpkg to manage dependencies, and depends on several libraries:

- boost (signals2, container)
- EnTT
- Eigen3
- utfcpp
- range-v3

### Getting

Please note that the notepadgame depends on a scintilla submodule. The simplest way to clone it is:
```
git clone --recurse-submodules -j8 git@github.com:autogalkin/notepadgame.git 
```

### Build

The project uses CMake, and the CMakePresets contain a preset based on Ninja and MSVC

```
cmake --build --preset x64-debug --target all install
```

Binary files will be located in the */bin* directory. You need to run loader.exe


Enjoy!

## Thanks <a name = "thanks"></a>

- [render-with-notepad](https://github.com/khalladay/render-with-notepad) for showing the ability to implement a game in text editor

- [SuperNotepadBros](https://github.com/branw/SuperNotepadBros) for the clean iat table's hook example and an good game's time

- the long and intresting story with examples about quad tree by user @DragonEnergy on [stackoverflow](https://stackoverflow.com/questions/41946007/efficient-and-well-explained-implementation-of-a-quadtree-for-2d-collision-det)
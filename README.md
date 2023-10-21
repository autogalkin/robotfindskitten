# robotfindskitten

## About

`robotfindskitten` is a c++ reimplementation of the game originally written by
[Leonard Richardson](https://www.crummy.com/software/robotfindskitten/), now
inside notepad.exe and powered by the Entity Component System approach using
[EnTT].

The game uses DLL injection to modify the notepad and replaces the default Win32 EditControl with [Scintilla]. 

Also, I've implemented a gun, so be careful!

// An image here


[Scintilla]: https://www.scintilla.org
[EnTT]:  https://github.com/skypjack/entt


## Getting Started

The game can works only with the old version of Notepad.exe before Windows 11. To get it:
- Open Settings > Apps
- In the center pane select Advanced app settings > App execution aliases
- Toggle off Notepad

And get it:
- Download Pre-built binaries from [Releases page]().
- Unpack and run `loader.exe`

## Build

In this project I wanted to practice with glm and boost libraries and I use
`vcpkg` for:
- boost
- glm
- EnTT

I created CMakePresets for 64-bit builds based on Ninja and MSVC. To build the game, run:

```
cmake --preset x64-release
cmake --build --preset x64-release --target install
```
Binary files will be located in the *./out/bin/x64-release* directory.


## Details

- I have implemented a cool header only Quad Tree collision detection algorithm based on
[@DragonEnergy] article, You can check it in
*src/robotfindskitten/game/systems/collision.h* and tests are located in
*tests/test_collision.cpp*
- I update the notepad message pipe from GetMessageW() to the PeekMessageW() inside
  hook_GetMessageW() so now it does not sleep for a message, just poll and run
  as a real game.

- Remove Notepad's main menu and status bar, and changes the window title to fit the game's context

- Fixed time step with 60 fps in *src/robotfindskitten/engine/time.h*.


## Special thanks

- [render-with-notepad](https://github.com/khalladay/render-with-notepad) for
  showing the abilities to implement a game in the notepad

- [SuperNotepadBros](https://github.com/branw/SuperNotepadBros) for the clean iat table's hook example and an good game's time

- the long and intresting story with examples about quad tree by user @DragonEnergy on [stackoverflow](https://stackoverflow.com/questions/41946007/efficient-and-well-explained-implementation-of-a-quadtree-for-2d-collision-det)


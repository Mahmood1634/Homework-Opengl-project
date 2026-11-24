# Homework 2026 - Card game scene (OpenGL)

## Controls
| Key | Action |
|-----|--------|
| `1` | Camera 1 - straight above the table, cards lie flat |
| `2` | Camera 2 - player's point of view, cards stand up |
| `G` | A random card flies from the DECK zone to the PLAY zone (6th press does nothing) |
| `Esc` | Quit |

## Build
```
cmake -S . -B build
cmake --build build --config Release
```
Run `GLProject` (the exe finds `shaders/` and `textures/` through the `ASSET_ROOT` path set in CMake).

## Images you must replace (same file names, `GLProject/textures/`)
| File | What it must be |
|------|-----------------|
| `card_1.jpg` | Photo of one team member (as a fictional character) - portrait, ratio 2:3 (e.g. 512x768) |
| `card_2.jpg` ... `card_5.jpg` | Detailed fictional characters - portrait 2:3 |
| `poster.jpg` | Artwork of your choice, portrait ~3:4. The torn look is created by the shader. |

Optional (placeholders already work): `wall.jpg`, `floor.jpg`, `table.jpg`, `felt.jpg`, `slot.png`,
`dark_wood.jpg`, `brass.jpg`, `clock_face.jpg`, `globe.jpg`.

`python tools/make_placeholder_textures.py` regenerates ALL placeholders (it overwrites your cards/poster!).

## Structure
```
L4/
  CMakeLists.txt
  GLProject/Main.cpp                      scene, cameras, animation, input
  GLProject/shaders/vs/L4.vs, fs/L4.fs    shaders (lighting, card frame, torn poster)
  GLProject/textures/                     images
  OpenGL.SharedModule/src/abstraction     SharedModule.hpp, Drawer.hpp, OpenGlShape.hpp
  OpenGL.SharedModule/src/implementation  SharedModule.cpp, Drawer.cpp, OpenGlShape.cpp
  OpenGL.SharedModule/dependencies/include glad, KHR, stb_image.h (+ your GLFW / glm)
  prompts.txt                             list of prompts (hand this in)
```

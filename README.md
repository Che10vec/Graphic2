# Computer graphic assesment 2

A small C++ OpenGL demo that loads and renders two glTF models(third-party) with a free camera and an ImGui panel.
## Features

- OpenGL 3.3 Core rendering
- glTF model loading via tinygltf
- Light sources + ambient lighting
- Base colors, textures, normal maps
- Keyboard + mouse controls
- ImGui panel for transformations

## Demo (gif, link to a video)

[![Demo preview](./demo.gif)](./demo.mp4)

## Tool stack
Prerequisite
- CMake
- C++ compiler (GCC/Clang/MSVC)
- Git

Dependencies
- [GLFW](https://github.com/glfw/glfw.git)
- [GLAD](https://github.com/Webfra/glad.git)
- [GLM](https://github.com/g-truc/glm.git)
- [tinygltf](https://github.com/syoyo/tinygltf.git)
- [Dear ImGui](https://github.com/ocornut/imgui.git)

>[!note]
> Dependencies are fetched automatically with `FetchContent` during CMake configure.

## Quick start

From the root, where `CMakeLists.txt`, you may build and run via custom target, like this
```bash
cmake --build --preset graphic2 --target run
```

## Build

For a manual build from the root, where `CMakeLists.txt`, you may build using preset
```bash
cmake --preset graphic2
cmake --build --preset graphic2
```
Or generic CMake
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## Run

After a succesful build, from the project root
```bash
./out/build/graphic2/graphic2
```
On Windows it is typically
```bash
.\out\build\graphic2\graphic2.exe
```
[A quick link to graphic2.exe](./out/build/graphic2/graphic2.exe)

## Controls

`C` - toggle mouse capture
<br/>While mouse is captured:
- `W/A/S/D` - move forward/left/back/right
- `Space` - move up
- `Left Ctrl` - move down
- `Mouse move` - look around
- `Mouse wheel` - projection blend

ImGui **Controls** window:
- _Per-model_ transformations
  - Translation
  - Rotation
  - Scale (+ mirror)
- Lighting
  - Ambient intensity (also changes defult buffer color)
  - _Per-light-source_ transformations
    - Position
    - Color
    - Intensity
- Reset camera button

## Project structure

- `src/main.cpp` - app entry point, render loop, camera input, ImGui UI
- `src/GltfModel.h`/`src/GltfModel.cpp` - glTF loading and GPU upload/draw code
- `src/Shader.h`/`src/Shader.cpp` - shader compilation/linking
- `src/Camera.h` - camera math and controls
- `src/shaders/` - GLSL shader files
- `assets/` - bundled glTF assets and textures

## Assets structure

`130` and `vette` are both following similar outline:
```text
assets/
  <model name>/
    scene.gltf
    textures/
    license.txt
```
Required model entry files used by the app:
- `assets/130/scene.gltf`
- `assets/vette/scene.gltf`

> [!important]
> Dynamic model upload is unsupported therefore would require code and/or asset alteration

> [!tip]
> Each model has an original `.zip` to it. Extract in the `assets/` again if needed.  

## Assets and licenses

This repository includes third-party model assets. See:<br/>
`assets/130/license.txt`<br/>
`assets/vette/license.txt`

# 3D Rotation

[![C++](https://img.shields.io/badge/C++-17-00599C?style=flat&logo=cplusplus)](https://isocpp.org/)
[![OpenGL](https://img.shields.io/badge/OpenGL-4.6-5586A4?style=flat&logo=opengl)](https://www.opengl.org/)
[![CMake](https://img.shields.io/badge/CMake-Build-064F8C?style=flat&logo=cmake)](https://cmake.org/)

> A real-time 3D scene renderer built with C++ and OpenGL, featuring interactive object rotation, AABB/OBB dual-volume collision detection, Phong lighting, and an ImGui debug overlay — serving as the foundational rendering engine for the extended AI and reflection projects.

## Introduction

3D Rotation is the **base rendering engine** from which the AI Engine and 3D Reflection projects are derived. It solves the core problem of interactive 3D object manipulation — allowing users to rotate, translate, and inspect 3D geometry in real-time using keyboard and mouse input, while collision volumes prevent objects from overlapping.

Built with a clean C++ separation between rendering (`Shader`, `Camera`), geometry, and physics (`AABB`, `OBB`), this project establishes the modular architecture pattern reused across all subsequent 3D projects in this series.

## Table of Contents

- [Getting Started](#getting-started)
  - [System Requirements](#system-requirements)
  - [Prerequisites](#prerequisites)
  - [Quick Start](#quick-start)
  - [Manual Build](#manual-build)
  - [Troubleshooting](#troubleshooting)
- [Key Features](#key-features)
- [Architecture](#architecture)
- [Controls](#controls)
- [Design Decisions & Trade-offs](#design-decisions--trade-offs)
- [Project Layout](#project-layout)
- [License](#license)

## Getting Started

### System Requirements

* **OS:** Windows 10/11 (x64)
* **GPU:** OpenGL 4.6-capable GPU
* **RAM:** 4GB minimum
* **Disk:** 500MB free space

### Prerequisites

* **CMake** 3.15+
* **Visual Studio 2019/2022** with C++ Desktop workload
* **GLEW** and **GLFW3** — pre-built DLLs included in `build/Release/`

### Quick Start

1. **Clone the repository**

   ```bash
   git clone https://github.com/Yupoer/3D_rotation.git
   cd 3D_rotation
   ```

2. **Configure and build**

   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build . --config Release
   ```

3. **Run**

   ```bash
   .\Release\3DRender.exe
   ```

   > **Note:** Shader files (`.frag`, `.vert`) and `picSource/` textures must reside in the same directory as the executable. `glew32.dll` and `glfw3.dll` are pre-placed in `build/Release/`.

### Manual Build

Open `build/3DRender.sln` in Visual Studio and build the `3DRender` target in **Release** configuration.

### Troubleshooting

1. **Objects not rotating:** Ensure mouse input is captured — click the window to focus before using mouse-drag controls.
2. **Missing DLL error:** Copy `glew32.dll` and `glfw3.dll` from `build/Release/` to the same folder as the executable.
3. **Shader compilation error:** Verify `.frag` / `.vert` files are co-located with the `.exe`.

## Key Features

* **Interactive 3D Rotation**: Real-time object rotation via mouse drag, translating screen-space delta into quaternion-based world-space rotation for gimbal-lock-free manipulation.
* **AABB Collision Detection**: Axis-Aligned Bounding Box broad-phase collision tests — fast O(1) rejection of non-overlapping object pairs.
* **OBB Collision Detection**: Oriented Bounding Box narrow-phase collision using the **Separating Axis Theorem (SAT)** for accurate detection between arbitrarily-rotated objects.
* **Hierarchical Collision Pipeline**: AABB filters out non-colliding pairs first; only pairs passing AABB proceed to the more expensive OBB SAT check — minimising CPU load.
* **Phong Lighting**: Per-fragment ambient, diffuse, and specular shading via GLSL with configurable light position.
* **ImGui Debug Overlay**: Real-time display of object transform matrices, collision state, and light parameters without recompiling.
* **STL Model Support**: Load arbitrary 3D geometry from `.stl` files via the included converter tool.

## Architecture

```mermaid
graph TB
    Main[main.cpp\nRender + Input Loop]
    AABB[AABB.cpp\nBroad-phase Collision]
    OBB[OBB.cpp\nNarrow-phase Collision (SAT)]
    Bounding[BoundingStructures.h\nShared Volume Base]
    Camera[Camera.cpp\nView & Input]
    Shader[Shader.cpp\nGLSL Pipeline]
    ImGui[Dear ImGui\nDebug Panel]

    Main -->|Broad-phase| AABB
    AABB -->|Candidates| OBB
    AABB --> Bounding
    OBB --> Bounding
    Main --> Camera
    Main --> Shader
    Main --> ImGui
```

### Collision Pipeline

**Broad Phase (AABB):**
1. For each object pair, test axis-aligned bounding box overlap
2. Pairs with no AABB overlap are immediately discarded
3. Remaining pairs advance to narrow phase

**Narrow Phase (OBB SAT):**
1. Test 15 potential separating axes (3 face normals from each OBB + 9 edge cross-products)
2. If any axis separates the boxes → no collision
3. If all 15 axes overlap → collision confirmed; resolve penetration

**Why this pipeline?**
- **Two-phase approach:** AABB rejection is branchless and cache-friendly; OBB SAT is accurate but O(15) per pair. Combining them keeps average cost close to O(1) for sparse scenes.
- **BoundingStructures.h as base:** Shared definitions allow AABB and OBB to share bounding data without duplication.

## Controls

| Input | Action |
|-------|--------|
| **Mouse Drag** | Rotate selected object |
| **W / A / S / D** | Translate camera |
| **Scroll Wheel** | Zoom in / out |
| **ImGui Panel** | Adjust light, view collision state |

## Design Decisions & Trade-offs

* **Why AABB + OBB instead of just OBB?** OBB SAT requires testing 15 axes per pair — expensive for dense scenes. AABB provides a near-zero-cost pre-filter, reducing OBB invocations to only true collision candidates.
* **Why quaternion rotation instead of Euler angles?** Euler angles suffer from **gimbal lock** when two rotation axes align, producing unintuitive user input. Quaternions interpolate smoothly and avoid this singularity entirely.
* **Why a shared `BoundingStructures.h`?** Both AABB and OBB need to store centre and extents. A common header prevents struct duplication and ensures both systems stay in sync when the scene graph changes.
* **Why Dear ImGui for debug output?** Rendering transform matrices and collision flags to a terminal would require stopping the render loop. ImGui overlays this data non-intrusively in the same frame, preserving interactivity during debugging.

## Project Layout

```plaintext
.
├── AABB.cpp / AABB.h          # Axis-Aligned Bounding Box collision
├── OBB.cpp / OBB.h            # Oriented Bounding Box collision (SAT)
├── BoundingStructures.h       # Shared bounding volume base definitions
├── Camera.cpp / .h            # FPS camera + mouse-drag rotation input
├── Shader.cpp / .h            # GLSL shader loader & compiler
├── main.cpp                   # Application entry, render loop, input handler
├── ball.h                     # Hardcoded ball vertex array
├── irregular.h                # Irregular mesh vertex array
├── fragmentShaderSource.frag  # Fragment shader (Phong lighting)
├── vertexShaderSource.vert    # Vertex shader (MVP transform)
├── ball.stl                   # Source STL model
├── imgui/                     # Dear ImGui (GLFW + OpenGL3 backend)
├── picSource/                 # Texture images (.jpg)
├── build/                     # CMake build output (VS solution + Release exe)
└── CMakeLists.txt             # Build system configuration
```

## License

Distributed under the MIT License. See [LICENSE](LICENSE) for more information.

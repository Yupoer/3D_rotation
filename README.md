# 3D Rotation

[![C++](https://img.shields.io/badge/C++-17-00599C?style=flat&logo=cplusplus)](https://isocpp.org/)
[![OpenGL](https://img.shields.io/badge/OpenGL-4.6-5586A4?style=flat&logo=opengl)](https://www.opengl.org/)
[![CMake](https://img.shields.io/badge/CMake-Build-064F8C?style=flat&logo=cmake)](https://cmake.org/)

> A real-time 3D physics sandbox built with C++ and OpenGL, featuring rigid-body simulation with gravity, impulse application, quaternion-based rotation, AABB/OBB dual-volume collision detection, dual Phong light sources, and an ImGui runtime debug panel — the foundational rendering engine from which AI Engine and 3D Reflection are derived.

## Introduction

3D Rotation is the **base rendering and physics engine** from which the AI Engine and 3D Reflection projects are derived. It solves the core problem of interactive 3D physics simulation — rendering a ball and an irregular mesh inside a closed room, applying gravity and impulse forces, detecting collisions between objects and room boundaries, and resolving penetration in real time.

Built with a clean C++ separation between rendering (`Shader`, `Camera`), scene objects (`object.cpp`), collision volumes (`AABB`, `OBB`, `BoundingStructures.h`), and simulation (`PhysicManager`), this project establishes the modular architecture pattern reused across all subsequent 3D projects in this series.

## Table of Contents

- [Getting Started](#getting-started)
  - [System Requirements](#system-requirements)
  - [Prerequisites](#prerequisites)
  - [Quick Start](#quick-start)
  - [Manual Build](#manual-build)
  - [Troubleshooting](#troubleshooting)
- [Key Features](#key-features)
- [Architecture](#architecture)
- [Physics System](#physics-system)
- [Controls](#controls)
- [Design Decisions & Trade-offs](#design-decisions--trade-offs)
- [Project Layout](#project-layout)
- [License](#license)

## Getting Started

### System Requirements

* **OS:** Windows 10/11 (x64)
* **GPU:** OpenGL 4.6-capable GPU (NVIDIA GTX 1050+ or AMD RX 570+ recommended)
* **RAM:** 4 GB minimum
* **Disk:** 500 MB free space

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

1. **Objects falling through the floor:** Ensure `PhysicManager::update()` is called every frame with a valid `deltaTime`; very large `deltaTime` spikes (e.g. on first frame) can cause tunnelling — clamp delta to 0.05 s.
2. **Missing DLL error:** Copy `glew32.dll` and `glfw3.dll` from `build/Release/` to the same folder as the executable.
3. **Shader compilation error:** Verify `.frag` / `.vert` files are co-located with the `.exe`.
4. **Objects not visible:** The camera starts at `(1, 9, 1)` looking into the room. Use the ImGui sliders to reposition if the view is obscured.

## Key Features

* **Rigid-Body Physics Simulation**: `PhysicManager` integrates velocity and angular velocity each frame, applies gravity (`9.81 m/s²` default, adjustable via ImGui), and resolves object-room and object-object collisions with impulse-based penetration correction.
* **Impulse Application**: The ImGui panel exposes an "Apply Upward Impulse" button that fires `applyImpulse()` on both objects simultaneously, producing a physically correct upward launch with spin.
* **Quaternion-Based Rotation**: Object orientation is stored and updated as `glm::quat`, eliminating gimbal lock and enabling smooth angular integration from `angularVelocity`.
* **AABB Broad-Phase Collision**: Axis-Aligned Bounding Box overlap tests provide O(1) rejection of non-colliding object pairs before expensive narrow-phase tests.
* **OBB Narrow-Phase (SAT)**: Oriented Bounding Box tests using the **Separating Axis Theorem** check 15 potential separating axes for accurate collision between arbitrarily-rotated objects.
* **Bounding Sphere**: Ball object uses a `BoundingSphere` for fast sphere-room boundary detection alongside OBB for object-object collision.
* **Dual Phong Light Sources**: Two independently togglable point lights — a white ambient light and a coloured accent light — computed per-fragment in GLSL.
* **ImGui Runtime Panel**: Real-time control of camera position/orientation, gravity strength, angular drag, ball restitution, physics pause, and impulse application — no recompile needed.
* **STL Model Support**: Load arbitrary 3D geometry from `.stl` files via the included `stl2VA.exe` converter tool.

## Architecture

The system separates simulation, rendering, and collision into independent subsystems:

```
main.cpp  (Render + Physics Loop)
  ├── PhysicManager   — gravity, impulse, collision resolution
  ├── Object          — position, velocity, rotation, mass, restitution
  │     ├── AABB      — axis-aligned bounding box
  │     ├── OBB       — oriented bounding box (SAT)
  │     └── BoundingSphere — sphere collision volume
  ├── Camera          — FPS camera, view matrix
  ├── Shader          — GLSL shader loader
  └── Dear ImGui      — debug / control overlay
```

### Collision Pipeline

**Broad Phase (AABB):**
1. For each object pair, test axis-aligned bounding box overlap
2. Pairs with no AABB overlap are immediately discarded
3. Remaining pairs advance to narrow phase

**Narrow Phase (OBB SAT):**
1. Test 15 potential separating axes (3 face normals per OBB + 9 edge cross-products)
2. If any axis separates the boxes → no collision
3. If all 15 axes overlap → collision confirmed; compute impulse and resolve penetration

**Room Boundary:**
- Ball: `BoundingSphere` tests against the `roomAABB` walls; on contact, velocity is reflected and damped by `restitution`
- Irregular mesh: `OBB` tests against each of the 6 room faces

## Physics System

| Parameter | Default | Adjustable via ImGui |
|-----------|---------|----------------------|
| **Gravity** | 9.81 m/s² | Yes — slider 0–20 |
| **Angular Drag** | configurable | Yes — slider 0–5 |
| **Ball Restitution** | 0.4 | Yes — slider 0–1 |
| **Irregular Restitution** | 0.3 | No (code only) |
| **Ball Damping** | 0.98 | No (code only) |
| **Irregular Damping** | 0.75 | No (code only) |
| **Physics Pause** | false | Yes — checkbox |

**Initial Conditions:**
- Ball starts at `(7, 6, 7)` with zero velocity — falls under gravity
- Irregular mesh starts at `(7, 2, 7)` with 45° initial rotation about `(1,1,0)`

## Controls

| Input | Action |
|-------|--------|
| **ImGui — Camera Position sliders** | Move camera in world space |
| **ImGui — Pitch / Yaw sliders** | Rotate camera |
| **ImGui — Gravity slider** | Adjust gravitational acceleration |
| **ImGui — Angular Drag slider** | Tune rotational damping |
| **ImGui — Ball Restitution slider** | Change ball bounciness |
| **ImGui — Pause Physics checkbox** | Freeze / resume simulation |
| **ImGui — Apply Upward Impulse** | Launch both objects upward |
| **ImGui — Reset button** | Return objects to initial positions |
| **ImGui — Light 1 / Light 2 checkbox** | Toggle each Phong light source |
| **Escape** | Close application |

## Design Decisions & Trade-offs

* **Why AABB + OBB instead of just OBB?** OBB SAT requires testing 15 axes per pair — expensive for dense scenes. AABB provides a near-zero-cost pre-filter, reducing OBB invocations to only true collision candidates.
* **Why quaternion rotation instead of Euler angles?** Euler angles suffer from **gimbal lock** when two rotation axes align. Quaternions interpolate smoothly via `glm::quat` and avoid this singularity, enabling stable angular velocity integration.
* **Why `PhysicManager` as a separate class?** Centralising gravity, drag, impulse resolution, and collision handling in one class keeps `main.cpp` focused on the render loop. Adding new physics features (friction, joints) only requires changes inside `PhysicManager`.
* **Why a shared `BoundingStructures.h`?** Both AABB and OBB need to store centre and extents. A common header prevents struct duplication and ensures both collision volumes stay in sync when object transforms change.
* **Why Dear ImGui for runtime control?** Rendering transform matrices and collision flags to a terminal would require stopping the render loop. ImGui overlays this data non-intrusively in the same frame, preserving interactivity during debugging and tuning.
* **Why `std::this_thread::sleep_for(30ms)` FPS cap?** Without a frame cap the physics integrator receives near-zero `deltaTime` on fast GPUs, causing floating-point precision loss. A soft 33 FPS cap keeps `deltaTime` in a stable range without V-Sync dependency.

## Project Layout

```plaintext
.
├── AABB.cpp / AABB.h          # Axis-Aligned Bounding Box collision & wireframe draw
├── OBB.cpp / OBB.h            # Oriented Bounding Box collision (SAT) & wireframe draw
├── BoundingStructures.h       # Shared bounding volume base definitions
├── object.cpp / object.h      # Scene object: position, velocity, rotation, mass, impulse
├── physicManager.cpp / .h     # Physics update: gravity, drag, collision resolution
├── Camera.cpp / .h            # FPS camera + view matrix
├── Shader.cpp / .h            # GLSL shader loader & compiler
├── main.cpp                   # Application entry, render loop, ImGui panel
├── ball.h                     # Hardcoded ball vertex array
├── irregular.h                # Irregular mesh vertex array
├── room.h                     # Room geometry vertex data
├── fragmentShaderSource.frag  # Fragment shader (dual Phong lighting)
├── vertexShaderSource.vert    # Vertex shader (MVP transform)
├── ball.stl                   # Source STL model for ball
├── irregular.stl              # Source STL model for irregular mesh
├── stb_image.h                # Single-header texture loader
├── stl2VA.exe                 # STL-to-vertex-array converter
├── stl_to_vertex_array.cpp    # Converter source code
├── imgui/                     # Dear ImGui (GLFW + OpenGL3 backend)
├── picSource/                 # Texture images (.jpg)
├── build/                     # CMake build output (VS solution + Release exe)
└── CMakeLists.txt             # Build system configuration
```

## License

Distributed under the MIT License. See [LICENSE](LICENSE) for more information.

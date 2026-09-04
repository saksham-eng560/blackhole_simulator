# Schwarzschild Black Hole Ray Tracer

Real-time interactive simulation of a Schwarzschild black hole with accretion disk,
rendered via general-relativistic null geodesic ray tracing.

![Phase 1: Terminal](https://img.shields.io/badge/Phase_1-Terminal_ASCII-brightgreen)
![C++17](https://img.shields.io/badge/C%2B%2B-17-orange)

---

## Physics

| Feature | Equation / Method |
|---|---|
| **Spacetime** | Schwarzschild metric: ds² = −(1−rₛ/r)dt² + (1−rₛ/r)⁻¹dr² + r²dΩ² |
| **Ray tracing** | Null geodesic integration: **a⃗** = −(3/2)·rₛ·h²/r⁵·**r⃗** |
| **Integrator** | 4th-order Runge-Kutta with adaptive step sizing |
| **Accretion disk** | Novikov-Thorne thin disk: T(r) ∝ r⁻³/⁴·[1−√(rᵢₙ/r)]¹/⁴ |
| **Doppler beaming** | I_obs = g⁴·I_emit, g = g_grav·g_doppler |
| **Gravitational redshift** | g_grav = √(1 − rₛ/r) |
| **Einstein ring** | Emerges naturally from geodesic integration |
| **Photon sphere** | r = 3M = 1.5rₛ (unstable circular photon orbits) |
| **Event horizon** | r = 2M = rₛ (Schwarzschild radius) |
| **Performance** | C++11 `std::thread` multicore parallelization + RK4 LTO inlining + 60 FPS cap |

---

## Quick Start (Terminal)

### Prerequisites

- **C++17 compiler**: `g++` (≥ 7) or `clang++` (≥ 5)
- **CMake** ≥ 3.16 (optional — a plain Makefile is also provided)
- **macOS or Linux** (uses POSIX `termios` for raw terminal mode)

### Build & Run

**Option A — CMake:**
```bash
cd blackhole_physics
mkdir build && cd build
cmake .. && make
./blackhole_sim
```

**Option B — Make:**
```bash
cd blackhole_physics
make run
```

**Option C — Direct compile:**
```bash
g++ -std=c++17 -O2 -Iinclude -o blackhole_sim \
    src/main.cpp src/accretion_disk.cpp src/camera.cpp \
    src/ray_tracer.cpp src/terminal_renderer.cpp
./blackhole_sim
```

**Non-Interactive Mode:**
The application gracefully handles non-TTY environments (piped output, background processes) and will render a single frame rather than entering the interactive loop. Use `strace` or pipe to `cat` for debugging:`./blackhole_sim | cat`

### Interactive Controls

| Key | Action |
|-----|--------|
| `←` `→` | Rotate camera azimuth (φ) |
| `↑` `↓` | Rotate camera elevation (θ) |
| `+` / `-` | Zoom in / out |
| `m` / `M` | Decrease / increase black hole mass |
| `f` / `F` | Narrow / widen field of view |
| `e` / `E` | Decrease / increase exposure (brightness) |
| `c` | Cycle color mode: ASCII → 256-color → Truecolor |
| `p` | Cycle color palette: Red-Orange → Electric Blue → Purple |
| `s` | Toggle background starfield |
| `r` | Reset camera to default position |
| `q` | Quit |

---

## Project Structure

```
blackhole_physics/
├── CMakeLists.txt               # CMake build
├── Makefile                     # Simple alternative build
├── README.md                    # This file
├── include/
│   ├── vec3.h                   # 3D vector math (header-only)
│   ├── black_hole.h             # Schwarzschild BH model (header-only)
│   ├── accretion_disk.h         # Thin disk model
│   ├── camera.h                 # Observer camera
│   ├── ray_tracer.h             # Null geodesic integrator
│   └── terminal_renderer.h     # ASCII/ANSI terminal renderer
├── src/
│   ├── main.cpp                 # Terminal entry point
│   ├── accretion_disk.cpp       # Disk physics implementation
│   ├── camera.cpp               # Camera implementation
│   ├── ray_tracer.cpp           # RK4 integration engine
│   └── terminal_renderer.cpp   # Terminal rendering engine
```

---

## How It Works

1. **Camera** generates a grid of ray directions (one per terminal character or pixel)
2. Each ray is integrated through **Schwarzschild curved spacetime** using RK4
3. At each integration step, the ray is checked for:
   - **Event horizon absorption** (r ≤ rₛ → black pixel)
   - **Accretion disk intersection** (equatorial plane crossing within disk radii)
   - **Escape to infinity** (r > r_escape → starfield or dark sky)
4. For disk hits, **Doppler beaming** and **gravitational redshift** are computed
5. Intensities are **tone-mapped** and rendered as ASCII characters or colored pixels

---

## License

MIT License — see individual source files for detailed documentation.

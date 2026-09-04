cmake_minimum_required(VERSION 3.16)
project(blackhole_sim
    VERSION 1.0.0
    DESCRIPTION "Schwarzschild Black Hole Ray Tracer"
    LANGUAGES CXX)

# ── C++17 required ──────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Source files ────────────────────────────────────────────────────────────
set(PHYSICS_SOURCES
    src/accretion_disk.cpp
    src/camera.cpp
    src/ray_tracer.cpp
)

    add_executable(blackhole_sim
        src/main.cpp
        src/terminal_renderer.cpp
        ${PHYSICS_SOURCES}
    )
    target_include_directories(blackhole_sim PRIVATE include)
    target_compile_options(blackhole_sim PRIVATE
        -O2 -Wall -Wextra -Wpedantic -march=native
    )
    # Link pthreads if available (for future multi-threading)
    find_package(Threads QUIET)
    if(Threads_FOUND)
        target_link_libraries(blackhole_sim Threads::Threads)
    endif()



#include "black_hole.h"
#include "accretion_disk.h"
#include "camera.h"
#include "ray_tracer.h"
#include "terminal_renderer.h"

#include <chrono>
#include <cstdio>
#include <string>
#include <sstream>
#include <iomanip>
#include <csignal>
#include <unistd.h>
#include <thread>
#include <vector>

static volatile bool g_running = true;

static void signalHandler(int) {
    g_running = false;
}

int main() {

    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);


    BlackHole     bh(1.0);                       
    AccretionDisk disk(bh, 10.0);                
    Camera        camera(12.0, 1.57, 0.0, 55.0); 
    RayTracer     tracer(bh, disk);


    TerminalRenderer renderer;
    if (!renderer.init()) {
        fprintf(stderr, "Error: failed to initialize terminal.\n");
        return 1;
    }


    double fps          = 0.0;
    int    frame_count  = 0;

    auto last_fps_time = std::chrono::high_resolution_clock::now();



    while (g_running) {
        auto frame_start = std::chrono::high_resolution_clock::now();


        renderer.updateSize();
        int w = renderer.getWidth();
        int h = renderer.getHeight();


        int key;
        while ((key = renderer.readInput()) != -1) {
            switch (key) {

                case 'q': case 'Q':
                    g_running = false;
                    break;


                case KEY_LEFT:   camera.rotatePhi(-0.04);   break;
                case KEY_RIGHT:  camera.rotatePhi(0.04);    break;
                case KEY_UP:     camera.rotateTheta(-0.03); break;
                case KEY_DOWN:   camera.rotateTheta(0.03);  break;


                case '+': case '=': camera.zoom(0.9);  break;  
                case '-': case '_': camera.zoom(1.1);  break;  


                case 'm': bh.setMass(std::max(0.2, bh.mass - 0.1));
                          disk = AccretionDisk(bh, 10.0);
                          break;
                case 'M': bh.setMass(std::min(5.0, bh.mass + 0.1));
                          disk = AccretionDisk(bh, 10.0);
                          break;


                case 'f': camera.adjustFOV(-2.0); break;
                case 'F': camera.adjustFOV(2.0);  break;


                case 'e': tracer.exposure = std::max(0.5, tracer.exposure - 0.5); break;
                case 'E': tracer.exposure = std::min(30.0, tracer.exposure + 0.5); break;


                case 'c': case 'C':
                    switch (renderer.color_mode) {
                        case ColorMode::ASCII_ONLY: renderer.color_mode = ColorMode::COLOR_256; break;
                        case ColorMode::COLOR_256:  renderer.color_mode = ColorMode::TRUECOLOR; break;
                        case ColorMode::TRUECOLOR:  renderer.color_mode = ColorMode::ASCII_ONLY; break;
                    }
                    break;


                case 'p': case 'P':
                    switch (renderer.color_scheme) {
                        case ColorScheme::RED_ORANGE: renderer.color_scheme = ColorScheme::BLUE; break;
                        case ColorScheme::BLUE:       renderer.color_scheme = ColorScheme::PURPLE; break;
                        case ColorScheme::PURPLE:     renderer.color_scheme = ColorScheme::RED_ORANGE; break;
                    }
                    break;


                case 's': case 'S':
                    tracer.enable_stars = !tracer.enable_stars;
                    break;


                case 'r': case 'R':
                    camera = Camera(12.0, 1.57, 0.0, 55.0);
                    break;
            }
        }




        double aspect = static_cast<double>(w) / (static_cast<double>(h) * 2.0);
        Vec3 cam_pos = camera.getPosition();

        renderer.clear();

        unsigned int num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;
        std::vector<std::thread> threads;

        for (unsigned int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t, num_threads]() {
                for (int y = t; y < h; y += num_threads) {
                    for (int x = 0; x < w; ++x) {
                        double u = 2.0 * static_cast<double>(x) / static_cast<double>(w) - 1.0;
                        double v = 1.0 - 2.0 * static_cast<double>(y) / static_cast<double>(h);

                        Vec3 dir = camera.generateRayDirection(u, v, aspect);
                        TraceResult result = tracer.trace(cam_pos, dir);

                        if (result.hit == HitType::BACKGROUND_STAR && result.intensity < 0.6) {
                            result.intensity = 0.6; 
                        }

                        renderer.setPixel(x, y, result.intensity, result.hue);
                    }
                }
            });
        }

        for (auto& th : threads) {
            th.join();
        }


        frame_count++;


        auto frame_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> frame_duration = frame_end - frame_start;
        double target_ms = 1000.0 / 60.0;
        if (frame_duration.count() < target_ms) {
            std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(target_ms - frame_duration.count()));
        }


        auto now = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(now - last_fps_time).count();
        if (elapsed >= 0.5) {
            fps = frame_count / elapsed;
            frame_count = 0;
            last_fps_time = now;
        }


        const char* color_label = "ASCII";
        if (renderer.color_mode == ColorMode::COLOR_256)  color_label = "256-color";
        if (renderer.color_mode == ColorMode::TRUECOLOR)  color_label = "Truecolor";

        char hud1[256], hud2[256];
        snprintf(hud1, sizeof(hud1),
                 " FPS: %.1f | %dx%d | M=%.1f rs=%.1f | "
                 "d=%.1f θ=%.2f φ=%.2f | FOV=%.0f° | exp=%.1f | %s%s",
                 fps, w, h, bh.mass, bh.rs,
                 camera.cam_distance, camera.cam_theta, camera.cam_phi,
                 camera.fov * 180.0 / M_PI, tracer.exposure,
                 color_label,
                 tracer.enable_stars ? " ★" : "");

        snprintf(hud2, sizeof(hud2),
                 " [←→↑↓] Rotate  [+/-] Zoom  [m/M] Mass  [f/F] FOV  "
                 "[e/E] Exposure  [c] Mode  [p] Palette  [s] Stars  [r] Reset  [q] Quit");

        renderer.setHUD(hud1, hud2);


        renderer.render();
    }


    renderer.cleanup();
    printf("Black hole simulation terminated.\n");
    return 0;
}
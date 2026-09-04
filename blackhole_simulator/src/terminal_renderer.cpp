

#include "terminal_renderer.h"

#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <algorithm>
#include <cmath>

static const char ASCII_GRADIENT[] = " .,:;=+*#%@";
static constexpr int GRADIENT_LEN = 10;  

static struct termios g_original_termios;

TerminalRenderer::TerminalRenderer() : width(80), height(24) {}

TerminalRenderer::~TerminalRenderer() {
    cleanup();
}

bool TerminalRenderer::init() {
    if (initialized) return true;


    if (!isatty(STDIN_FILENO)) {

        width = 80;
        height = 24;
        initialized = true;
        return true;
    }


    if (tcgetattr(STDIN_FILENO, &g_original_termios) < 0) {
        return false;
    }

    struct termios raw = g_original_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_cc[VMIN]  = 0;   
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);


    printf("\033[?25l\033[2J\033[H");
    fflush(stdout);

    updateSize();
    initialized = true;
    return true;
}

void TerminalRenderer::cleanup() {
    if (!initialized) return;


    if (!isatty(STDIN_FILENO)) {
        initialized = false;
        return;
    }


    printf("\033[?25h\033[0m\033[2J\033[H");
    fflush(stdout);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_original_termios);
    initialized = false;
}

void TerminalRenderer::updateSize() {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        width  = ws.ws_col;
        height = std::max(4, static_cast<int>(ws.ws_row) - 3); 
    }
    intensity_buf.assign(width * height, 0.0);
    hue_buf.assign(width * height, 0.0);
}

void TerminalRenderer::setPixel(int x, int y, double intensity, double hue) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        int idx = y * width + x;
        intensity_buf[idx] = intensity;
        hue_buf[idx]       = hue;
    }
}

void TerminalRenderer::clear() {
    std::fill(intensity_buf.begin(), intensity_buf.end(), 0.0);
    std::fill(hue_buf.begin(), hue_buf.end(), 0.0);
}

void TerminalRenderer::setHUD(const std::string& line1, const std::string& line2) {
    hud_line1 = line1;
    hud_line2 = line2;
}

char TerminalRenderer::intensityToChar(double intensity) const {
    int idx = static_cast<int>(std::clamp(intensity, 0.0, 1.0) * GRADIENT_LEN);
    return ASCII_GRADIENT[std::min(idx, GRADIENT_LEN)];
}

void TerminalRenderer::heatmapRGB(double hue, double brightness,
                                   int& r, int& g, int& b) const {
    double t  = std::clamp(hue, 0.0, 1.0);
    double br = std::clamp(brightness, 0.0, 1.0);
    br = std::pow(br, 0.6); 

    double rd, gn, bl;

    if (color_scheme == ColorScheme::RED_ORANGE) {
        if (t < 0.25) {

            double s = t / 0.25;
            rd = 0.6 + 0.4 * s;
            gn = 0.1 * s;
            bl = 0.0;
        } else if (t < 0.5) {

            double s = (t - 0.25) / 0.25;
            rd = 1.0;
            gn = 0.1 + 0.6 * s;
            bl = 0.0 + 0.05 * s;
        } else if (t < 0.75) {

            double s = (t - 0.5) / 0.25;
            rd = 1.0;
            gn = 0.7 + 0.3 * s;
            bl = 0.05 + 0.7 * s;
        } else {

            double s = (t - 0.75) / 0.25;
            rd = 1.0 - 0.25 * s;
            gn = 1.0 - 0.1 * s;
            bl = 0.75 + 0.25 * s;
        }
    } else if (color_scheme == ColorScheme::BLUE) {
        if (t < 0.25) {
            double s = t / 0.25;
            rd = 0.0;
            gn = 0.2 * s;
            bl = 0.6 + 0.4 * s;
        } else if (t < 0.5) {
            double s = (t - 0.25) / 0.25;
            rd = 0.0;
            gn = 0.2 + 0.6 * s;
            bl = 1.0;
        } else if (t < 0.75) {
            double s = (t - 0.5) / 0.25;
            rd = 0.6 * s;
            gn = 0.8 + 0.2 * s;
            bl = 1.0;
        } else {
            double s = (t - 0.75) / 0.25;
            rd = 0.6 + 0.4 * s;
            gn = 1.0;
            bl = 1.0;
        }
    } else {
        if (t < 0.25) {
            double s = t / 0.25;
            rd = 0.3 + 0.3 * s;
            gn = 0.0;
            bl = 0.5 + 0.5 * s;
        } else if (t < 0.5) {
            double s = (t - 0.25) / 0.25;
            rd = 0.6 + 0.4 * s;
            gn = 0.0;
            bl = 1.0;
        } else if (t < 0.75) {
            double s = (t - 0.5) / 0.25;
            rd = 1.0;
            gn = 0.4 * s;
            bl = 1.0;
        } else {
            double s = (t - 0.75) / 0.25;
            rd = 1.0;
            gn = 0.4 + 0.6 * s;
            bl = 1.0;
        }
    }

    r = std::clamp(static_cast<int>(255.0 * rd * br), 0, 255);
    g = std::clamp(static_cast<int>(255.0 * gn * br), 0, 255);
    b = std::clamp(static_cast<int>(255.0 * bl * br), 0, 255);
}

std::string TerminalRenderer::coloredChar(double intensity, double hue) const {
    char ch = intensityToChar(intensity);

    if (color_mode == ColorMode::ASCII_ONLY) {
        return std::string(1, ch);
    }


    if (hue < -0.5) {
        ch = '@';  
        int grey = static_cast<int>(255.0 * (0.3 + 0.4 * intensity));  
        int r = grey; int g = grey - 30; int b = grey + 50;  
        r = std::clamp(r, 0, 255);
        g = std::clamp(g, 0, 255);
        b = std::clamp(b, 0, 255);

        char buf[48];
        if (color_mode == ColorMode::TRUECOLOR) {
            snprintf(buf, sizeof(buf), "\033[38;2;%d;%d;%dm%c", r, g, b, ch);
        } else {
            int r6 = r * 5 / 255;
            int g6 = g * 5 / 255;
            int b6 = b * 5 / 255;
            int idx = 16 + 36 * r6 + 6 * g6 + b6;
            snprintf(buf, sizeof(buf), "\033[38;5;%dm%c", idx, ch);
        }
        return std::string(buf);
    }

    if (intensity < 0.001) {

        return std::string(1, ' ');
    }

    int r, g, b;

    if (hue > 0.001) {

        heatmapRGB(hue, intensity, r, g, b);
    } else {

        int v = static_cast<int>(255.0 * std::clamp(intensity, 0.0, 1.0));
        r = static_cast<int>(v * 0.9);
        g = static_cast<int>(v * 0.95);
        b = std::min(255, static_cast<int>(v * 1.1));
    }

    char buf[48];
    if (color_mode == ColorMode::TRUECOLOR) {
        snprintf(buf, sizeof(buf), "\033[38;2;%d;%d;%dm%c", r, g, b, ch);
    } else {

        int r6 = r * 5 / 255;
        int g6 = g * 5 / 255;
        int b6 = b * 5 / 255;
        int idx = 16 + 36 * r6 + 6 * g6 + b6;
        snprintf(buf, sizeof(buf), "\033[38;5;%dm%c", idx, ch);
    }
    return std::string(buf);
}

void TerminalRenderer::render() {
    output_buf.clear();
    output_buf.reserve(static_cast<size_t>(width) * height * 20 + 512);


    output_buf += "\033[H";

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            output_buf += coloredChar(intensity_buf[idx], hue_buf[idx]);
        }
        output_buf += "\r\n";
    }


    output_buf += "\033[0m";           
    output_buf += "\033[K";            
    output_buf += "\033[1m";           
    output_buf += hud_line1;
    output_buf += "\033[0m\r\n\033[K"; 
    output_buf += hud_line2;


    write(STDOUT_FILENO, output_buf.c_str(), output_buf.size());
}

int TerminalRenderer::readInput() {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv = {0, 0};

    if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) <= 0) {
        return -1;
    }

    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return -1;


    if (c == '\033') {
        char seq[2] = {0, 0};
        struct timeval short_tv;

        short_tv = {0, 10000};  
        fd_set fds2;
        FD_ZERO(&fds2);
        FD_SET(STDIN_FILENO, &fds2);
        if (select(STDIN_FILENO + 1, &fds2, nullptr, nullptr, &short_tv) > 0) {
            if (read(STDIN_FILENO, &seq[0], 1) == 1) {
                short_tv = {0, 10000}; 
                FD_ZERO(&fds2);
                FD_SET(STDIN_FILENO, &fds2);
                if (select(STDIN_FILENO + 1, &fds2, nullptr, nullptr, &short_tv) > 0) {
                    if (read(STDIN_FILENO, &seq[1], 1) != 1) seq[1] = 0;
                }
            }
        }

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
            }
        }
        return '\033';  
    }

    return static_cast<int>(c);
}

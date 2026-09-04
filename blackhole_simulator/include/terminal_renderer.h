
#pragma once

#include <string>
#include <vector>

enum class ColorMode {
    ASCII_ONLY,    
    COLOR_256,     
    TRUECOLOR      
};

enum class ColorScheme {
    RED_ORANGE,
    BLUE,
    PURPLE
};

enum ArrowKey : int {
    KEY_UP    = 1000,
    KEY_DOWN  = 1001,
    KEY_RIGHT = 1002,
    KEY_LEFT  = 1003
};

class TerminalRenderer {
public:
    TerminalRenderer();
    ~TerminalRenderer();


    bool init();


    void cleanup();


    int getWidth()  const { return width; }

    int getHeight() const { return height; }


    void setPixel(int x, int y, double intensity, double hue = 0.0);


    void clear();


    void setHUD(const std::string& line1, const std::string& line2 = "");


    void render();


    int readInput();


    void updateSize();


    ColorMode color_mode = ColorMode::TRUECOLOR;


    ColorScheme color_scheme = ColorScheme::RED_ORANGE;

private:
    int width, height;
    std::vector<double> intensity_buf;
    std::vector<double> hue_buf;
    std::string output_buf;
    std::string hud_line1, hud_line2;
    bool initialized = false;


    char intensityToChar(double intensity) const;


    std::string coloredChar(double intensity, double hue) const;


    void heatmapRGB(double hue, double brightness,
                    int& r, int& g, int& b) const;
};

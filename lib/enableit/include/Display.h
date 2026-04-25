#pragma once

#include <Arduino.h>

namespace enableit {

// Display properties structure
struct DisplayProps
{
    int width;  // Horizontal resolution (pixels)
    int height; // Vertical resolution (pixels)
    int lines;  // Number of text lines
    int rows;   // Number of text columns
};

// Abstract class to represent the basic functionalities of a display
class Display
{
public:
    // Enum with common colors
    enum class Color
    {
        BLACK = 0x0000,
        WHITE = 0xFFFF,
        RED = 0xF800,
        GREEN = 0x07E0,
        BLUE = 0x001F,
        YELLOW = 0xFFE0,
        CYAN = 0x07FF,
        MAGENTA = 0xF81F,
        GRAY = 0x8410
    };

    virtual ~Display() = default;

    virtual void begin() = 0;

    virtual void end() = 0;

    // Clears the screen
    virtual void clear() = 0;

    // Sets the text size
    virtual void setTextSize(int size) = 0;

    // Sets the text color
    virtual void setTextColor(Color color) = 0;

    // Sets the cursor position
    virtual void setCursor(int x, int y) = 0;

    // Fills the screen with a color
    virtual void fillScreen(Color color) = 0;

    // Draws a filled rectangle
    virtual void fillRect(int x, int y, int w, int h, Color color) = 0;

    // Prints a string
    virtual void print(const char *text) = 0;

    // Prints a String
    virtual void print(const String &text) = 0;

    // Prints an integer
    virtual void print(const int number) = 0;

    // Prints a string and moves to the next line
    virtual void println(const char *text) = 0;

    // Prints a String and moves to the next line
    virtual void println(const String &text) = 0;

    virtual void println(const int number) = 0;

    // Get display peoperties
    virtual DisplayProps getProps() const = 0;

    // from kinetix project
    virtual void setTitle(String) = 0;
    virtual void setLine(int, String) = 0;
    virtual void refresh() = 0;

    // from M5 Display
    void qrcode(const char *string, uint16_t x = 5, uint16_t y = 45,
                uint8_t width = 70, uint8_t version = 7);
    void qrcode(const String &string, uint16_t x = 5, uint16_t y = 45,
                uint8_t width = 70, uint8_t version = 7);
};

// Mock implementation of Display for testing or non-hardware environments
class MockDisplay : public Display
{
public:
    void begin() override {}

    void clear() override {}

    void end() override {}

    void setTextSize(int size) override {}

    void setTextColor(Color color) override {}

    void setCursor(int x, int y) override {}

    void fillScreen(Color color) override {}

    void fillRect(int x, int y, int w, int h, Color color) override {}

    void print(const char *text) override {}

    void print(const String &text) override {}

    void print(const int number) override {}

    void println(const char *text) override {}

    void println(const String &text) override {}

    void println(const int number) override {}

    DisplayProps getProps() const override
    {
        return DisplayProps{0, 0, 0, 0};
    }

    void setTitle(String) override {}

    void setLine(int, String) override {}

    void refresh() override {}
};

extern Display& display;

} // namespace enableit
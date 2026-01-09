#pragma once

#include <ConsoleTransport.h>
#include <Display.h>

// Base class for a board
namespace enableit {

class Wifi;

class Board {
public:
    virtual ~Board() = default;

    // HAL aggregate
    virtual Wifi& wifi() = 0;

    // Initialize the board, optionally enabling the LCD
    virtual void begin(bool lcdEnabled = true) = 0;

    // Deinitialize the board
    virtual void end() = 0;

    // Get a reference to the display
    virtual Display& getDisplay() = 0;

    // Return the default hardware serial port for the board
    virtual ConsoleTransport& serial() = 0;
};

extern Board& board;

} // namespace enableit




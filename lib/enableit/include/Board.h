#pragma once

#include <Display.h>

class WifiHal;

// Base class for a board
class Board {
public:
    virtual ~Board() = default;

    // HAL aggregate
    virtual WifiHal& wifi() = 0;

    // Initialize the board, optionally enabling the LCD
    virtual void begin(bool lcdEnabled = true) = 0;

    // Deinitialize the board
    virtual void end() = 0;

    // Get a reference to the display
    virtual Display& getDisplay() = 0;
};

// Concrete board must define also the global instance 'eBoardImpl' and the macro 'board'
// Example: 
extern Board& board;




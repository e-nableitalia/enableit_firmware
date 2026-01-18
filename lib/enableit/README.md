# enableit-platform

The `enableit-platform` library provides core software components for the enable.it bionic platform, supporting bionic and electronically controlled assistive devices based on ESP32 microcontrollers.

## Features

- Hardware abstraction for board and display management
- Real-time EMG (Electromyography) signal acquisition and processing
- Telemetry and data streaming over WiFi (including RTP packet support)
- Command-line interface and configuration management
- Utilities for circular buffering, FFT, QR code generation, and more
- Integration with third-party libraries for device connectivity and control

## Usage

This library is intended for use as a dependency in ESP32-based projects that require robust support for bionic device firmware, including sensor data acquisition, device control, and remote monitoring.

Refer to the source code and examples for integration details.

## Customizing for New Boards

To support a new hardware board, you should:

1. **Create a Board Class**  
   Implement a new class derived from the abstract `Board` class (see `include/Board.h`).  
   This class should implement the required methods:  
   - `begin(bool lcdEnabled = true)`
   - `end()`
   - `getDisplay()`

2. **Implement a Display Class (if needed)**  
   If your board uses a custom display, implement a class derived from the abstract `Display` class (see `include/Display.h`).  
   Implement all pure virtual methods to provide display functionality (text, graphics, etc.).

   **Note:**  
   For boards without a real display, you can use the provided `MockDisplay` class, which implements the `Display` interface with empty methods for testing or headless operation.

3. **Register the Board**  
   Define a global instance of your board class and expose it as `board` (see the example in `include/Board.h`).

4. **Enable Board Selection**  
   Use preprocessor macros (e.g., `ARDUINO_M5STACK_ATOMS3`) to select the appropriate board implementation at compile time.

5. **Example**  
   ```cpp
   // Example: CustomBoard.h
   class CustomBoard : public Board {
   public:
       void begin(bool lcdEnabled = true) override { /* ... */ }
       void end() override { /* ... */ }
       Display& getDisplay() override { return customDisplay; }
   private:
       MockDisplay customDisplay; // Use MockDisplay if no real display is present
   };
   Board& board = customBoardInstance;
   ```

6. **Integration**  
   Update your project configuration to include your new board and display classes.

## Multiple Application Approach

The enableit-platform firmware is designed around a modular, multi-application architecture. Each major functionality is implemented as a separate application, allowing flexible and scalable firmware composition.

### BoardApp: The Application Interface

Each application must implement the abstract `BoardApp` interface (see `include/BoardApp.h`), which defines the following methods:

- `void enter()`: Called when the application becomes active.
- `void process()`: Called periodically to perform the application's main logic.
- `void leave()`: Called when the application is deactivated.
- `const char *name()`: Returns the application's unique name.

### Application Registration

Applications are self-registering. Each application class should instantiate itself as a global object and use the `REGISTER_BOARD_APP` macro to register with the board manager at startup.

**Example (see `src/apps/bootloader/BootLoaderApp.cpp`):**
```cpp
// BootLoaderApp.cpp
#include <BoardAppRegistrar.h>
#include "BootLoaderApp.h"

BootLoaderApp bootloaderApp;
REGISTER_BOARD_APP(bootloaderApp);
```

This mechanism allows the firmware to automatically discover and manage all available applications.

### Modular Firmware Composition

Each application is typically implemented as a separate library or module. By including only the desired application libraries in your project, you can compose the final firmware with just the macro functionalities you need.

- **Add or remove applications** by including or excluding their source files/libraries.
- Each application registers itself, so no manual list or central registry is needed.
- The board manager handles application switching and lifecycle.

This approach enables highly customizable and maintainable firmware, where new features can be added or removed simply by managing application modules.

## License

Creative Commons Attribution-NonCommercial-shareAlike 4.0 International Public License (CC BY-NC-SA 4.0)

For more information, visit [https://e-nable.it/](https://e-nable.it/)


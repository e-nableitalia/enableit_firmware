# Libraries Structure: enable.it Firmware

This directory contains all the libraries and application modules for the enable.it firmware platform. Each library resides in its own subfolder and is built as a static library by PlatformIO.

## Directory Structure

```
lib/
  enableit/            # Core platform library: board abstraction, device drivers, utilities, CLI, telemetry, etc.
  app_bootloader/      # Bootloader application: board initialization, configuration, interactive boot management
  app_emgdemo/         # EMG demo application: real-time EMG acquisition, streaming, command-line control
  app_kinetichand/     # Kinetic Hand application: pressure sensor-based hand control and servo actuation
  app_pressuremouse/   # Pressure Mouse application: pressure sensor-based mouse click simulation
  webupdater/          # Web Updater application: web-based firmware update and file management
  otaupdater/          # OTA Updater application: secure firmware update via HTTPS
```

## Module Descriptions

- **enableit/**  
  The core platform library. Provides hardware abstraction for boards and displays, device drivers (EMG, pressure sensors, servos), configuration management, telemetry, command-line interface, and utilities for all applications.

- **app_bootloader/**  
  Implements the bootloader application. Handles board startup, configuration, WiFi setup, and interactive management via serial console.

- **app_emgdemo/**  
  Implements a demonstration application for real-time EMG acquisition and streaming, with command-line configuration and telemetry.

- **app_kinetichand/**  
  Implements the Kinetic Hand application, enabling pressure sensor-based hand actuation and servo control, with state machine logic.

- **app_pressuremouse/**  
  Implements a pressure sensor-based mouse application, simulating mouse clicks or actions based on analog input.

- **app_webupdater/**  
  Provides a web interface for firmware (OTA) updates and file management (upload, edit, delete) on the device's filesystem.

- **app_otaupdater/**  
  Provides secure OTA firmware updates via HTTPS from a remote server.

## Adding or Customizing Modules

- To add a new application, create a new subfolder (e.g., `app_myfeature/`) and implement the `BoardApp` interface.
- Each application is self-registering and can be included/excluded from the build for modular firmware composition.
- See each module's `README.md` and `library.json` for details.

For more information, visit [https://e-nable.it/](https://e-nable.it/)

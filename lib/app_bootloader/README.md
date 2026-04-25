# enableit-app-bootloader

The `enableit-app-bootloader` library implements the bootloader application for the enable.it platform. It is responsible for board initialization, configuration, and providing an interactive boot management interface.

## Features

- Board hardware and configuration initialization
- Interactive command-line interface via USB serial
- WiFi configuration and management (station and access point modes)
- Telemetry and OTA update support
- Application selection and switching at boot
- Secure and privileged modes for advanced configuration
- Modular registration as a `BoardApp` for use in multi-application firmware

## Usage

This application is intended to be included as a module in the enable.it firmware. It registers itself automatically at startup and can be selected as the active application during the boot process.

Typical usage includes:
- Initial board setup and diagnostics
- Entering configuration commands via serial console
- Selecting which application to run after boot
- Managing WiFi and OTA update settings

## Integration

To include the bootloader application in your firmware, add `app_bootloader` as a dependency in your project and ensure it is linked during the build. The application will self-register and be available for selection at boot.

## Dependencies

- [enableit-platform](../enableit)

## License

Creative Commons Attribution-NonCommercial 4.0 International Public License

For more information, visit [https://e-nable.it/](https://e-nable.it/)

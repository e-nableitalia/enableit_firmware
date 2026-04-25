# enableit-app-otaupdater

The `enableit-app-otaupdater` library implements an OTA (Over-The-Air) firmware updater application for the enable.it platform. It allows secure firmware updates via HTTPS from a remote server.

## Features

- OTA firmware update via HTTPS
- Integration with board configuration for update URL and credentials
- Status and progress reporting via serial console
- Modular registration as a `BoardApp` for use in multi-application firmware

## Usage

This application is intended to be included as a module in the enable.it firmware. It registers itself automatically at startup and can be selected as the active application.

Typical usage includes:
- Securely updating device firmware from a remote server
- Triggering OTA updates via command or application switch

## Integration

To include the OTA updater application in your firmware, add `enableit-app-otaupdater` as a dependency in your project and ensure it is linked during the build. The application will self-register and be available for selection.

## Dependencies

- [enableit-platform](../enableit)

## License

Creative Commons Attribution-NonCommercial 4.0 International Public License

For more information, visit [https://e-nable.it/](https://e-nable.it/)

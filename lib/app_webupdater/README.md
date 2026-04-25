# enableit-app-webupdater

The `enableit-app-webupdater` library implements a web-based firmware updater and file manager application for the enable.it platform.

## Features

- Web interface for firmware (OTA) updates
- File management (upload, edit, delete, list files) on the device's filesystem (SPIFFS)
- User authentication for secure access
- Modular registration as a `BoardApp` for use in multi-application firmware

## Usage

This application is intended to be included as a module in the enable.it firmware. It registers itself automatically at startup and can be selected as the active application.

Typical usage includes:
- Updating firmware via a web browser
- Managing files on the device (edit, upload, delete)
- Secure access with username and password

## Integration

To include the web updater application in your firmware, add `enableit-app-webupdater` as a dependency in your project and ensure it is linked during the build. The application will self-register and be available for selection.

## Dependencies

- [enableit-platform](../enableit)

## License

Creative Commons Attribution-NonCommercial 4.0 International Public License

For more information, visit [https://e-nable.it/](https://e-nable.it/)

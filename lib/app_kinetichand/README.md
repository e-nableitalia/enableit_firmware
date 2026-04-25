# enableit-app-kinetichand

The `enableit-app-kinetichand` library implements the Kinetic Hand application for the enable.it platform. It provides pressure sensor-based hand control and servo actuation, enabling bionic hand functionality.

## Features

- Reads pressure sensor input to determine hand state
- Controls a servo motor for hand opening and closing
- Implements a state machine for progressive and hold-time hand operation
- Displays sensor and servo status on supported displays
- Modular registration as a `BoardApp` for use in multi-application firmware

## Usage

This application is intended to be included as a module in the enable.it firmware. It registers itself automatically at startup and can be selected as the active application.

Typical usage includes:
- Real-time hand actuation based on muscle pressure input
- Visual feedback of pressure and servo state
- Integration with other board applications

## Integration

To include the Kinetic Hand application in your firmware, add `app_kinetichand` as a dependency in your project and ensure it is linked during the build. The application will self-register and be available for selection.

## Dependencies

- [enableit-platform](../enableit)

## License

Creative Commons Attribution-NonCommercial 4.0 International Public License

For more information, visit [https://e-nable.it/](https://e-nable.it/)

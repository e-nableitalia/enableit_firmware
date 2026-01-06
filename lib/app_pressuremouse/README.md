# enableit-app-pressuremouse

The `enableit-app-pressuremouse` library implements a pressure sensor-based mouse application for the enable.it platform. It simulates mouse clicks or actions based on input from a pressure sensor, enabling assistive or experimental device control.

## Features

- Reads analog input from a pressure sensor (e.g., MD30-60)
- Maps pressure values to display output and (optionally) BLE mouse actions
- Visualizes pressure data on supported displays
- Modular registration as a `BoardApp` for use in multi-application firmware

## Usage

This application is intended to be included as a module in the enable.it firmware. It registers itself automatically at startup and can be selected as the active application.

Typical usage includes:
- Real-time pressure monitoring and visualization
- Simulated mouse click or control based on pressure threshold (BLE mouse support)
- Integration with other board applications

## Integration

To include the Pressure Mouse application in your firmware, add `enableit-app-pressuremouse` as a dependency in your project and ensure it is linked during the build. The application will self-register and be available for selection.

## Dependencies

- [enableit-platform](../enableit)

## License

Creative Commons Attribution-NonCommercial 4.0 International Public License

For more information, visit [https://e-nable.it/](https://e-nable.it/)

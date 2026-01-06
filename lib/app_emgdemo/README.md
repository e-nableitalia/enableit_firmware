# enableit-app-emgdemo

The `enableit-app-emgdemo` library implements a demonstration application for real-time EMG (Electromyography) acquisition and streaming on the enable.it platform.

## Features

- Real-time EMG signal acquisition from ADS129X-based hardware
- Command-line interface for configuration and control via serial console
- Streaming of EMG data over UDP or to ThingsBoard telemetry
- Channel source and gain configuration
- Test signal generation and self-test routines
- Application switching and modular registration as a `BoardApp`

## Usage

This application is intended to be included as a module in the enable.it firmware. It registers itself automatically at startup and can be selected as the active application.

Typical usage includes:
- Acquiring and streaming EMG signals for research or prosthetic control
- Configuring acquisition parameters and streaming targets via serial commands
- Switching between different board applications

## Integration

To include the EMG demo application in your firmware, add `app_emgdemo` as a dependency in your project and ensure it is linked during the build. The application will self-register and be available for selection.

## Dependencies

- [enableit-platform](../enableit)

## License

Creative Commons Attribution-NonCommercial 4.0 International Public License

For more information, visit [https://e-nable.it/](https://e-nable.it/)

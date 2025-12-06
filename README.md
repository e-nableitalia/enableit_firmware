# enableit_firmware

enable-it Bionic Boards - Third Iteration  
This project is part of the e-Nable.it Bionic Platform

## Overview

This repository contains the source code for the firmware powering the enable-it Bionic Boards, developed as part of the e-Nable.it Bionic Platform. The platform is an open-source initiative focused on creating prosthetic and assistive devices through frugal innovation, enhancing mechanical designs with advanced, human-centered electronic control while maintaining simplicity, affordability, and accessibility.

The firmware is designed to run on custom electronic modules that interface with mechanical prosthetic devices, providing features such as actuator control, sensor integration, and modular expandability.

## Community & Development

This project is developed and maintained by volunteers from the e-Nable Italia community, including biomedical engineers, mechatronics engineers, and makers. Our goal is to empower users with open-source, affordable, and customizable assistive devices.

For more information about the e-Nable.it Bionic Platform, visit the [e-nable.it Bionic Platform Wiki](https://e-nableitalia.it/).

## Project Roadmap

The project is structured into three development iterations:

1. **Ideas Validation**  
   Experiments and tests to validate initial concepts, exploring actuator and sensor control options, and collecting data to guide prototyping.

2. **Prototyping & Custom Solutions**  
   Development of custom solutions using commercial components and specialized boards, focusing on simplicity, reliability, and adaptability.

3. **Frugal Innovation Engineering**  
   Engineering optimized, scalable, and cost-effective solutions, creating an open-source hardware platform for modular bionic devices.

**This repository relates to Iteration #3: reusable deliverables for advanced functionalities in bionic devices.**

## Firmware Modules

The firmware supports the following hardware modules:

- **Bionic Controller**  
  Custom control board for M5Stack Atom S3, with battery management, power outputs, and expansion slot for daughter boards.

- **ActuatorSense Module**  
  Compact H-bridge daughter board for controlling two linear actuators or motors, and supporting sensor integration.

- **Advanced Control Board**  
  Enhanced controller with three daughter board slots for multi-actuator control.

- **TI ADS1298 Daughter Board**  
  Specialized board for EMG signal acquisition (up to eight channels).

## Getting Started

- Clone this repository.
- Refer to the hardware documentation for board setup and connections.
- Build and flash the firmware to your board using PlatformIO or Arduino IDE.
- Use the provided command interface to control actuators, configure settings, and test functionalities.

## Caution

The boards and firmware in this repository are under development and are intended as technological demonstrators. They are not designed for practical use until fully validated.

## Project Updates

### Firmware History

- **2023-02-26** – Initial commit and first upload of firmware
- **2023-03-10** – Added initial ADS1298 support and EMGApp
- **2023-04-10** – Improved ADS1298 communication  
  &nbsp;&nbsp;• Fixed circular buffer logic  
  &nbsp;&nbsp;• Introduced ThingsBoard support  
  &nbsp;&nbsp;• Added RTOS-based streaming in EMGApp
- **2023-10-30** – Major update: applications, structure refactor  
  &nbsp;&nbsp;• Added pressure apps and BTmouse app  
  &nbsp;&nbsp;• Added support for M5 Atom S3 board  
  &nbsp;&nbsp;• Updated OTAWebUpdater  
  &nbsp;&nbsp;• Added Kinetic Hand support  
  &nbsp;&nbsp;• Replaced debug.h with console  
  &nbsp;&nbsp;• Added telnet console
- **2023-11-02** – Updates on config, bootloader and OTAWebUpdater
- **2024-01-23** – ADS1298 integration merged into main
- **2025-02-04** – Improvements to EMG and bootloader logic  
  &nbsp;&nbsp;• Added EMGFilters  
  &nbsp;&nbsp;• Updated ADS129X interface  
  &nbsp;&nbsp;• Sample rate changed to 1 kHz  
  &nbsp;&nbsp;• Streaming only active channels  
  &nbsp;&nbsp;• New EMGApp modes (sequence capture, offset calc)  
  &nbsp;&nbsp;• Bootloader command parsing refactor + ThingsBoard enhancements  
  &nbsp;&nbsp;• Added WiFi event handling app  
  &nbsp;&nbsp;• Updated PlatformIO config
- **2025-02-04** – HDLC protocol implementation added  
  &nbsp;&nbsp;• Framing  
  &nbsp;&nbsp;• Transport interface  
  &nbsp;&nbsp;• Command mode handling
- **2025-06-14** – New motor subsystem introduced  
  &nbsp;&nbsp;• Added Motor and BoardTest apps  
  &nbsp;&nbsp;• Speed/direction/sleep control  
  &nbsp;&nbsp;• Analog testing and INA219 configuration  
  &nbsp;&nbsp;• Bootloader shows firmware version + app list  
  &nbsp;&nbsp;• Refactor of EMG components (BufferProducer)  
  &nbsp;&nbsp;• Updated PlatformIO config  
  &nbsp;&nbsp;• Improved command parser for new apps
- **2025-08-20** – Refactor Motor class  
  &nbsp;&nbsp;• Multi-motor support  
  &nbsp;&nbsp;• Initial compatibility with DRV8411 Rev. B
- **2025-12-06** – Added SMotor class and extended MotorApp commands  
  &nbsp;&nbsp;• Waveshare SC Servo support  
  &nbsp;&nbsp;• New motor commands

**Known Issues:**  
N/A

## Authors

- Alberto Navatta - alberto@e-nableitalia.it / alberto.navatta@gmail.com
- e-Nable Italia - info@e-nableitalia.it

For updates and further information, visit: [https://e-nableitalia.it/](https://e-nableitalia.it/)

## License

This material is released under Creative Commons - Attribution - Non-Commercial - Share Alike license.

### Limitation of Liability

UNDER NO CIRCUMSTANCES AND UNDER NO LEGAL THEORY, WHETHER TORT (INCLUDING NEGLIGENCE), CONTRACT, OR OTHERWISE, SHALL YOU, THE INITIAL DEVELOPER, ANY OTHER CONTRIBUTOR, OR ANY DISTRIBUTOR OF COVERED CODE, OR ANY SUPPLIER OF ANY OF SUCH PARTIES, BE LIABLE TO ANY PERSON FOR ANY INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES OF ANY CHARACTER INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF GOODWILL, WORK STOPPAGE, COMPUTER FAILURE OR MALFUNCTION, OR ANY AND ALL OTHER COMMERCIAL DAMAGES OR LOSSES, EVEN IF SUCH PARTY SHALL HAVE BEEN INFORMED OF THE POSSIBILITY OF SUCH DAMAGES. THIS LIMITATION OF LIABILITY SHALL NOT APPLY TO LIABILITY FOR DEATH OR PERSONAL INJURY RESULTING FROM SUCH PARTY'S NEGLIGENCE TO THE EXTENT APPLICABLE LAW PROHIBITS SUCH LIMITATION. SOME JURISDICTIONS DO NOT ALLOW THE EXCLUSION OR LIMITATION OF INCIDENTAL OR CONSEQUENTIAL DAMAGES, SO THIS EXCLUSION AND LIMITATION MAY NOT APPLY TO YOU.
/**
 * @file main.cpp
 * @brief Demonstrates how firmware can be composed using a base library that manages board-specific capabilities,
 *        abstracts hardware through a Hardware Abstraction Layer (HAL), and integrates multiple applications to extend functionality.
 *
 * This example initializes the eBoard firmware by registering two applications (DemoApp and KinetixApp),
 * and configures the bootloader to start the Kinetix application at device startup.
 * The BoardManager library is used to handle board initialization and application lifecycle,
 * showcasing modular firmware design for extensibility and maintainability.
 *
 * Author: A.Navatta / e-Nable Italia
 */
#include <enableit.h>

ENABLE_BOARD_APP(OtaApp);
ENABLE_BOARD_APP(DemoApp);
ENABLE_BOARD_APP(KinetixApp);

void setup() {
    ENABLEIT_BOOT("kinetix");
}

void loop() {
    ENABLEIT_LOOP();
}
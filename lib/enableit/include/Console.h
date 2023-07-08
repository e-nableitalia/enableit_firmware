#include <Arduino.h>
#include <USB.h>

#define USE_USB_SERIAL

#if ARDUINO_USB_CDC_ON_BOOT
#ifdef USE_USB_SERIAL
#define HWSerial Serial0
#define USBSerial Serial
#define DBGSerial Serial
#else
#define HWSerial Serial0
#define USBSerial Serial
#define DBGSerial Serial0
#endif
#else
// Console serial
#define Console Serial

#endif
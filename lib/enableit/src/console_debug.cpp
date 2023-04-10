

#include <stdarg.h>
#include <stdio.h>

#include "debug.h"

#if defined (STM32F2XX)	// Photon
#include <particle.h>
#else
#include <Arduino.h>
#endif

#include <Console.h>

// console output format buffer
char format_buffer[FORMAT_BUFFERSIZE];

bool debug_enabled = true;

#ifdef USE_USB_SERIAL
static void usbEventCallback(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
  if (event_base == ARDUINO_USB_EVENTS) {
    arduino_usb_event_data_t * data = (arduino_usb_event_data_t*)event_data;
    switch (event_id){
      case ARDUINO_USB_STARTED_EVENT:
        DBG("USB PLUGGED");
        break;
      case ARDUINO_USB_STOPPED_EVENT:
        DBG("USB UNPLUGGED");
        break;
      case ARDUINO_USB_SUSPEND_EVENT:
        DBG("USB SUSPENDED: remote_wakeup_en: %u\n", data->suspend.remote_wakeup_en);
        break;
      case ARDUINO_USB_RESUME_EVENT:
        DBG("USB RESUMED");
        break;
      
      default:
        break;
    }
  } else if (event_base == ARDUINO_USB_CDC_EVENTS) {
    arduino_usb_cdc_event_data_t * data = (arduino_usb_cdc_event_data_t*)event_data;
    switch (event_id){
      case ARDUINO_USB_CDC_CONNECTED_EVENT:
        DBG("CDC CONNECTED");
        break;
      case ARDUINO_USB_CDC_DISCONNECTED_EVENT:
        DBG("CDC DISCONNECTED");
        break;
      case ARDUINO_USB_CDC_LINE_STATE_EVENT:
        DBG("CDC LINE STATE: dtr: %u, rts: %u\n", data->line_state.dtr, data->line_state.rts);
        break;
      case ARDUINO_USB_CDC_LINE_CODING_EVENT:
        DBG("CDC LINE CODING: bit_rate: %u, data_bits: %u, stop_bits: %u, parity: %u\n", data->line_coding.bit_rate, data->line_coding.data_bits, data->line_coding.stop_bits, data->line_coding.parity);
        break;
      case ARDUINO_USB_CDC_RX_EVENT:
/*        DBG("CDC RX [%u]:", data->rx.len);
        {
            uint8_t buf[data->rx.len];
            size_t len = USBSerial.read(buf, data->rx.len);
            HWSerial.write(buf, len);
        }
        HWSerial.println();
*/        
        break;
       case ARDUINO_USB_CDC_RX_OVERFLOW_EVENT:
        DBG("CDC RX Overflow of %d bytes", data->rx_overflow.dropped_bytes);
        break;
     
      default:
        break;
    }
  }
}
#endif

void debug_enable(bool d) {
  debug_enabled = d;
  
  Console.begin(115200);
  Console.setDebugOutput(true);

#ifdef USE_USB_SERIAL  
  USB.onEvent(usbEventCallback);

  USBSerial.onEvent(usbEventCallback);
  USBSerial.begin(115200);

  USB.begin();
#endif
}

// Serial debug function
// print function name and formatted text
// accepted format arguments:
// %d, %i -> integer
// %s -> string
void console_debug(const bool dbg, const char *prefix, bool addlf, const char *function, const char *format_str, ...) {
  
    if ((dbg) && (!debug_enabled)) return;

    va_list argp;
    va_start(argp, format_str);
    char *bp=format_buffer;
    int bspace = FORMAT_BUFFERSIZE - 1;

    if ((prefix) && (addlf)) {
      while ((*prefix) && (bspace)) {
        *bp++ = *prefix++;
        --bspace;
      }
      *bp++ = ':';
      --bspace;
    }

    if ((function) && (addlf)) {
      while ((*function) && (bspace)) {
        *bp++ = *function++;
        --bspace;
      }

      *bp++ = ':';
      --bspace;
      *bp++ = ' ';
      --bspace;
    }

    while (*format_str != '\0' && bspace > 0) {
      if (*format_str != '%') {
        *bp++ = *format_str++;
        --bspace;
      } else if (format_str[1] == '%') // An "escaped" '%' (just print one '%').
      {
        *bp++ = *format_str++;    // Store first %
        ++format_str;             // but skip second %
        --bspace;
      } else {
         ++format_str;
        // parse format
        switch (*format_str) {
          case 's': {
            // string
            char *str = va_arg (argp, char *);
            while ((*str) && (bspace)) {
              *bp++ = *str++;
              --bspace;
            }
          };
          break;
          case 'd': case 'i': {
            // decimal
            char ibuffer[16];
            int val = va_arg (argp, int);
            snprintf(ibuffer,16,"%d",val);
            char *str = ibuffer;
            while ((*str) && (bspace)) {
              *bp++ = *str++;
              --bspace;
            }
          };
          break;
          case 'u': {
            // decimal
            char ibuffer[16];
            unsigned int val = va_arg (argp, unsigned int);
            snprintf(ibuffer,16,"%u",val);
            char *str = ibuffer;
            while ((*str) && (bspace)) {
              *bp++ = *str++;
              --bspace;
            }
          };
          break;
          case 'x': {
            // decimal
            char ibuffer[16];
            unsigned int val = va_arg (argp, unsigned int);
            snprintf(ibuffer,16,"%x",val);
            char *str = ibuffer;
            while ((*str) && (bspace)) {
              *bp++ = *str++;
              --bspace;
            }
          };
          break; 
          case 'f': {
            // decimal
            char ibuffer[16];
            double val = va_arg (argp, double);
            snprintf(ibuffer,16,"%f",val);
            char *str = ibuffer;
            while ((*str) && (bspace)) {
              *bp++ = *str++;
              --bspace;
            }
          };
          break;                    
          default: {
            // skip format
          }
        }
         
        ++format_str;
      }
    }
    // terminate string
    *bp = 0;
    if (addlf) {
      Console.println(format_buffer);
    } else {
      Console.print(format_buffer);
    }
}
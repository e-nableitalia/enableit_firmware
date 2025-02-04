#include <Arduino.h>
#include <USB.h>
#include <ESPTelnetStream.h>

#include <stdarg.h>
#include <stdio.h>

#include <Console.h>

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
// Hardware serial
#define HWSerial Serial
#endif

#if !defined(NO_GLOBAL_INSTANCES)
ConsoleWrapper Console;
#endif

ESPTelnetStream telnetServer;

// console output format buffer
char format_buffer[FORMAT_BUFFERSIZE];

bool ConsoleWrapper::telnet_connected = false;

//bool debug_enabled = true;

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

void ConsoleWrapper::init(int baudRate, bool d, bool t) {

  debug_enabled = d;

  HWSerial.begin(baudRate);
  HWSerial.setDebugOutput(debug_enabled);

#ifdef USE_USB_SERIAL  

  if (debug_enabled) {
      USB.onEvent(usbEventCallback);
      USBSerial.onEvent(usbEventCallback);
  }

  USB.begin();

  USBSerial.begin(baudRate);

#endif

  telnet = t;
  telnet_connected = false;

  if (telnet) {
    DBG("enabling telnet server");
    telnetSetup();
  }
}

void ConsoleWrapper::enableTelnet(bool t) {
  if (t) {
    if (!telnet) {
      DBG("Enabling telnet server");
      telnetSetup();
      telnet_connected = false;
    } else {
      DBG("Telnet server already enabled");
    }
  } else {
    if (telnet) {
      DBG("Disconnecting clients and disabling telnet server");
      telnetServer.disconnectClient(true);
      telnetServer.stop();
      telnet_connected = false;
      telnet = false;
    } else {
      DBG("Telnet server already disabled");
    }
  }
  telnet = t;
}

void ConsoleWrapper::telnetSetup() {
  telnetServer.onConnect(&ConsoleWrapper::onTelnetConnect);
  telnetServer.onConnectionAttempt(&ConsoleWrapper::onTelnetConnectionAttempt);
  telnetServer.onReconnect(&ConsoleWrapper::onTelnetReconnect);
  telnetServer.onDisconnect(&ConsoleWrapper::onTelnetDisconnect);
  //telnetServer.onInputReceived(&ConsoleWrapper::onTelnetInput);

  DBGNOLF("Telnet Server: ");
  if (telnetServer.begin()) {
    DBG("running");
  } else {
    DBG("error.");
  }
}

void ConsoleWrapper::onTelnetConnect(String ip) {
  DBG("Telnet connected from[%s]",ip.c_str());
  DBG("Disabling serial I/O, switching to telnet");
  
  telnetServer.println("\nWelcome " + telnetServer.getIP());
  telnetServer.println("(Use ^] + q  to disconnect.)");

  telnet_connected = true;
}

void ConsoleWrapper::onTelnetDisconnect(String ip) {
  DBGNOLF("Telnet: ");
  DBGNOLF(ip.c_str());
  DBG(" disconnected");

  telnet_connected = false;
}

void ConsoleWrapper::onTelnetReconnect(String ip) {
  DBGNOLF("Telnet: ");
  DBGNOLF(ip.c_str());
  DBG(" reconnected");
}

void ConsoleWrapper::onTelnetConnectionAttempt(String ip) {
  DBGNOLF("Telnet: ");
  DBGNOLF(ip.c_str());
  DBG(" tried to connected");
}

void ConsoleWrapper::onTelnetInput(String str) {
  HWSerial.print("Received[");
  HWSerial.print(str.c_str());
  HWSerial.println("]");
}

void ConsoleWrapper::pool() {
    if (telnet)
      telnetServer.loop();
}

size_t ConsoleWrapper::write(uint8_t c) {
  if (telnet_connected)
    return telnetServer.write(c);
  else
    return HWSerial.write(c);
}

int ConsoleWrapper::available() {
  if (telnet_connected) {
    return telnetServer.available();
  } else  
    return HWSerial.available();
}

int ConsoleWrapper::read() {
  if (telnet_connected) {
    return telnetServer.read();
  } else  
    return HWSerial.read();
}

void ConsoleWrapper::echo(uint8_t c) {
  if (!telnet_connected)
    HWSerial.write(c);
}

int ConsoleWrapper::peek() {
  if (telnet_connected)
    return telnetServer.peek();
  else  
    return HWSerial.peek();
}

// Serial debug function
// print function name and formatted text
// accepted format arguments:
// %d, %i -> integer
// %s -> string
void ConsoleWrapper::debug(const bool dbg, const char *prefix, bool addlf, const char *function, const char *format_str, ...) {
  
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
          case 'F': {
            // decimal
            char ibuffer[16];
            double val = va_arg (argp, double);
            snprintf(ibuffer,16,"%f",val);
            // Sostituisci il punto con la virgola
            for (char *p = ibuffer; *p != '\0'; p++) {
                if (*p == '.') {
                    *p = ',';
                    break;
                }
            }
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
      println(format_buffer);
    } else {
      print(format_buffer);
    }
}



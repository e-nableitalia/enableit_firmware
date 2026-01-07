#include <Arduino.h>
#include <USB.h>
#include <ESPTelnetStream.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>

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

// console output format buffer
char format_buffer[FORMAT_BUFFERSIZE];

#ifdef USE_USB_SERIAL
static void usbEventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_base == ARDUINO_USB_EVENTS)
  {
    arduino_usb_event_data_t *data = (arduino_usb_event_data_t *)event_data;
    switch (event_id)
    {
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
  }
  else if (event_base == ARDUINO_USB_CDC_EVENTS)
  {
    arduino_usb_cdc_event_data_t *data = (arduino_usb_cdc_event_data_t *)event_data;
    switch (event_id)
    {
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

void ConsoleWrapper::init(int baudRate, bool d, bool t)
{
  debug_enabled = d;

  // Register Serial transport
  static SerialConsoleTransport serialTransport(HWSerial);
  registerTransport(&serialTransport, PRIORITY_SERIAL);
  HWSerial.begin(baudRate);
  HWSerial.setDebugOutput(debug_enabled);

#ifdef USE_USB_SERIAL
  if (debug_enabled)
  {
    USB.onEvent(usbEventCallback);
    USBSerial.onEvent(usbEventCallback);
  }
  USB.begin();
  USBSerial.begin(baudRate);
#endif

  if (t)
  {
    static TelnetConsoleTransport telnetTransport;
    registerTransport(&telnetTransport, PRIORITY_TELNET);
    telnetTransport.attach(this);
    telnetTransport.enable(true);
  }

  selectActiveTransport();
}

void ConsoleWrapper::poll()
{
  for (auto &entry : transports)
  {
    if (entry.transport->needsPoll())
    {
      entry.transport->poll();
    }
  }
  selectActiveTransport();
}

size_t ConsoleWrapper::write(uint8_t c)
{
  selectActiveTransport();
  if (activeTransport)
  {
    return activeTransport->write(c);
  }
  // Fallback to Serial
  return HWSerial.write(c);
}

int ConsoleWrapper::available()
{
  selectActiveTransport();
  if (activeTransport)
  {
    return activeTransport->available();
  }
  return HWSerial.available();
}

int ConsoleWrapper::read()
{
  selectActiveTransport();
  if (activeTransport)
  {
    return activeTransport->read();
  }
  return HWSerial.read();
}

void ConsoleWrapper::echo(uint8_t c)
{
  selectActiveTransport();
  if (activeTransport && activeTransport->getPriority() == PRIORITY_SERIAL)
  {
    activeTransport->write(c);
  }
}

int ConsoleWrapper::peek()
{
  selectActiveTransport();
  if (activeTransport)
  {
    return activeTransport->peek();
  }
  return HWSerial.peek();
}

// Serial debug function
// print function name and formatted text
// accepted format arguments:
// %d, %i -> integer
// %s -> string
void ConsoleWrapper::debug(const bool dbg, const char *prefix, bool addlf, const char *function, const char *format_str, ...)
{
  if ((dbg) && (!debug_enabled))
    return;

  va_list argp;
  va_start(argp, format_str);
  char *bp = format_buffer;
  int bspace = FORMAT_BUFFERSIZE - 1;

  if ((prefix) && (addlf))
  {
    while ((*prefix) && (bspace))
    {
      *bp++ = *prefix++;
      --bspace;
    }
    *bp++ = ':';
    --bspace;
  }

  if ((function) && (addlf))
  {
    while ((*function) && (bspace))
    {
      *bp++ = *function++;
      --bspace;
    }

    *bp++ = ':';
    --bspace;
    *bp++ = ' ';
    --bspace;
  }

  while (*format_str != '\0' && bspace > 0)
  {
    if (*format_str != '%')
    {
      *bp++ = *format_str++;
      --bspace;
    }
    else if (format_str[1] == '%')
    {
      *bp++ = *format_str++;
      ++format_str;
      --bspace;
    }
    else
    {
      ++format_str;
      switch (*format_str)
      {
      case 's':
      {
        char *str = va_arg(argp, char *);
        while ((*str) && (bspace))
        {
          *bp++ = *str++;
          --bspace;
        }
      }
      break;
      case 'd':
      case 'i':
      {
        char ibuffer[16];
        int val = va_arg(argp, int);
        snprintf(ibuffer, 16, "%d", val);
        char *str = ibuffer;
        while ((*str) && (bspace))
        {
          *bp++ = *str++;
          --bspace;
        }
      }
      break;
      case 'u':
      {
        char ibuffer[16];
        unsigned int val = va_arg(argp, unsigned int);
        snprintf(ibuffer, 16, "%u", val);
        char *str = ibuffer;
        while ((*str) && (bspace))
        {
          *bp++ = *str++;
          --bspace;
        }
      }
      break;
      case 'x':
      {
        char ibuffer[16];
        unsigned int val = va_arg(argp, unsigned int);
        snprintf(ibuffer, 16, "%x", val);
        char *str = ibuffer;
        while ((*str) && (bspace))
        {
          *bp++ = *str++;
          --bspace;
        }
      }
      break;
      case 'f':
      {
        char ibuffer[16];
        double val = va_arg(argp, double);
        snprintf(ibuffer, 16, "%f", val);
        char *str = ibuffer;
        while ((*str) && (bspace))
        {
          *bp++ = *str++;
          --bspace;
        }
      }
      break;
      default:
      {
        // skip format
      }
      }
      ++format_str;
    }
  }
  *bp = 0;
  size_t len = strlen(format_buffer);
  for (size_t i = 0; i < len; ++i)
  {
    write((uint8_t)format_buffer[i]);
  }
  if (addlf)
  {
    write((uint8_t)'\n');
  }
}
// Transport registration
void ConsoleWrapper::registerTransport(ConsoleTransport *transport, ConsolePriority priority)
{
  if (!transport)
    return;
  TransportEntry entry;
  entry.transport = transport;
  entry.priority = priority;
  entry.state = transport->isConnected() ? CONSOLE_CONNECTED : CONSOLE_DISCONNECTED;
  transports.push_back(entry);
  transport->attach(this);
  selectActiveTransport();
}

void ConsoleWrapper::unregisterTransport(ConsoleTransport *transport)
{
  for (auto it = transports.begin(); it != transports.end(); ++it)
  {
    if (it->transport == transport)
    {
      transports.erase(it);
      break;
    }
  }
  selectActiveTransport();
}

void ConsoleWrapper::onTransportConnected(ConsoleTransport *transport)
{
  for (auto &entry : transports)
  {
    if (entry.transport == transport)
    {
      entry.state = CONSOLE_CONNECTED;
      break;
    }
  }
  selectActiveTransport();
}

void ConsoleWrapper::onTransportDisconnected(ConsoleTransport *transport)
{
  for (auto &entry : transports)
  {
    if (entry.transport == transport)
    {
      entry.state = CONSOLE_DISCONNECTED;
      break;
    }
  }
  selectActiveTransport();
}

void ConsoleWrapper::selectActiveTransport()
{
  ConsoleTransport *best = nullptr;
  int bestPriority = 0;
  for (auto &entry : transports)
  {
    if (entry.state == CONSOLE_CONNECTED && entry.priority > bestPriority)
    {
      best = entry.transport;
      bestPriority = entry.priority;
    }
  }
  activeTransport = best;
}

ConsoleTransport *ConsoleWrapper::getActiveTransport() const
{
  return activeTransport;
}
// ConsoleTransport event helpers
void ConsoleTransport::notifyConnect()
{
  if (wrapper)
    wrapper->onTransportConnected(this);
}
void ConsoleTransport::notifyDisconnect()
{
  if (wrapper)
    wrapper->onTransportDisconnected(this);
}

// TelnetConsoleTransport implementation
#include <ESPTelnetStream.h>
static ESPTelnetStream telnetServer;

// Singleton instance pointer
TelnetConsoleTransport* TelnetConsoleTransport::_instance = nullptr;

// Static callback wrappers for ESPTelnetStream
void TelnetConsoleTransport::onConnectStatic(String ip) {
    if (_instance) _instance->onConnect();
}
void TelnetConsoleTransport::onDisconnectStatic(String ip) {
    if (_instance) _instance->onDisconnect();
}
void TelnetConsoleTransport::onReconnectStatic(String ip) {
    if (_instance) _instance->onReconnect();
}
void TelnetConsoleTransport::onConnectionAttemptStatic(String ip) {
    if (_instance) _instance->onConnectionAttempt();
}
void TelnetConsoleTransport::onInputReceivedStatic(String str) {
    if (_instance) _instance->onInput(str);
}

TelnetConsoleTransport::TelnetConsoleTransport()
{
    connected = false;
    _instance = this;
}
bool TelnetConsoleTransport::available() { return connected ? telnetServer.available() : 0; }
int TelnetConsoleTransport::read() { return connected ? telnetServer.read() : -1; }
size_t TelnetConsoleTransport::write(uint8_t c) { return connected ? telnetServer.write(c) : 0; }
bool TelnetConsoleTransport::isConnected() { return connected; }
int TelnetConsoleTransport::peek() { return connected ? telnetServer.peek() : -1; }
void TelnetConsoleTransport::poll() { telnetServer.loop(); }
void TelnetConsoleTransport::enable(bool enable)
{
    if (enable)
    {
        telnetServer.onConnect(&TelnetConsoleTransport::onConnectStatic);
        telnetServer.onDisconnect(&TelnetConsoleTransport::onDisconnectStatic);
        telnetServer.onReconnect(&TelnetConsoleTransport::onReconnectStatic);
        telnetServer.onConnectionAttempt(&TelnetConsoleTransport::onConnectionAttemptStatic);
        telnetServer.onInputReceived(&TelnetConsoleTransport::onInputReceivedStatic);
        telnetServer.begin();
    }
    else
    {
        telnetServer.disconnectClient(true);
        telnetServer.stop();
        connected = false;
        notifyDisconnect();
    }
}

void TelnetConsoleTransport::onConnect()
{
    connected = true;
    telnetServer.println("\nWelcome " + telnetServer.getIP());
    telnetServer.println("(Use ^] + q  to disconnect.)");
    notifyConnect();
}
void TelnetConsoleTransport::onDisconnect()
{
    connected = false;
    notifyDisconnect();
}
void TelnetConsoleTransport::onReconnect()
{
    connected = true;
    notifyConnect();
}
void TelnetConsoleTransport::onConnectionAttempt()
{
    // Optionally handle connection attempts
}
void TelnetConsoleTransport::onInput(String str)
{
    // Optionally handle input
}

#include <Arduino.h>
#include <USB.h>
#include <ESPTelnetStream.h>

#include <stdarg.h>
#include <stdio.h>

#include <Console.h>
#include <Board.h>

using namespace enableit;

namespace enableit {

// console output format buffer
char format_buffer[FORMAT_BUFFERSIZE];

void ConsoleWrapper::init(int baudRate, bool d, bool t)
{
  log_d("ConsoleWrapper Init: baudRate=%d, debug=%s, telnet=%s", baudRate, d ? "true" : "false", t ? "true" : "false");
  debug_enabled = d;

  log_d("Serial begin at %d baud", baudRate);
  board.serial().begin(baudRate);
  log_d("Registering Serial transport");
  // Register Serial transport using board's default serial port
  registerTransport(&board.serial(), PRIORITY_SERIAL);
  log_d("Notifying Serial transport connected");
  // force notify connect for serial to be active by default
  board.serial().notifyConnect();
  
  if (t)
  {
    static TelnetConsoleTransport telnetTransport;
    registerTransport(&telnetTransport, PRIORITY_TELNET);
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
  return board.serial().write(c);
}

int ConsoleWrapper::available()
{
  selectActiveTransport();
  if (activeTransport)
  {
    return activeTransport->available();
  }
  return board.serial().available();
}

int ConsoleWrapper::read()
{
  selectActiveTransport();
  if (activeTransport)
  {
    return activeTransport->read();
  }
  return board.serial().read();
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
  return board.serial().peek();
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

  log_i("Registering transport with priority %d", priority);
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

ConsoleWrapper Console;

} // namespace enableit

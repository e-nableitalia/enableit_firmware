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
  
  // Always register the telnet transport (singleton), enable server only if requested
  registerTransport(TelnetConsoleTransport::instance(), PRIORITY_TELNET);
  if (t)
  {
    TelnetConsoleTransport::instance()->enable(true);
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
}

size_t ConsoleWrapper::write(uint8_t c)
{
  if (activeTransport)
  {
    return activeTransport->write(c);
  }
  // Fallback to Serial
  return board.serial().write(c);
}

int ConsoleWrapper::available()
{
  if (activeTransport)
  {
    return activeTransport->available();
  }
  return board.serial().available();
}

int ConsoleWrapper::read()
{
  if (activeTransport)
  {
    return activeTransport->read();
  }
  return board.serial().read();
}

void ConsoleWrapper::echo(uint8_t c)
{
  if (activeTransport)
  {
    activeTransport->write(c);
  }
}

int ConsoleWrapper::peek()
{
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

  log_i("Registering transport %s with priority %d", transport->getName().c_str(), priority);
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
      log_i("Unregistering transport %s", transport->getName().c_str());
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
      log_i("Transport %s connected", transport->getName().c_str());
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
      log_i("Transport %s disconnected", transport->getName().c_str());
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
      log_i("Selecting transport %s with priority %d", entry.transport->getName().c_str(), entry.priority);
      best = entry.transport;
      bestPriority = entry.priority;
    }
  }
  activeTransport = best;
  if (!activeTransport)
  {
    log_i("No active transport, defaulting to Serial");
    activeTransport = &board.serial();
  }
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

// Singleton instance
TelnetConsoleTransport* TelnetConsoleTransport::_instance = nullptr;

TelnetConsoleTransport* TelnetConsoleTransport::instance() {
    static TelnetConsoleTransport _singleton;
    return &_singleton;
}

// Static callback wrappers for ESPTelnetStream — use instance() directly (always valid)
void TelnetConsoleTransport::onConnectStatic(String ip) {
    log_d("Telnet onConnectStatic ip=%s", ip.c_str());
    instance()->onConnect();
}
void TelnetConsoleTransport::onDisconnectStatic(String ip) {
    log_d("Telnet onDisconnectStatic ip=%s", ip.c_str());
    instance()->onDisconnect();
}
void TelnetConsoleTransport::onReconnectStatic(String ip) {
    log_d("Telnet onReconnectStatic ip=%s", ip.c_str());
    instance()->onReconnect();
}
void TelnetConsoleTransport::onConnectionAttemptStatic(String ip) {
    log_d("Telnet onConnectionAttemptStatic ip=%s", ip.c_str());
    instance()->onConnectionAttempt();
}
void TelnetConsoleTransport::onInputReceivedStatic(String str) {
    log_d("Telnet onInputReceivedStatic str='%s'", str.c_str());
    instance()->onInput(str);
}

TelnetConsoleTransport::TelnetConsoleTransport() : ConsoleTransport("Telnet Console")
{
    connected = false;
    _instance = this;
    log_d("TelnetConsoleTransport created");
}
bool TelnetConsoleTransport::available() {
    if (!connected) return false;
    int n = telnetServer.available();
    //if (n > 0) log_d("Telnet available=%d", n);
    return n > 0;
}
int TelnetConsoleTransport::read() {
    if (!connected) return -1;
    int c = telnetServer.read();
    //if (c >= 0) log_d("Telnet read=0x%02x '%c'", c, (c >= 32 && c < 127) ? c : '?');
    return c;
}
size_t TelnetConsoleTransport::write(uint8_t c) {
    if (!connected) return 0;
    //log_d("Telnet write=0x%02x", c);
    return telnetServer.write(c);
}
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
        // onInputReceived NOT registered: ESPTelnetStream would consume the first char
        // of each line before passing it to the callback, breaking available()/read() flow.
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
    log_d("Telnet client connected from %s", telnetServer.getIP().c_str());
    connected = true;
    telnetServer.println("\nWelcome " + telnetServer.getIP());
    telnetServer.println("(Use ^] + q  to disconnect.)");
    notifyConnect();
    log_d("Telnet notifyConnect done, transport now active");
}
void TelnetConsoleTransport::onDisconnect()
{
    log_d("Telnet client disconnected");
    connected = false;
    notifyDisconnect();
    log_d("Telnet notifyDisconnect done, Serial should become active");
}
void TelnetConsoleTransport::onReconnect()
{
    log_d("Telnet client reconnected from %s", telnetServer.getIP().c_str());
    connected = true;
    notifyConnect();
}
void TelnetConsoleTransport::onConnectionAttempt()
{
    log_d("Telnet connection attempt from %s", telnetServer.getIP().c_str());
}
void TelnetConsoleTransport::onInput(String str)
{
    // ESPTelnetStream buffers input for available()/read() automatically.
    // This callback is informational only.
    log_d("Telnet onInput str='%s' len=%d available=%d", str.c_str(), str.length(), telnetServer.available());
}

ConsoleWrapper Console;

} // namespace enableit

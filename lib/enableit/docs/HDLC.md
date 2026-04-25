# HDLC Library

## Overview
This library implements a simplified **HDLC** protocol for serialized communication between an **embedded device** (such as ESP32) and a host. It provides an interface for transmitting and receiving data encapsulated in a structured format, ensuring communication integrity and reliability.

## Key Features
- **State Management**: Supports **IDLE**, **COMMAND**, and **CONNECTED** modes.
- **HDLC Framing**: Uses synchronization characters and negotiation protocols.
- **Timeout and Retransmission**: Supports timeout detection during read and write operations.
- **Buffered Support**: Allows block transmission and reception of data.
- **Abstract Transport Interface**: Based on an `HDLCTransport` abstraction, making it adaptable to different communication channels (e.g., UART, USB, TCP).

## Operating States
The library operates in three main states:
1. **IDLE**: The device is not active in HDLC communication.
2. **COMMAND**: In this phase, the device negotiates the connection with the host through an exchange of synchronization and acknowledgment packets.
3. **CONNECTED**: Once the connection is established, data can be transmitted in formatted packets.

## Connection Process
The protocol uses a handshake mechanism to establish a secure connection between client and server (**COMMAND** mode):

1. The client sends an initial **SYN** packet.
2. The server responds with an **ACK** sequence.
3. The client repeats the **SYN** transmission to confirm.
4. The server sends a **READY (RDY)** message.
5. The client responds with **GO**, indicating readiness for data communication.
6. Once the sequence is complete, both parties enter the **CONNECTED** state and can transmit data.

If any step fails, the client can retry a limited number of times before considering the connection failed and returning in **IDLE** state.

## Library Interface

### HDLCTransport Class

The `HDLCTransport` class is an abstract class that defines the interface for the transport layer. It must be implemented by any concrete transport class used by the `HDLC` class.

```cpp
class HDLCTransport {
public:
    virtual ~HDLCTransport() = default;
    virtual int write(uint8_t byte) = 0;
    virtual int read() = 0;
    virtual size_t available() const = 0;
    virtual void setTimeout(std::chrono::milliseconds timeout) = 0;
};

### Initialization
```cpp
HDLCTransport transport;
HDLC hdlc(transport, 3); // Maximum of 3 attempts
```

### Starting the Connection
```cpp
hdlc.start(); // Enters COMMAND state and initiates negotiation
```

### Data Transmission
```cpp
std::vector<uint8_t> data = {0x01, 0x02, 0x03};
hdlc.writeBuffer(data);
```

### Data Reception
```cpp
if (hdlc.available()) {
    std::vector<uint8_t> receivedData = hdlc.readBuffer();
}
```

### Custom Timeout
```cpp
hdlc.setTimeout(std::chrono::milliseconds(500)); // Sets a 500ms timeout
```

## Conclusion
This library provides an efficient and reliable implementation of HDLC for serialized communication on embedded devices, with state management, retransmissions, and timeouts. It is easily adaptable to different transport mediums and is ideal for environments where robust communication is a key requirement.


#include <Console.h>
#include <BioTelemetry.h>

BioTelemetry telemetry;

BioTelemetry::BioTelemetry() : 
    _debug(false),
    _streaming_enable(false)
{  
    for (int i= 0; i < TELEMETRY_MAX_CHANNELS; i++) {
        packet[i].setVersion(2);
        producers[i] = nullptr;
        channel_enabled[i] = false;
        // default fill buffer
        channel_fillmode[i] = true;
        channel_pktsize[i] = TELEMETRY_DEFAULT_PAYLOAD_SIZE;
        channel_pktready[i] = 0;
    }
}

bool BioTelemetry::streaming_enable(String remoteip, int port) {

    uint8_t _remoteip[4] = { 0 };
    unsigned int index = 0;

    for (unsigned int i=0; i < remoteip.length(); i++) {
        if (isdigit(remoteip.charAt(i))) {
            _remoteip[index] *= 10;
            _remoteip[index] += remoteip.charAt(i) - '0';
        } else
            index++;
    }

    if ((index != 3) || (!port)) 
        return false;

    remoteIp = _remoteip;
    remotePort = port;

    _streaming_enable = true;

    return true;
}

void BioTelemetry::streaming_disable() {
    _streaming_enable = false;
}

void BioTelemetry::init(bool d, int speed) {
    _debug = d;
 
    //Serial.begin(speed);
 
    _streaming_enable = false;

    udp.begin(32000);
}

void BioTelemetry::console(bool value) {
    _debug = value;
}

void BioTelemetry::debug(String &s) {
    if (_debug) {
        Serial.println(s.c_str());
    }
    send(DEBUG_CHANNEL,(uint8_t *)s.c_str(),s.length());
}

void BioTelemetry::debug(const char *data) {
    if (_debug) {
        Serial.println(data);
    }
    char *end = (char *)data;
    while (*end++);
    send(DEBUG_CHANNEL,(uint8_t *)data,end - data);
}

void BioTelemetry::format(const char *function, const char *format_str, va_list argp) {

    char *bp=format_buffer;
    int bspace = FORMAT_BUFFERSIZE - 1;

    while ((*function) && (bspace)) {
        *bp++ = *function++;
        --bspace;
    }

    *bp++ = ':';
    --bspace;
    *bp++ = ' ';
    --bspace;

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
                default: {
                    // skip format
                }
            }
           
            ++format_str;
        }
    }
    // terminate string
    *bp = 0;
}

void BioTelemetry::vdebug(const char *func, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    format(func,fmt,args);   
    debug(format_buffer);
}

void BioTelemetry::cdebug(const char *func, const char *fmt, ...) {
     if (_debug) {
        va_list args;
        va_start(args, fmt);
        format(func,fmt,args);        
        Serial.println(format_buffer);
    }
}

void BioTelemetry::enable(int channel, int packet_size, bool fill_mode) {
    channel_enabled[channel] = true;
    channel_fillmode[channel] = fill_mode;
    channel_pktsize[channel] = packet_size;
    channel_pktready[channel] = 0;
}

void BioTelemetry::disable(int channel) {
    channel_enabled[channel] = false;
}

void BioTelemetry::attach(int channel, BufferProducer *producer) {
    producers[channel] = producer;
}

BufferProducer *BioTelemetry::detach(int channel) {
    BufferProducer *_p = producers[channel];
    // set default
    producers[channel] = nullptr;
    channel_fillmode[channel] = true;
    channel_pktsize[channel] = TELEMETRY_DEFAULT_PAYLOAD_SIZE;
    channel_pktready[channel] = 0;
    return _p;
}

void BioTelemetry::poll() {
    if (_streaming_enable) {
        for (int channel=0; channel < TELEMETRY_MAX_CHANNELS; channel++) {
            if ((channel_enabled[channel]) && (producers[channel])) {
                DBG("Channel(%d)",channel);
                uint8_t *buffer = telemetry.getBuffer(channel);
                //C_DEBUG("consume");
                int res = producers[channel]->consume(buffer,channel_pktsize[channel]);
                if (res) {
                    DBG("prepared data(%d)",res);
                    channel_pktready[channel] = res;
                    //BioTelemetry.sendBuffer(channel,res);
                }
            }
        }
    }
}

void BioTelemetry::send() {
    if (_streaming_enable) {
        for (int channel=0; channel < TELEMETRY_MAX_CHANNELS; channel++) {
            if (channel_pktready[channel]) {
                sendBuffer(channel, channel_pktready[channel]);
                channel_pktready[channel] = 0;
            }
        }
    }
}

void BioTelemetry::send(unsigned int channel, uint8_t *data, int size) {
    if ((!_streaming_enable) || (!channel_enabled[channel]))
        return;

    packet[channel].init(channel);
    packet[channel].setPayload(data,size);
    packet[channel].setTS(micros() / 15000);
    //int result = udp.sendPacket(packet[channel].getData(),packet[channel].getSize(),remoteIp,remotePort);
    udp.beginPacket(remoteIp,remotePort);
    udp.write(packet[channel].getData(),packet[channel].getSize());
    udp.endPacket();
}

void BioTelemetry::sendBuffer(unsigned int channel, int size) {
    if ((!_streaming_enable) || (!channel_enabled[channel]))
        return;

    packet[channel].init(channel);
    packet[channel].setPayloadSize(size);
    packet[channel].setTS(micros() / 15000);
    //int result = udp.sendPacket(packet[channel].getData(),packet[channel].getSize(),remoteIp,remotePort);
    udp.beginPacket(remoteIp,remotePort);
    udp.write(packet[channel].getData(),packet[channel].getSize());
    udp.endPacket();
}

uint8_t *BioTelemetry::getBuffer(int channel) {
    return packet[channel].getPayload();
}
#include <BioTelemetry.h>
#include <EMGApp.h>
#include <WiFiUdp.h>

uint8_t _buffer[6000];

#define CMD_SETUP "* setup"
#define CMD_TEST "* test <1hz/2hz/dc> <2x gain>, configure test signal"
#define CMD_HOST "* host <ip> <delay>, configure streaming to host"
#define CMD_SOURCE "* source <channel> <electrode/noise/supply/temp/test/rldm/rldp/rldn>, configure channel source"
#define CMD_GAIN "* gain <channel> <1x/2x/3x/4x/6x/8x/12x>, configure channel gain"
#define CMD_ENABLE "* enable <channel>, enable channel, is 'all' is specified enables all channels"
#define CMD_DISABLE "* disable <channel>, enable channel, is 'all' is specified disables all channels"
#define CMD_READDATA "* readdata, execute one data read"
#define CMD_STATUS "* status, check data available in streaming buffer"
#define CMD_BUFFER "* buffer <depth>, set streaming buffer depth"
#define CMD_MODE "* mode <mode>, set emg mode"
#define CMD_ON "* on <signal>, where signals are cs0, cs1, dn, dout, drdy, clk, sclk"
#define CMD_OFF "* off <signal>, where signals are cs0, cs1, dn, dout, drdy, clk, sclk"
#define CMD_READ "* read <register>"
#define CMD_WRITE "* write <register>"
#define CMD_REBOOT "* reboot"
#define CMD_UPDATE "* update"
#define CMD_SEQUENCE "* sequence <mode> <channel>, where mode is burst|temp, channel is optional channel number"
#define CMD_OFFSET "* offset <value>, set dc offset"
#define CMD_HELP    "* help - show this help"

typedef struct {
    const char *name;
    int pin;
} line;

line lines[] = {
    { "cs0", GPIO_NUM_39 },
    { "cs1", GPIO_NUM_3 },
    { "din", GPIO_NUM_18 },
    { "dout", GPIO_NUM_17 },
    { "drdy", GPIO_NUM_7 },
    { "clk", GPIO_NUM_4 },
    { "sclk", GPIO_NUM_16 }
};

WiFiUDP Udp;
IPAddress remoteHost;
int remotePort = 0;
int frame = 0;
int sequence = 0;
int ts = 0;
int produced = 0;

bool runcommand = true;

void EMGApp::enter() {
    DBG("enter EMGApp");
    parser.init(this);
    parser.add("on", CMD_ON, &EMGApp::cmdSignalHigh);
    parser.add("off", CMD_OFF, &EMGApp::cmdSignalLow);
    parser.add("setup", CMD_SETUP, &EMGApp::cmdSetup);
    parser.add("test", CMD_TEST, &EMGApp::cmdTest);
    parser.add("source", CMD_SOURCE, &EMGApp::cmdSource);
    parser.add("gain", CMD_GAIN, &EMGApp::cmdGain);
    parser.add("enable", CMD_ENABLE, &EMGApp::cmdEnable);
    parser.add("disable", CMD_DISABLE, &EMGApp::cmdDisable);
    parser.add("mode", CMD_MODE, &EMGApp::cmdMode);
    parser.add("buffer", CMD_BUFFER, &EMGApp::cmdBuffer);
    parser.add("readdata", CMD_READDATA, &EMGApp::cmdReadData);
    parser.add("status", CMD_STATUS, &EMGApp::cmdStatus);
    parser.add("read", CMD_READ, &EMGApp::cmdRead);
    parser.add("write", CMD_WRITE, &EMGApp::cmdWrite);
    parser.add("reboot", CMD_REBOOT, &EMGApp::cmdReboot);
    parser.add("update", CMD_UPDATE, &EMGApp::cmdUpdate);
    parser.add("sequence", CMD_SEQUENCE, &EMGApp::cmdSequence);
    parser.add("offset", CMD_OFFSET, &EMGApp::cmdOffset);
    parser.add("host", CMD_HOST, &EMGApp::cmdDestHost);
    parser.add("help", CMD_HELP, &EMGApp::cmdHelp);

    telemetry.init();

    offset = 0;
    polling = false;

    // self test
    cmdSetup();

    DBG("Sample commands:");
    DBG("** Reference signal 2Hz test **");
    DBG("test 2hz 2x");
    DBG("source all test");
    DBG("host 192.168.1.12 32000 1200 100");
    
/*    parser.parseLine("test 2hz 2x");
    sleep(1);
    parser.parseLine("source all test");
    sleep(1);
    parser.parseLine("host 192.168.1.12 32000 1200 100");
    sleep(1);
    */
}

void EMGApp::leave() {
    DBG("leave EMGApp");
    emg.fini();
}

void EMGApp::process() {
    if ((Console.available() > 0) && (streaming)) {
        DBG("Disabling streaming");
        polling = false;
        streaming = false;
    }

    if (polling) {
        emg.poll();
    }

    if (streaming) {
        BufferProducer *buffer = emg.getBufferProducer();
        int consumed = 0;
        if (produced) {
            consumed = buffer->consume(_buffer + produced,frame - produced, delay);
        } else {
            consumed = buffer->consume(_buffer,frame, delay);
        }
        
        produced += consumed;
        if (produced >= frame) {
            //int *data = (int *) (_buffer);
            //DBG("Producing sequence[%d], size[%d], frame[%d]", sequence, produced, frame);
            packet.setSN(sequence++);
            packet.setPayload(_buffer,frame);
            packet.setTS(micros() / 15000);
            Udp.beginPacket(remoteHost, remotePort);
            Udp.write(packet.getData(), packet.getSize());
            Udp.endPacket();
            emg.poll();
            produced = 0;
        }
    }
    
    parser.poll();
/*
    if (runcommand) {
        runcommand = false;
        DBG("Sending host command");
        sendHost("192.168.1.12",32000,1200,100);
    }
  */  
    //cmdStatus();
    //delay(100);
    //emg.poll();
    //telemetry.poll();
}

void EMGApp::cmdHelp() {
    OUT("** Help **");
    OUT("Commands:");
    for (int i = 0; i < MAX_COMMANDS; i++) {
        const char *h = parser.getHelp(i);
        if (h) OUT(h);
    }
}

void EMGApp::cmdDestHost() {
    String _host = parser.getString(1);
    remotePort = parser.getInt(2);
    frame = parser.getInt(3);
    delay = parser.getInt(4);
    sendHost(_host,remotePort,frame,delay);
}

void EMGApp::sendHost(String _host,int remotePort, int frame, int delay) {
    DBG("Configuring streaming to host, remote host[%s:%d], frame size[%d], delay %d ms", _host.c_str(), remotePort, frame, delay);
    
    remoteHost.fromString(_host);
    streaming = true;
    Udp.begin(32000);

    packet.setM(false);
    packet.setSSRC(0);
    packet.setCC(0);
    packet.setVersion(2);
    packet.setPT(96);

    DBG("Setting buffer size[2000]");
    emg.setBuffer(2000);

    DBG("Starting streaming");
    emg.streaming(true);

}


void EMGApp::cmdSignalHigh() {
    String _signal = parser.getString(1);

    for (int i = 0; i < sizeof(lines) / sizeof(line); i++) {
        if (!strcasecmp(_signal.c_str(),lines[i].name)) {
            DBG("Setting Signal[%s] High", lines[i].name);
            pinMode(lines[i].pin, OUTPUT);
            digitalWrite(lines[i].pin, HIGH);
        }
    }
}

void EMGApp::cmdSignalLow() {
    String _signal = parser.getString(1);

    for (int i = 0; i < sizeof(lines) / sizeof(line); i++) {
        if (!strcasecmp(_signal.c_str(),lines[i].name)) {
            DBG("Setting Signal[%s] Low", lines[i].name);
            pinMode(lines[i].pin, OUTPUT);
            digitalWrite(lines[i].pin, LOW);
        }
    }    
}

void EMGApp::cmdReboot() {
   OUT("** Reboot **");
   changeApp(STATE_REBOOT);
}

void EMGApp::cmdUpdate() {
   OUT("** Start OTA Web Update **");
   changeApp(APP_OTAWEBUPDATE);
}

void EMGApp::cmdSetup() {
    DBG("EMG Setup");
    emg.init();
}

void EMGApp::cmdReadData() {
    DBG("EMG Read data");

    if (emg.readData(buffer)) {
        DBG("Data found");
        for (int i = 0; i < 9; i++) {
            double uV = buffer[i] / pow(2,23) * 2.4 * 1000000 - offset;
            DBG("Channel %d, Buffer[%s], Value[%f]", i, String(buffer[i], HEX).c_str(), uV);
        }
    } else    
        DBG("no data available");
}

void EMGApp::cmdWrite() {
    String r = parser.getString(1);
    String value = parser.getString(2);

    DBG("EMG Write[%d], value[%d]", r.toInt(), value.toInt());
    emg.write(r.toInt(), value.toInt());
}

void EMGApp::cmdRead() {
    String r = parser.getString(1);
    int value = emg.read(r.toInt());
    DBG("EMG Read[%d], value[%d]", r.toInt(), value);
}

void EMGApp::cmdTest() {
    String mode = parser.getString(1);
    String amply = parser.getString(2);

    bool amply_2x = amply.equalsIgnoreCase("2x");

    if (mode.equalsIgnoreCase("2hz")) {
        emg.setTestMode(amply_2x, SerialEmg::SIG_2HZ);
    } else if (mode.equalsIgnoreCase("1hz")) {
        emg.setTestMode(amply_2x, SerialEmg::SIG_1HZ);
    } else if (mode.equalsIgnoreCase("dc")) {
        emg.setTestMode(amply_2x, SerialEmg::SIG_DC);
    }
}

void EMGApp::cmdGain() {
    String channel = parser.getString(1);
    String gain = parser.getString(2);

    SerialEmg::CHANNEL_GAIN g;

    if (gain.equalsIgnoreCase("1x")) {
        g = SerialEmg::GAIN_1X;
    } else if (gain.equalsIgnoreCase("2x")) {
        g = SerialEmg::GAIN_2X;
    } else if (gain.equalsIgnoreCase("3x")) {
        g = SerialEmg::GAIN_3X;
    } else if (gain.equalsIgnoreCase("4x")) {
        g = SerialEmg::GAIN_4X;
    } else if (gain.equalsIgnoreCase("6x")) {
        g = SerialEmg::GAIN_6X;
    } else if (gain.equalsIgnoreCase("8x")) {
        g = SerialEmg::GAIN_8X;
    } else if (gain.equalsIgnoreCase("12x")) {
        g = SerialEmg::GAIN_12X;
    } else {
        DBG("unexpected gain[%s]", gain.c_str());
        return;  
    }

    if (channel.equalsIgnoreCase("all")) {
        for (int i = 0; i < emg.getMaxChannels(); i++)
            emg.setGain(i, g);
    } else {
        emg.setGain(channel.toInt(), g);
    }
    
}

void EMGApp::cmdSource() {
    String channel = parser.getString(1);
    String source = parser.getString(2);

    SerialEmg::CHANNEL_SRC src;
    
    if (source.equalsIgnoreCase("electrode")) {
        src = SerialEmg::SRC_ELECTRODE;
    } else if (source.equalsIgnoreCase("noise")) {
        src = SerialEmg::SRC_NOISE;
    } else if (source.equalsIgnoreCase("supply")) {
        src = SerialEmg::SRC_SUPPLY;
    } else if (source.equalsIgnoreCase("temp")) {
        src = SerialEmg::SRC_TEMPERATURE;
    } else if (source.equalsIgnoreCase("test")) {
        src = SerialEmg::SRC_TEST;
    } else if (source.equalsIgnoreCase("rldm")) {
        src = SerialEmg::SRC_RLD_MEAS;
    } else if (source.equalsIgnoreCase("rldp")) {
        src = SerialEmg::SRC_RLD_POS;
    } else if (source.equalsIgnoreCase("rldn")) {
        src = SerialEmg::SRC_RLD_NEG;
    } else {
        DBG("unexpected source[%s]", source.c_str());
        return;
    }

    DBG("Setting src");

    if (channel.equalsIgnoreCase("all")) {
        for (int i = 0; i < emg.getMaxChannels(); i++)
            emg.setSrc(i, src);
    } else {
        emg.setSrc(channel.toInt(), src);
    }

}

void EMGApp::cmdEnable() {
    String channel = parser.getString(1);

    if (channel.equalsIgnoreCase("all")) {
        DBG("Enabling channel[all]");
        for (int i = 0; i < emg.getMaxChannels(); i++)
            emg.enable(i);
    } else {
        DBG("Enabling channel[%d]", channel.toInt());
        emg.enable(channel.toInt());
    }
}

void EMGApp::cmdDisable() {
    String channel = parser.getString(1);

    if (channel.equalsIgnoreCase("all")) {
        DBG("Disabling channel[all]");
        for (int i = 0; i < emg.getMaxChannels(); i++)
            emg.disable(i);
    } else {
        DBG("Disabling channel[%d]", channel.toInt());
        emg.disable(channel.toInt());
    } 
}

void EMGApp::cmdMode() {
    String mode = parser.getString(1);

    if (mode.equalsIgnoreCase("streaming")) {
        bool on = parser.getBool(2);
        emg.streaming(on);
        polling = on;
    } else if (mode.equalsIgnoreCase("host")) {
        bool on = parser.getBool(2);
        if (on) {
            String ip = parser.getString(3);
            int port = parser.getInt(4);
            DBG("Starting data streaming[%s:%d]", ip.c_str(), port);
            telemetry.streaming_enable(ip, port);
            BufferProducer *buffer = emg.getBufferProducer();
            DBG("Attaching buffer[0x%x]",buffer);
            telemetry.attach(SEMG_CHANNEL0,buffer);
            DBG("Enabling channel SEMG_CHANNEL0(1)");
            telemetry.enable(SEMG_CHANNEL0);
        } else {
            DBG("Stopping data streaming");
            telemetry.disable(SEMG_CHANNEL0);
        }
    } else if (mode.equalsIgnoreCase("go")) {
        bool on = parser.getBool(2);
        if (on) {
            DBG("Starting acquisition");
            emg.init();
            emg.setBuffer(1000);
            emg.streaming(true);
        } else {
            DBG("Stopping acquisition");
            emg.streaming(false);
        }
    }
}

void EMGApp::cmdStatus() {
    long status = emg.getStatus();
    byte state = emg.getChannelState();
    int avail = emg.avail();
    emg.poll();
    BufferProducer *buffer = emg.getBufferProducer();
    int consumed = buffer->consume(_buffer,6000);
    DBG("Status[%x], channel state[%x], available data[%d], consumed[%d], ticks[%d]", status, state, avail, consumed, millis());
}

void EMGApp::cmdBuffer() {
    String size = parser.getString(1);
    int value = size.toInt();

    if (value >= 0 ) {
        DBG("Setting buffer depth[%d]", value);
        emg.setBuffer(value);
    }
}

void EMGApp::cmdOffset() {
    DBG("Offset");
    String number = parser.getString(1);

    offset = number.toDouble();
    DBG("DC Offset[%f]", offset);
}

void EMGApp::cmdSequence() {
    DBG("Sequence");
    String command = parser.getString(1);
    String channel = parser.getString(2);

    int _channel = 1;
    if (!channel.equalsIgnoreCase("")) {
        _channel = channel.toInt();

        if ((_channel <= 0) || (_channel > emg.getMaxChannels())) {
            ERR("Wrong channel number[%d]", _channel);
            return;
        }
    }

    if (command.equalsIgnoreCase("burst")) {
        DBG("Burst reading from channel[%d]", _channel);
        double avg = 0;
        for (int i = 0; i < 250; i++) {
            if (emg.readData(buffer)) {
                double uV = buffer[_channel] / pow(2,23) * 2.4 * 1000000 - offset;
                if (i > 0)
                    avg = (uV + i * avg) / (i+1);
                else
                    avg = uV;
                DBG("%d, %d, %d, %f, %f", i, _channel, buffer[_channel], uV, avg);
            }
        }
    } else if (command.equalsIgnoreCase("temp")) {
        if (emg.readData(buffer)) {
            double uV = buffer[1] / pow(2,23) * 2.4 * 1000000;
            double temp = ((uV - 145300)/490) + 25;
            DBG("uV[%f], temp[%f]", uV, temp);
        }
    }
}
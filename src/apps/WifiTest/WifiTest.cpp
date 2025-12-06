/*
 *  This sketch shows the WiFi event usage
 *
*/

/*
* WiFi Events

0  ARDUINO_EVENT_WIFI_READY               < ESP32 WiFi ready
1  ARDUINO_EVENT_WIFI_SCAN_DONE                < ESP32 finish scanning AP
2  ARDUINO_EVENT_WIFI_STA_START                < ESP32 station start
3  ARDUINO_EVENT_WIFI_STA_STOP                 < ESP32 station stop
4  ARDUINO_EVENT_WIFI_STA_CONNECTED            < ESP32 station connected to AP
5  ARDUINO_EVENT_WIFI_STA_DISCONNECTED         < ESP32 station disconnected from AP
6  ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE      < the auth mode of AP connected by ESP32 station changed
7  ARDUINO_EVENT_WIFI_STA_GOT_IP               < ESP32 station got IP from connected AP
8  ARDUINO_EVENT_WIFI_STA_LOST_IP              < ESP32 station lost IP and the IP is reset to 0
9  ARDUINO_EVENT_WPS_ER_SUCCESS       < ESP32 station wps succeeds in enrollee mode
10 ARDUINO_EVENT_WPS_ER_FAILED        < ESP32 station wps fails in enrollee mode
11 ARDUINO_EVENT_WPS_ER_TIMEOUT       < ESP32 station wps timeout in enrollee mode
12 ARDUINO_EVENT_WPS_ER_PIN           < ESP32 station wps pin code in enrollee mode
13 ARDUINO_EVENT_WIFI_AP_START                 < ESP32 soft-AP start
14 ARDUINO_EVENT_WIFI_AP_STOP                  < ESP32 soft-AP stop
15 ARDUINO_EVENT_WIFI_AP_STACONNECTED          < a station connected to ESP32 soft-AP
16 ARDUINO_EVENT_WIFI_AP_STADISCONNECTED       < a station disconnected from ESP32 soft-AP
17 ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED         < ESP32 soft-AP assign an IP to a connected station
18 ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED        < Receive probe request packet in soft-AP interface
19 ARDUINO_EVENT_WIFI_AP_GOT_IP6               < ESP32 ap interface v6IP addr is preferred
19 ARDUINO_EVENT_WIFI_STA_GOT_IP6              < ESP32 station interface v6IP addr is preferred
20 ARDUINO_EVENT_ETH_START                < ESP32 ethernet start
21 ARDUINO_EVENT_ETH_STOP                 < ESP32 ethernet stop
22 ARDUINO_EVENT_ETH_CONNECTED            < ESP32 ethernet phy link up
23 ARDUINO_EVENT_ETH_DISCONNECTED         < ESP32 ethernet phy link down
24 ARDUINO_EVENT_ETH_GOT_IP               < ESP32 ethernet got IP from connected AP
19 ARDUINO_EVENT_ETH_GOT_IP6              < ESP32 ethernet interface v6IP addr is preferred
25 ARDUINO_EVENT_MAX
*/

#include <WiFi.h>
#include <BioTelemetry.h>
#include "WifiTest.h"

const char *ssid = "enable-test";
const char *password = "enableit";

// WARNING: This function is called from a separate FreeRTOS task (thread)!
void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);

  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:               DBG("WiFi interface ready"); break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:           DBG("Completed scan for access points"); break;
    case ARDUINO_EVENT_WIFI_STA_START:           DBG("WiFi client started"); break;
    case ARDUINO_EVENT_WIFI_STA_STOP:            DBG("WiFi clients stopped"); break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:       DBG("Connected to access point"); break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:    DBG("Disconnected from WiFi access point"); break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE: DBG("Authentication mode of access point has changed"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      DBG("Obtained IP address: %s", WiFi.localIP().toString().c_str());
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:        DBG("Lost IP address and IP address is reset to 0"); break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:          DBG("WiFi Protected Setup (WPS): succeeded in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_FAILED:           DBG("WiFi Protected Setup (WPS): failed in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:          DBG("WiFi Protected Setup (WPS): timeout in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_PIN:              DBG("WiFi Protected Setup (WPS): pin code in enrollee mode"); break;
    case ARDUINO_EVENT_WIFI_AP_START:           DBG("WiFi access point started"); break;
    case ARDUINO_EVENT_WIFI_AP_STOP:            DBG("WiFi access point  stopped"); break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:    DBG("Client connected"); break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: DBG("Client disconnected"); break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:   DBG("Assigned IP address to client"); break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:  DBG("Received probe request"); break;
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:         DBG("AP IPv6 is preferred"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:        DBG("STA IPv6 is preferred"); break;
    case ARDUINO_EVENT_ETH_GOT_IP6:             DBG("Ethernet IPv6 is preferred"); break;
    case ARDUINO_EVENT_ETH_START:               DBG("Ethernet started"); break;
    case ARDUINO_EVENT_ETH_STOP:                DBG("Ethernet stopped"); break;
    case ARDUINO_EVENT_ETH_CONNECTED:           DBG("Ethernet connected"); break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:        DBG("Ethernet disconnected"); break;
    case ARDUINO_EVENT_ETH_GOT_IP:              DBG("Obtained IP address"); break;
    default:                                    break;
  }
}

// WARNING: This function is called from a separate FreeRTOS task (thread)!
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  DBG("WiFi connected");
  DBG("IP address: %s", IPAddress(info.got_ip.ip_info.ip.addr).toString().c_str());
}

void WifiApp::enter() {
  Serial.begin(115200);

  // delete old config
  WiFi.disconnect(true);

  delay(1000);

  // Examples of different ways to register wifi events;
  // these handlers will be called from another thread.
  WiFi.onEvent(WiFiEvent);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFiEventId_t eventID = WiFi.onEvent(
    [](WiFiEvent_t event, WiFiEventInfo_t info) {
      DBG("WiFi lost connection. Reason: %d",info.wifi_sta_disconnected.reason);
    },
    WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED
  );

  // Remove WiFi event
  DBG("WiFi Event ID: %d",eventID);
  // WiFi.removeEvent(eventID);

  DBG("Scan start");

  WiFi.setTxPower(WIFI_POWER_5dBm);

  // WiFi.scanNetworks will return the number of networks found.
  int n = WiFi.scanNetworks();
  DBG("Scan done");
  if (n == 0) {
    DBG("no networks found");
  } else {
    Serial.print(n);
    DBG(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID for each network found
      DBG("%s\n", WiFi.SSID(i).c_str());
      delay(10);
    }
  }
  DBG("");

  // Delete the scan result to free memory for code below.
  WiFi.scanDelete();

  WiFi.begin(ssid, password);

  DBG("Wait for WiFi... ");
}

void WifiApp::process() {
  delay(1000);
}

const char *WifiApp::name() {
    return APP_WIFITEST;
}

void WifiApp::leave() {

}
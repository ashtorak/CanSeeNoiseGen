#include "wifihandler.h"

static CS_CONFIG_t *wifi_config;
static boolean wiFiIsActive = false;
WiFiServer server(35000);
WiFiClient serverClients[MAX_SRV_CLIENTS];

void wifi_init () {
  wifi_config = getConfig ();
  if (!wifi_config->mode_wifi) return;
  if (wifi_config->mode_bluetooth) {
    wifi_config->mode_wifi = 0;
    if (wifi_config->mode_debug) Serial.println ("Wifi can not coexist with Bluetooth and is disabled.");
  }

  if (wifi_config->mode_wifi == WIFI_SOFTAP) {
    if (wifi_config->mode_debug) Serial.println("Wifi AP '" + String (wifi_config->ssid_ap) + "' started.");
    WiFi.softAP (wifi_config->ssid_ap, wifi_config->password_ap);           // init WiFi access point
    wiFiIsActive = true;                                                    // no need to check for active network
    if (wifi_config->mode_debug) {
      IPAddress IP = WiFi.softAPIP ();
      Serial.print ("AP IP address: ");
      Serial.println (IP);
    }
    server.begin ();                                                        // start the server
  } else if (wifi_config->mode_wifi == WIFI_STATION) {
    if (wifi_config->mode_debug) Serial.println("Wifi connecting to " + String (wifi_config->ssid_station) + " starting ...");
    WiFi.begin(wifi_config->ssid_station, wifi_config->password_station);   // init WiFi station. Cheking is done in main loop
    WiFi.mode(WIFI_STA);

    server.begin();                                                         // start the server
  }
}

void writeOutgoingWiFi (String o) {
  if (!wifi_config->mode_wifi) return;
  if (!wiFiIsActive) return;
  char buf[1024];
  unsigned int len;
  // o.replace ("\n", "\n\r");
  if ((len = o.length()) > 1024) len = 1024;
  o.toCharArray(buf, len);
  for (int i = 0; i < MAX_SRV_CLIENTS; i++) {
    if (serverClients[i] && serverClients[i].connected()) {
      //serverClients[i].write(o);
      serverClients[i].write(buf, len);
      //delay(1);
    }
  }
}

void readIncomingWiFi (String &readBuffer) {
  if (!wifi_config->mode_wifi) return;

  // if in station mode, check if connected and set wiFiActive
  // if in softap modem wifi is always active
  if (wifi_config->mode_wifi == WIFI_STATION) {
    if (WiFi.status() == WL_CONNECTED) {            // check if connected
      if (!wiFiIsActive) {
        if (wifi_config->mode_debug) {
          IPAddress IP = WiFi.localIP ();
          Serial.print ("IP ST address: ");
          Serial.println (IP);
        }
        wiFiIsActive = true;
      }
    } else {
      if (wiFiIsActive) {
        wiFiIsActive = false;
      }
    }
  } else if (wifi_config->mode_wifi == WIFI_SOFTAP) {
    wiFiIsActive = true;
  }
  if (!wiFiIsActive) return;

  // at this point in time we can be sure there is operational WiFi
  uint8_t i;
  if (server.hasClient()) {                        // check if there are any *NEW* clients
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {        //if so, find free or disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()) {
        if (serverClients[i]) {                    // if not free (so disconnected)
          serverClients[i].stop();                 // stop the client
          if (wifi_config->mode_debug) {
            Serial.print("Disconnected: ");
            Serial.println(i);
          }
        }
        serverClients[i] = server.available();     // fetch the client
        if (serverClients[i]) {                    // it should be here
          if (wifi_config->mode_debug) {
            Serial.print("New client: ");
            Serial.print(i); Serial.print(' ');
            Serial.println(serverClients[i].remoteIP());
          }
        } else {                                   // if gone, oh well
          if (wifi_config->mode_debug) Serial.println("available broken");
        }
        break;
      }
    }
    if (i >= MAX_SRV_CLIENTS) {                    //no free/disconnected spot so reject
      server.available().stop();
      if (wifi_config->mode_debug) Serial.println("Refused");
    }
  }

  // at this point actual connections (new and lost ones) are handled
  for (i = 0; i < MAX_SRV_CLIENTS; i++) {          // check clients for data
    if (serverClients[i] && serverClients[i].connected()) {
      while (serverClients[i].available()) {       // if there is data
        char ch = serverClients[i].read();         // get it
        if (ch == '\n' || ch == '\r') {            // buffer / process it
          if (readBuffer != "") {
            if (wifi_config->command_handler) wifi_config->command_handler ();
            readBuffer = "";
          }
        } else {
          readBuffer += ch;
        }
      }
    } else {                                       // no client, or unconnected
      if (serverClients[i]) {                      // if there is a client (so unconnected)
        serverClients[i].stop();                   // stop the client
        if (wifi_config->mode_debug) {
          Serial.print("Disconnected: ");
          Serial.println(i);
        }
      }
    }
  }
}

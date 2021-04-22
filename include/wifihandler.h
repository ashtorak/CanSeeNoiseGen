#ifndef _WIFIHANDLER_H_
#define _WIFIHANDLER_H_

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"

#define MAX_SRV_CLIENTS 1

void wifi_init ();
void writeOutgoingWiFi (String o);
void readIncomingWiFi (String &readBuffer);

#endif

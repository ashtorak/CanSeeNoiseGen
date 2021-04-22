#ifndef _BLUETOOTHHANDLER_H_
#define _BLUETOOTHHANDLER_H_

#include <Arduino.h>
#include <BluetoothSerial.h>

#include "config.h"

// Repository included libraries includes, see ./lib/ ************************

void bluetooth_init ();
void writeOutgoingBluetooth (String o);
void readIncomingBluetooth (String &readBuffer);
void setActiveBluetooth (bool active);

#endif

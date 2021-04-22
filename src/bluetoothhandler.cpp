#include "bluetoothhandler.h"

static CS_CONFIG_t *bluetooth_config;
static BluetoothSerial SerialBT;
static bool bluetooth_active = false;

void bluetooth_init () {
  bluetooth_config = getConfig ();
  bluetooth_active = bluetooth_config->mode_bluetooth;
  if (bluetooth_active) {
    if (bluetooth_config->mode_debug) Serial.println("Bluetooth '" + String (bluetooth_config->name_bluetooth) + "' started.");
    SerialBT.begin(bluetooth_config->name_bluetooth); // init Bluetooth serial, no password in current framework
  }
}

void setActiveBluetooth (bool active) {
  // If active is false in and output is simply ignored. At the moment This
  // is a safer solution than switching off Bluetooth or BluetoothSerial as
  // there seems to be a nasty memory leak in that part of the core, see
  // https://github.com/espressif/arduino-esp32/issues/1798
  if (!bluetooth_config->mode_bluetooth) return;
  bluetooth_active = active;
}

void writeOutgoingBluetooth (String o) {
  if (bluetooth_active && SerialBT.hasClient()) SerialBT.print (o);
}

void readIncomingBluetooth (String &readBuffer) {
  if (!bluetooth_config->mode_bluetooth) return;
  if (!SerialBT.available()) return;
  char ch = SerialBT.read();
  if (ch == '\n' || ch == '\r') {
    if (readBuffer != "") {
      if (bluetooth_active && bluetooth_config->command_handler) bluetooth_config->command_handler ();
      readBuffer = "";
    }
  } else {
    readBuffer += ch;
  }
}

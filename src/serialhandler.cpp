#include "serialhandler.h"

static CS_CONFIG_t *serial_config;

void serial_init () {                              // Serial is already running
  serial_config = getConfig ();
  if (!serial_config->mode_serial && !serial_config->mode_debug) Serial.end();
}

void writeOutgoingSerial (String o) {
  if (serial_config->mode_serial || serial_config->mode_debug) Serial.print (o);
}

void readIncomingSerial (String &readBuffer) {
  if (!serial_config->mode_serial && !serial_config->mode_debug) return;
  if (!Serial.available()) return;
  char ch = Serial.read();
  if (ch == '\n' || ch == '\r') {
    if (readBuffer != "") {
      if (serial_config->command_handler) serial_config->command_handler ();
      readBuffer = "";
    }
  } else {
    readBuffer += ch;
  }
}

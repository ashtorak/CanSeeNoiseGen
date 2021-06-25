#include "config.h"
#include "CAN_config.h"

static CS_CONFIG_t cs_config;
static bool fetched = false;

void setConfigDefault_3 () {
  cs_config.version                   = 3;         // change if length of config changes
  cs_config.can0_rx                   = GPIO_NUM_16;
  cs_config.can0_tx                   = GPIO_NUM_17;
  cs_config.can0_speed                = (uint16_t)CAN_SPEED_1000KBPS;
  /* On ESP32 Rev. 3 can bus speed needs to be set to twice the old value. Before it was 500KBPS.
  See https://gitlab.com/jeroenmeijer/cansee/-/issues/17 or 
  https://www.esp32.com/viewtopic.php?t=15581
  */
  cs_config.can1_rx                   = GPIO_NUM_36;
  cs_config.can1_tx                   = GPIO_NUM_39;
  cs_config.can1_speed                = (uint16_t)CAN_SPEED_250KBPS;
}

void setConfigDefault_4 (bool reset_boot_count) {
  cs_config.version                   = 4;         // change if length of config changes
  if (reset_boot_count)
    cs_config.boot_count              = 0;
}

void setConfigRam () {
  cs_config.bus = 0;
  cs_config.command_handler = NULL;
  cs_config.output_handler = NULL;
  fetched = true;
}

void setConfigDefault () {
  cs_config.magicnumber = 0x0caacee0;              // does that read CanSee?
  cs_config.version                   = 1;         // change if length of config changes
  cs_config.mode_serial               = 1;
  cs_config.mode_bluetooth            = 1;
  cs_config.mode_wifi                 = 0;         // WIFI_SOFTAP or WIFI_STATION;
  cs_config.mode_debug                = 0xf6;
  strcpy (cs_config.name_bluetooth,   "CANSee");
  strcpy (cs_config.pin_bluetooth,    "1234");     // not implemented in framework yet
  strcpy (cs_config.ssid_ap,          "CANSee");
  strcpy (cs_config.password_ap,      "CANSeeMe");
  strcpy (cs_config.ssid_station,     "");
  strcpy (cs_config.password_station, "");
  setConfigDefault_3 ();
  setConfigDefault_4 (false);
  setConfigRam ();
}

CS_CONFIG_t *getConfig () {
  if (fetched) return &cs_config;
  if (!EEPROM.begin (sizeof (CS_CONFIG_t))) {
    Serial.println ("failed to initialise EEPROM for reading");
    setConfigDefault ();
    setConfigRam ();
    return &cs_config;
  }
  if (EEPROM.readBytes (0, &cs_config, sizeof (CS_CONFIG_t)) != sizeof (CS_CONFIG_t) || cs_config.magicnumber != 0x0caacee0) {
    Serial.println ("Not a valid EEPROM record");
    setConfigToEeprom (true);
  }
  // so here we have a valid cs_config
  if (cs_config.version < 3) {
    Serial.println ("EEPROM structure changed");
    EEPROM.end ();
    setConfigDefault_3 ();
    setConfigToEeprom (false);
  } else if (cs_config.version < 4) {
      Serial.println ("EEPROM structure changed");
      EEPROM.end ();
      setConfigDefault_4 (true);
      setConfigToEeprom (false);
  } else {
    EEPROM.end ();
  }
  // Clear ram components
  setConfigRam ();
  return &cs_config;
}

void setConfigToEeprom (bool reset) {
  if (!EEPROM.begin (sizeof (CS_CONFIG_t)))
  {
    Serial.println ("failed to initialise EEPROM for writing");
    return;
  }
  if (reset) setConfigDefault ();
  EEPROM.writeBytes (0, &cs_config, sizeof (CS_CONFIG_t));
  EEPROM.commit ();
  EEPROM.end ();
  return;
}

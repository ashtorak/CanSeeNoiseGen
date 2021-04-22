#include "canhandler.h"

static CS_CONFIG_t *can_config;
static uint8_t used_bus;
CAN_device_t CAN_cfg;

void can_bus_set () {
  if (used_bus == 0) {                                            // init the CAN bus (pins and baudrate)
    CAN_cfg.speed     = (CAN_speed_t)can_config->can0_speed;
    CAN_cfg.tx_pin_id = (gpio_num_t)can_config->can0_tx;
    CAN_cfg.rx_pin_id = (gpio_num_t)can_config->can0_rx;
  } else {
    CAN_cfg.speed     = (CAN_speed_t)can_config->can1_speed;
    CAN_cfg.tx_pin_id = (gpio_num_t)can_config->can1_tx;
    CAN_cfg.rx_pin_id = (gpio_num_t)can_config->can1_rx;
  }
}

void can_init () {
  can_config = getConfig ();
  used_bus = 0;
  can_bus_set ();
  // create a generic RTOS queue for CAN receiving, with 10 positions
  CAN_cfg.rx_queue = xQueueCreate(10, sizeof(CAN_frame_t));
  if (CAN_cfg.rx_queue == 0) {
    if (can_config->mode_debug) Serial.println("Can't create CANbus buffer. Stopping");
    while (1);
  }
  if (can_config->mode_debug) Serial.println("CANbus started");
  ESP32Can.CANInit();                              // initialize CAN Module
}

void can_send (CAN_frame_t *frame, uint8_t bus) {
  if (bus != used_bus) {
    used_bus = bus;
    ESP32Can.CANStop();
    can_bus_set ();
    ESP32Can.CANInit();                            // initialize CAN Module
  }
  ESP32Can.CANWriteFrame (frame);
}

boolean can_receive (CAN_frame_t *rx_frame) {
  return xQueueReceive (CAN_cfg.rx_queue, rx_frame, (TickType_t)0) == pdTRUE ? true : false;
}

// convert a CAN_frame to readable hex output format
String canFrameToString(CAN_frame_t &frame) {
  String dataString = String(frame.MsgID, HEX) + ",";
  for (int i = 0; i < frame.FIR.B.DLC; i++) {
    dataString += getHex(frame.data.u8[i]);
  }
  dataString += "\n";
  return dataString;
}

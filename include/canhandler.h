#ifndef _CANHANDLER_H_
#define _CANHANDLER_H_

#include <Arduino.h>

// Repository included libraries includes, see ./lib/ ************************
#include "ESP32CAN.h"
#include "CAN_config.h"

#include "config.h"
#include "utils.h"

void can_init ();
void can_send (CAN_frame_t *frame, uint8_t bus);
boolean can_receive (CAN_frame_t *rx_frame);
String canFrameToString(CAN_frame_t &frame);

#endif

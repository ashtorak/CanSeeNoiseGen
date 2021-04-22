#ifndef _FREEFRAMEHANDLER_H_
#define _FREEFRAMEHANDLER_H_

#include <Arduino.h>

#include "config.h"
#include "canhandler.h"
#include "utils.h"


// Free frames storage *******************************************************
// all free frames are stored in an array for all 700 free frames. This speeds
// up request/response time significantly.
#define FREEFRAMEARRAYSIZE 0x700
typedef struct {
  uint8_t data[8];
  uint8_t length;
  uint8_t age;
} FREEFRAME_t;

void freeframe_init ();
void storeFreeframe (CAN_frame_t &frame, uint8_t bus);
void ageFreeFrame ();
FREEFRAME_t *getFreeframe (uint32_t id, uint8_t bus);
void requestFreeframe  (uint32_t id, uint8_t bus);
String bufferedFrameToString (uint32_t id, uint8_t bus);

#endif

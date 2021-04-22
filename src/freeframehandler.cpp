#include "freeframehandler.h"

static CS_CONFIG_t *freeframe_config;
static FREEFRAME_t freeframes[FREEFRAMEARRAYSIZE];

void freeframe_init () {
  freeframe_config = getConfig ();
  for (int id = 0; id < FREEFRAMEARRAYSIZE; id++) {// clear the free frame buffer. Data zeroed, length zeroed, not timed out
    for (int i = 0; i < 8; i++) freeframes[id].data[i] = 0;
    freeframes[id].length = 0;
    freeframes[id].age = 6;                        // allow 25-30 seconds after boot to display empty frames
  }
}

void storeFreeframe (CAN_frame_t &frame, uint8_t bus) {
  if (!(frame.MsgID < FREEFRAMEARRAYSIZE)) return;
  if (freeframe_config->mode_debug & DEBUG_BUS_RECEIVE_FF) {
    Serial.print ("< can:FF:");
    Serial.print(canFrameToString(frame));
  }
  for (int i = 0; i < 8; i++) {                    // store a copy
    freeframes[frame.MsgID].data[i] = frame.data.u8[i];
  }
  freeframes[frame.MsgID].length = frame.FIR.B.DLC;// and the length
  freeframes[frame.MsgID].age = 2;                 // age in to 5-10 seconds
}

void ageFreeFrame () {
  for (int id = 0; id < FREEFRAMEARRAYSIZE; id++) {     // clear the free frame buffer. Data zeroed, length zeroed, not timed out
    if (freeframes[id].age > 0) freeframes[id].age--;
  }
}


FREEFRAME_t *getFreeframe (uint32_t id, uint8_t bus) {
  if (id >= FREEFRAMEARRAYSIZE) return &freeframes [0];
  return &freeframes [id];
}

void requestFreeframe  (uint32_t id, uint8_t bus) {
  if (freeframe_config->output_handler) {
    freeframe_config->output_handler (bufferedFrameToString (id, bus));
  }
}

// convert a buffered frame to readable hex output format
String bufferedFrameToString (uint32_t id, uint8_t bus) {
  String dataString = String (id, HEX) + ",";
  if (freeframes[id].age) {                        // do not output stale data
    for (int i = 0; i < freeframes[id].length; i++) {
      dataString += getHex(freeframes[id].data[i]);
    }
  }
  dataString += "\n";
  return dataString;
}

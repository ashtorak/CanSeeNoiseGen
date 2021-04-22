#include "isotphandler.h"

static CS_CONFIG_t *isotp_config;
static ISO_MESSAGE_t isoMessageIncoming; // declare an ISO-TP message
static ISO_MESSAGE_t isoMessageOutgoing; // declare an ISO-TP message
static unsigned long lastMicros;

void isotp_init()
{
  isotp_config = getConfig();
  isoMessageIncoming.id = isoMessageOutgoing.id = 0xffff;
  isoMessageIncoming.length = isoMessageIncoming.index = isoMessageOutgoing.length = isoMessageOutgoing.index = 0;
}

void isotp_ticker()
{
  CAN_frame_t frame; // build the CAN frame
  if (isoMessageOutgoing.flow_active == 0)
    return; //
  if ((micros() - lastMicros) < isoMessageOutgoing.flow_delay)
    return;

  // Prepare the next frame
  frame.FIR.B.FF = isoMessageOutgoing.id < 0x800 ? CAN_frame_std : CAN_frame_ext;
  frame.FIR.B.RTR = CAN_no_RTR;        // no RTR
  frame.MsgID = isoMessageOutgoing.id; // set the ID
  frame.FIR.B.DLC = 8;                 //command.requestLength + 1;// set the length. Note some ECU's like DLC 8

  frame.data.u8[0] = 0x20 | (isoMessageOutgoing.next++ & 0x0f);
  int i;
  for (i = 0; i < 7 && isoMessageOutgoing.index < isoMessageOutgoing.length; i++)
  {
    frame.data.u8[i + 1] = isoMessageOutgoing.data[isoMessageOutgoing.index++];
  }
  for (; i < 7; i++)
  {
    frame.data.u8[i + 1] = 0;
  }

  // send the frame
  if (isotp_config->mode_debug & DEBUG_COMMAND_ISO)
  {
    Serial.print("> com:Sending ISOTP NEXT:");
    Serial.print(canFrameToString(frame));
  }

  if (isoMessageOutgoing.length == isoMessageOutgoing.index)
  {
    // Done sending the outgoing message, so reset it and cancel further
    // handling by this ticker
    isoMessageOutgoing.length = isoMessageOutgoing.index = 0;
    isoMessageOutgoing.flow_active = 0;

    // At this moment, further sending by the ticker will stop. A few clow control
    // should not come in, but we now expect the answer, so do not invalidate the
    // incoming id
    // isoMessageIncoming.id = 0xffff;
    // the incoming message is full initiaized (id, index)
  }

  if (isoMessageOutgoing.flow_counter != 0)
  {
    if (--isoMessageOutgoing.flow_counter == 0)
    {
      isoMessageOutgoing.flow_active = 0;
    }
  }

  can_send(&frame, 0); // bus ogic needs to be added

  lastMicros = micros();
}

void storeIsotpframe(CAN_frame_t &frame, uint8_t bus)
{
  // if there is content and this is the frame we are waiting for
  if (frame.FIR.B.DLC > 0 && frame.MsgID == isoMessageIncoming.id)
  {

    uint8_t type = frame.data.u8[0] >> 4; // type = first nibble

    // single frame answer ***************************************************
    if (type == 0x0)
    {
      if (isotp_config->mode_debug & DEBUG_BUS_RECEIVE_ISO)
      {
        Serial.print("< can:ISO SING:");
        Serial.print(canFrameToString(frame));
      }

      uint16_t messageLength = frame.data.u8[0] & 0x0f; // length = second nibble + second byte
      if (messageLength > 7)
        messageLength = 7; // this should never happen
      isoMessageIncoming.length = messageLength;

      // fill up with this initial first-frame data (should always be 6)
      for (int i = 1; i < frame.FIR.B.DLC && isoMessageIncoming.index < isoMessageIncoming.length; i++)
      {
        isoMessageIncoming.data[isoMessageIncoming.index++] = frame.data.u8[i];
      }
      if (isotp_config->mode_debug & DEBUG_BUS_RECEIVE_ISO)
        Serial.print("> can:ISO MSG:");
      if (isotp_config->output_handler)
        isotp_config->output_handler(isoMessageToString(isoMessageIncoming));
      isoMessageIncoming.id = 0xffff; // cancel this message so nothing will be added intil it is re-initialized
    }

    // first frame of a multi-framed message *********************************
    else if (type == 0x1)
    {
      if (isotp_config->mode_debug & DEBUG_BUS_RECEIVE_ISO)
      {
        Serial.print("< can:ISO FRST:");
        Serial.print(canFrameToString(frame));
      }

      // start by requesting requesing the type Consecutive (0x2) frames by sending a Flow frame
      can_send_flow(isoMessageOutgoing.id, bus);

      uint16_t messageLength = (frame.data.u8[0] & 0x0f) << 8; // length = second nibble + second byte
      messageLength |= frame.data.u8[1];
      if (messageLength > 4096)
        messageLength = 4096; // this should never happen
      isoMessageIncoming.length = messageLength;
      for (int i = 2; i < 8; i++)
      {
        isoMessageIncoming.data[isoMessageIncoming.index++] = frame.data.u8[i];
      }
    }

    // consecutive frame(s) **************************************************
    else if (type == 0x2)
    {
      if (isotp_config->mode_debug & DEBUG_BUS_RECEIVE_ISO)
      {
        Serial.print("< can:ISO NEXT:");
        Serial.print(canFrameToString(frame));
      }

      uint8_t sequence = frame.data.u8[0] & 0x0f;
      if (isoMessageIncoming.next == sequence)
      {
        for (int i = 1; i < frame.FIR.B.DLC && isoMessageIncoming.index < isoMessageIncoming.length; i++)
        {
          isoMessageIncoming.data[isoMessageIncoming.index++] = frame.data.u8[i];
        }

        // wait for next message, rollover from 15 to 0
        isoMessageIncoming.next = isoMessageIncoming.next == 15 ? 0 : isoMessageIncoming.next + 1;

        // is this the last part?
        if (isoMessageIncoming.index == isoMessageIncoming.length)
        {
          // output the data
          String dataString = isoMessageToString(isoMessageIncoming);
          if (isotp_config->mode_debug & DEBUG_BUS_RECEIVE_ISO)
            Serial.print("> can:ISO MSG:");
          if (isotp_config->output_handler)
            isotp_config->output_handler(dataString);
          isoMessageIncoming.id = 0xffff; // cancel this message so nothing will be added intil it is re-initialized
        }
      }
      else
      {
        if (isotp_config->mode_debug)
          Serial.println("< can:ISO Out of sequence, resetting");
        isoMessageIncoming.id = 0xffff; // cancel this message so nothing will be added intil it is re-initialized
      }

      // incoming flow control ***********************************************
    }
    else if (type == 0x3)
    {
      //uint8_t flag = isoMessageIncoming.data[0] &0x0f;
      isoMessageOutgoing.flow_counter = frame.data.u8[1];
      isoMessageOutgoing.flow_delay = frame.data.u8[2] <= 127 ? frame.data.u8[2] * 1000 : frame.data.u8[2] - 0xf0;
      // to avoid overwhelming the outgoing queue, set minimum to 5 ms
      // this is experimental.
      if (isoMessageOutgoing.flow_delay < 5000)
        isoMessageOutgoing.flow_delay = 5000;
      isoMessageOutgoing.flow_active = 1;
      lastMicros = micros();
    }
    else
    {
      if (isotp_config->mode_debug)
        Serial.println("< can:ISO ignoring unknown frame type:" + String(type));
    }
  }
  else
  {
    if (isotp_config->mode_debug)
      Serial.println("< can:ISO frame of unrequested id:" + String(frame.MsgID, HEX));
  }
}

void can_send_flow(uint32_t requestId, uint8_t bus)
{
  CAN_frame_t flow;
  flow.FIR.B.FF = requestId < 0x800 ? CAN_frame_std : CAN_frame_ext;
  flow.FIR.B.RTR = CAN_no_RTR; // no RTR
  flow.MsgID = requestId;      // send it to the requestId
  flow.FIR.B.DLC = 8;          // length 8 bytes
  flow.data.u8[0] = 0x30;      // type Flow (3), flag Clear to send (0)
  flow.data.u8[1] = 0x00;      // instruct to send all remaining frames without flow control
  flow.data.u8[2] = 0x00;      // delay between frames <=127 = millis, can maybe set to 0
  flow.data.u8[3] = 0;         // fill-up
  flow.data.u8[4] = 0;         // fill-up
  flow.data.u8[5] = 0;         // fill-up
  flow.data.u8[6] = 0;         // fill-up
  flow.data.u8[7] = 0;         // fill-up
  can_send(&flow, bus);
}

void requestIsotp(uint32_t id, int16_t length, uint8_t *request, uint8_t bus)
{
  CAN_frame_t frame; // build the CAN frame
  // only accept this command if the requested ID belongs to an ISO-TP frame
  //if (id < 0x700 || id > 0x7ff) {
  //  if (isotp_config->output_handler) isotp_config->output_handler (String (id, HEX) + "\n");
  //  return;
  //}
  // prepare the incoming message
  isoMessageIncoming.id = id;   // expected ID of answer
  isoMessageIncoming.index = 0; // starting
  isoMessageIncoming.next = 1;

  if ((isoMessageOutgoing.id = getRequestId(id)) == 0)
  { // ID to send request to
    if (isotp_config->mode_debug & DEBUG_COMMAND)
      Serial.println("> com:" + String(id, HEX) + " has no corresponding request ID");
    if (isotp_config->output_handler)
      isotp_config->output_handler(String(id, HEX) + "\n");
    return;
  }
  // store request to send
  isoMessageOutgoing.length = length;
  if (isoMessageOutgoing.length > 4096)
    isoMessageOutgoing.length = 4096; // this should never happen (yet)
  for (uint16_t i = 0; i < length; i++)
  {
    isoMessageOutgoing.data[i] = request[i];
  }
  isoMessageOutgoing.index = 0; // start at the beginning
  isoMessageOutgoing.next = 1;

  // Prepare the initial frame
  frame.FIR.B.FF = isoMessageOutgoing.id < 0x800 ? CAN_frame_std : CAN_frame_ext;
  frame.FIR.B.RTR = CAN_no_RTR;        // no RTR
  frame.MsgID = isoMessageOutgoing.id; // set the ID
  frame.FIR.B.DLC = 8;                 //command.requestLength + 1;// set the length. Note some ECU's like DLC 8

  if (isoMessageOutgoing.length <= 6)
  { // send SING frame
    frame.data.u8[0] = (isoMessageOutgoing.length & 0x0f);
    for (int i = 0; i < isoMessageOutgoing.length; i++)
    { // fill up the other bytes with the request
      frame.data.u8[i + 1] = isoMessageOutgoing.data[i];
    }
    for (int i = isoMessageOutgoing.length; i < 7; i++)
    {
      frame.data.u8[i + 1] = 0; // zero out frame
    }
    // send the frame
    if (isotp_config->mode_debug & DEBUG_COMMAND_ISO)
    {
      Serial.print("> com:Sending ISOTP SING request:");
      Serial.print(canFrameToString(frame));
    }
    can_send(&frame, bus);
    // --> any incoming frames with the given id will be handled by "storeFrame"
    // and send off if complete. But ensure the ticker doesn't do any flow_block
    // controle
    isoMessageOutgoing.length = isoMessageOutgoing.index = 0;
  }
  else
  { // send a FIRST frame
    frame.data.u8[0] = (uint8_t)(0x10 + ((length >> 8) & 0x0f));
    frame.data.u8[1] = (uint8_t)(length & 0xff);
    for (int i = 0; i < 6; i++)
    { // fill up the other bytes with the request
      frame.data.u8[i + 2] = isoMessageOutgoing.data[isoMessageOutgoing.index++];
    }
    // send the frame
    if (isotp_config->mode_debug & DEBUG_COMMAND_ISO)
    {
      Serial.print("> com:Sending ISOTP FRST request:");
      Serial.print(canFrameToString(frame));
    }
    can_send(&frame, bus);
    // --> any incoming frames with the given id will be handled by "storeFrame" and send off if complete
  }
}

// convert a ISO-TP message to readable hex output format
String isoMessageToString(ISO_MESSAGE_t &message)
{
  String dataString = String(message.id, HEX) + ",";
  for (int i = 0; i < message.length; i++)
  {
    dataString += getHex(message.data[i]);
  }
  dataString += "\n";
  return dataString;
}

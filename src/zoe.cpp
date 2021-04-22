#include "zoe.h"

// ZOE CAN computer related functions ****************************************
uint32_t getRequestId(uint32_t responseId)
{
  // from ECU id   to ECU id
  if (responseId == 0x7ec)
    return 0x7e4; // EVC 11 bits
  else if (responseId == 0x7da)
    return 0x7ca; // TCU
  else if (responseId == 0x7bb)
    return 0x79b; // LBC
  else if (responseId == 0x77e)
    return 0x75a; // PEB
  else if (responseId == 0x772)
    return 0x752; // Airbag
  else if (responseId == 0x76d)
    return 0x74d; // UDP 11 bits
  else if (responseId == 0x763)
    return 0x743; // TDB / CLUSTER / instrument panel
  else if (responseId == 0x762)
    return 0x742; // EPS / PAS
  else if (responseId == 0x760)
    return 0x740; // ABS 11 bits
  else if (responseId == 0x7bc)
    return 0x79c; // UBP
  else if (responseId == 0x765)
    return 0x745; // BCM
  else if (responseId == 0x764)
    return 0x744; // CLIM / HVAC
  else if (responseId == 0x76e)
    return 0x74e; // UPA
  else if (responseId == 0x793)
    return 0x792; // BCB 11 bits
  else if (responseId == 0x7b6)
    return 0x796; // LBC2
  else if (responseId == 0x722)
    return 0x702; // LINSCH
  else if (responseId == 0x767)
    return 0x747; // AUTOS (R-LINK) / NAV

  // new id's for ZE50
  else if (responseId == 0x775)
    return 0x755; // APB
  else if (responseId == 0x7b9)
    return 0x799; // AAU
  else if (responseId == 0x18daf1d2)
    return 0x18dad2f1; // SGS
  else if (responseId == 0x7b9)
    return 0x799; // HFM
  else if (responseId == 0x734)
    return 0x714; // MIU
  else if (responseId == 0x70f)
    return 0x70e; // E-ACT-EBA 11 bits
  else if (responseId == 0x18daf1e2)
    return 0x18dae2f1; // E-ACT-EBA 29 bits
  else if (responseId == 0x18daf1da)
    return 0x18dadaf1; // EVC 29 bits
  else if (responseId == 0x18daf12d)
    return 0x18da2df1; // ABS 29 bits
  else if (responseId == 0x7dd)
    return 0x7dc; // FCAM
  else if (responseId == 0x700)
    return 0x70c; // IDM
  else if (responseId == 0x18daf1df)
    return 0x18dadff1; // INV-ME
  else if (responseId == 0x18dadef1)
    return 0x18daf1de; // BCB 29 bits
  else if (responseId == 0x7c8)
    return 0x7d8; // PLC-PLGW (Power Line connection, I assume CCS)
  else if (responseId == 0x735)
    return 0x723; // FRRAD (Laser radar)
  else if (responseId == 0x7ed)
    return 0x7e5; // LBC 11 bits
  else if (responseId == 0x18daf1db)
    return 0x18dadbf1; // LBC 29 bits
  else if (responseId == 0x7b6)
    return 0x796; // LBC2 11 bits
  else if (responseId == 0x18daf1dc)
    return 0x18dadcf1; // LBC2 29 bits
  else if (responseId == 0x18daf242)
    return 0x18da42f2; // UDP 29 bits
  else if (responseId == 0x761)
    return 0x73f; // VSP
  else if (responseId == 0x7ad)
    return 0x7aa; // WCGS

  else
    return 0;
}

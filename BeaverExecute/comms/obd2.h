#ifndef COMMS_OBD2_H_
#define COMMS_OBD2_H_

#define MIN_OBD2_SID    0x01

void InitOBD2(void);
void ProcessOBD2Data(uint8_t *pData, uint16_t size);

#endif

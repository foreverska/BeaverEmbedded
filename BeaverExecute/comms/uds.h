#ifndef COMMS_UDS_H_
#define COMMS_UDS_H_

#include <stdint.h>

#define MIN_UDS_SID     0x10

void InitUDS(void);
void ProcessUDSData(uint8_t *pData, uint16_t size);
void TickUDSTimer();

uint8_t GetSecurityLevel(void);

#endif

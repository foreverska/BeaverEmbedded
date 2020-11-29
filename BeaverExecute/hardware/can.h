#ifndef COMMS_CAN_H_
#define COMMS_CAN_H_

void InitCan(void);
void Can0ISR(void);
void ProcessCan(void);

void SendCanFrame(uint8_t *pFrame, uint16_t length);

#endif

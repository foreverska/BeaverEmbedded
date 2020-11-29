#ifndef COMMS_ISOTP_H_
#define COMMS_ISOTP_H_

#define MAX_ISOTP_SIZE  1027

typedef struct
{
uint8_t curData[MAX_ISOTP_SIZE];
uint16_t curLength;
uint16_t targetLength;
uint8_t curIndex;
} IsoTpInstance;

void InitIsoTP(void);
void ProcessFrame(uint8_t *pFrame);

void InFrameTimeoutIsr(void);
void OutFrameSendIsr(void);

IsoTpInstance *GetOutInstance(void);
void StartIsoTpOut(void);
void ProcessIsoTp(void);

#endif

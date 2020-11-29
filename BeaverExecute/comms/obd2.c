#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "isotp.h"
#include "obd2.h"

#define SID_POS             0
#define PID_POS             1

#define UNRECOGNIZED        0x7F

#define SID_VEHICINFO       0x09

#define SID_RESPOFFSET      0x40

#define VI_NAME_MESSCOUNT   0x09
#define VI_NAME             0x0A

#define NAME_LENGTH         20

char EcuName[] = "BEAVER ECU";

IsoTpInstance *pOutData;

void InitOBD2()
{
    pOutData = GetOutInstance();
}

void SendUnrecognizedSid(uint8_t pData[], uint16_t size)
{
    pOutData->curData[0] = UNRECOGNIZED;
    pOutData->curData[1] = pData[0];
    pOutData->curData[2] = (uint8_t) 0x31;
    pOutData->targetLength = 3;

    StartIsoTpOut();
}

void SendName()
{
    pOutData->curData[0] = SID_VEHICINFO + SID_RESPOFFSET;
    pOutData->curData[1] = VI_NAME;
    memset(&pOutData->curData[2], ' ', NAME_LENGTH);
    strncpy((char*) &pOutData->curData[2], EcuName, NAME_LENGTH);
    pOutData->targetLength = 22;

    StartIsoTpOut();
}

void SendVehicleInfo(uint8_t *pData, uint16_t size)
{
    uint8_t pid = pData[PID_POS];
    switch (pid)
    {
    case VI_NAME_MESSCOUNT:
        //spec says ignore
        break;
    case VI_NAME:
        SendName();
        break;
    default:
        SendUnrecognizedSid(pData, size);
    }
}

void ProcessOBD2Data(uint8_t *pData, uint16_t size)
{
    uint8_t serviceID = pData[SID_POS];
    switch (serviceID)
    {
    case SID_VEHICINFO:
        SendVehicleInfo(pData, size);
        break;
    default:
        SendUnrecognizedSid(pData, size);
    }

}

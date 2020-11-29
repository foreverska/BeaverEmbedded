#include <stdint.h>
#include <stdbool.h>

#include "isotp.h"
#include "uds.h"
#include "datastore/datastore.h"

#define NEGATIVE    0x7F

#define RESET_ECU   0x11
#define READ_BY_ID  0x22
#define WRITE_BY_ID 0x2E

#define RESET_BL    0x40

#define RESP_OFFSET 0x40

#define SID_POS     0
#define SUBFUNC_POS 1

#define WRITEID_SZ  7

#define DEF_RST_TM  1

#define SUBFUNC_NS  0x11
#define NRC_ILF     0x13
#define NRC_ROOR    0x31
#define NRC_SAD     0x33
#define NRC_GPF     0x72

IsoTpInstance *pOutData;

void InitUDS()
{
    pOutData = GetOutInstance();
}

void SendUnsupportedSid(uint8_t pData[], uint16_t size)
{
    pOutData->curData[0] = NEGATIVE;
    pOutData->curData[1] = pData[0];
    pOutData->curData[2] = 0x00;
    pOutData->targetLength = 3;

    StartIsoTpOut();
}

void SendNrc(uint8_t func, uint8_t val)
{
    pOutData->curData[0] = NEGATIVE;
    pOutData->curData[1] = func;
    pOutData->curData[2] = val;
    pOutData->targetLength = 3;

    StartIsoTpOut();
}

void ResetEcu(uint8_t *pData, uint16_t size)
{
    uint8_t subFunc;

    if (size != 2)
    {
        SendNrc(pData[0], NRC_ILF);
    }
    subFunc = pData[SUBFUNC_POS];
    switch (subFunc)
    {
    case RESET_BL:
        pOutData->curData[0] = RESET_ECU + RESP_OFFSET;
        pOutData->curData[1] = RESET_BL;
        pOutData->curData[2] = DEF_RST_TM;
        pOutData->targetLength = 3;

        StartIsoTpOut();

        //TODO:  Do the reset

        break;
    default:
        pOutData->curData[0] = NEGATIVE;
        pOutData->curData[1] = pData[0];
        pOutData->curData[2] = SUBFUNC_NS;
        pOutData->targetLength = 3;

        StartIsoTpOut();
    }

}

void SendRwIdNrc(uint8_t sid, int retval)
{
    switch (retval)
    {
    case DATASTORE_INVALID:
    case DATASTORE_NOPERM:
        SendNrc(sid, NRC_ROOR);
        break;
    case DATASTORE_NOSEC:
        SendNrc(sid, NRC_SAD);
        break;
    default:
        SendNrc(sid, NRC_GPF);
        break;
    }

    return;
}

void ReadById(uint8_t pData[], uint16_t size)
{
    uint32_t value;
    uint16_t id;
    int retval;

    if (size % 2 == 0 || size == 1)
    {
        SendNrc(pData[0], NRC_ILF);
    }

    pOutData->curData[0] = READ_BY_ID + RESP_OFFSET;
    for (int i = 0; i*2+1 < size; i++)
    {
        id = pData[i*2+1]<<8 | pData[i*2+2];

        retval = ReadId(id, &value, false);
        if (retval == DATASTORE_OK)
        {
            if (pOutData->targetLength + 6 >= MAX_ISOTP_SIZE)
            {
                SendNrc(pData[0], NRC_ILF);
                return;
            }

            pOutData->curData[i*6+1] = id >> 8;
            pOutData->curData[i*6+2] = id;
            pOutData->curData[i*6+3] = value >> 24;
            pOutData->curData[i*6+4] = value >> 16;
            pOutData->curData[i*6+5] = value >> 8;
            pOutData->curData[i*6+6] = value;
            pOutData->targetLength += 6;
        }
        else
        {
            SendRwIdNrc(READ_BY_ID, retval);
            return;
        }
    }

    StartIsoTpOut();
}

void WriteById(uint8_t *pData, uint16_t size)
{
    uint16_t id;
    uint32_t val;
    int retval;

    if (size == WRITEID_SZ)
    {
        SendNrc(pData[0], NRC_ILF);
    }

    id = pOutData->curData[1] << 8 | pOutData->curData[2];
    val = pOutData->curData[3] << 24 | pOutData->curData[4] << 16 |
          pOutData->curData[5] << 8 | pOutData->curData[6];

    retval = WriteId(id, val, false);
    if (retval == DATASTORE_OK)
    {
        pOutData->curData[0] = WRITE_BY_ID + RESP_OFFSET;
        pOutData->curData[1] = pOutData->curData[1];
        pOutData->curData[2] = pOutData->curData[2];
        pOutData->targetLength = 3;
        StartIsoTpOut();
    }
    else
    {
        SendRwIdNrc(WRITE_BY_ID, retval);
    }
}

void ProcessUDSData(uint8_t *pData, uint16_t size)
{
    uint8_t serviceID = pData[SID_POS];
    switch (serviceID)
    {
    case RESET_ECU:
        ResetEcu(pData, size);
        break;
    case READ_BY_ID:
        ReadById(pData, size);
        break;
    case WRITE_BY_ID:
        WriteById(pData, size);
        break;
    default:
        SendUnsupportedSid(pData, size);
    }

}

uint8_t GetSecurityLevel()
{
    return 0;
}

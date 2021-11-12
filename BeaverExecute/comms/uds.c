#include <stdint.h>
#include <stdbool.h>

#include "chacha-portable/chacha-portable.h"

#include "isotp.h"
#include "uds.h"
#include "datastore/datastore.h"
#include "hardware/resetdog.h"
#include "hardware/swrng.h"
#include "hardware/secparams.h"

#define NEGATIVE    0x7F

#define RESET_ECU   0x11
#define READ_BY_ID  0x22
#define SEC_ACC     0x27
#define WRITE_BY_ID 0x2E
#define TST_PRSNT   0x3E

#define RESET_BL    0x40

#define RESP_OFFSET 0x40

#define SID_POS     0
#define SUBFUNC_POS 1

#define WRITEID_SZ  7
#define TP_SIZE     2

#define DEF_RST_TM  1

#define SAT_RSD     0x01
#define SAT_SK      0x02

#define NRC_FNS     0x11
#define NRC_SNS     0x12
#define NRC_ILF     0x13
#define NRC_CNC     0x22
#define NRC_RSE     0x24
#define NRC_ROOR    0x31
#define NRC_SAD     0x33
#define NRC_IK      0x35
#define NRC_ENOA    0x36
#define NRC_RTDNE   0x37
#define NRC_GPF     0x72

#define SA_TIMEOUT      (50)
#define PRIV_TIME       (200)
#define ATT_PER_TIMEOUT (10)
#define ENOA_TIMEOUT    (1000)

static IsoTpInstance *pOutData;

#define CHALLENGE_LEN   (16)
static struct {
    uint8_t currentAccessLevel;
    enum {
       WAITING_CHALLENGE,
       CHALLENGE_SENT,
       CHALLENGE_TIMEOUT
    }challengeState;
    uint8_t challenge[CHALLENGE_LEN];
    uint8_t unsuccessfulAttempts;
    uint16_t timerCounter;
}SecAccInstance;

void InitSecAcc()
{
    SecAccInstance.currentAccessLevel = 0;
    SecAccInstance.challengeState = WAITING_CHALLENGE;
    SecAccInstance.unsuccessfulAttempts = 0;
    SecAccInstance.timerCounter = 0;
}

void InitUDS()
{
    pOutData = GetOutInstance();
    InitSecAcc();
}

void SendUnsupportedSid(uint8_t pData[], uint16_t size)
{
    pOutData->curData[0] = NEGATIVE;
    pOutData->curData[1] = pData[0];
    pOutData->curData[2] = NRC_FNS;
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
        return;
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

        RequestReset();

        break;
    default:
        pOutData->curData[0] = NEGATIVE;
        pOutData->curData[1] = pData[0];
        pOutData->curData[2] = NRC_SNS;
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
        SendNrc(pData[SID_POS], NRC_ILF);
        return;
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

void SendUnlocked()
{
    pOutData->curData[0] = SEC_ACC + RESP_OFFSET;
    pOutData->curData[1] = SAT_RSD;
    pOutData->curData[2] = 0x00;
    pOutData->curData[3] = 0x00;
    pOutData->targetLength = 4;

    StartIsoTpOut();
}

void SendChallenge()
{
    if (SecAccInstance.challengeState == CHALLENGE_SENT)
    {
        SendNrc(SEC_ACC, NRC_CNC);
        return;
    }

    if (SecAccInstance.currentAccessLevel == SAT_RSD)
    {
        SendUnlocked();
        return;
    }

    if (SecAccInstance.challengeState == CHALLENGE_TIMEOUT)
    {
        SendNrc(SEC_ACC, NRC_RTDNE);
        return;
    }

    SecAccInstance.challengeState = CHALLENGE_SENT;
    SecAccInstance.timerCounter = SA_TIMEOUT;
    uint32_t *pChallenge = (uint32_t*) SecAccInstance.challenge;
    for (int i = 0; i < CHALLENGE_LEN/4; i++)
    {
        pChallenge[i] = GetSwRand();
    }

    pOutData->curData[0] = SEC_ACC + RESP_OFFSET;
    pOutData->curData[1] = SAT_RSD;
    for (int i = 0; i < CHALLENGE_LEN; i++)
    {
        pOutData->curData[2+i] = SecAccInstance.challenge[i];
    }
    pOutData->targetLength = CHALLENGE_LEN + 2;

    StartIsoTpOut();
}

void InvalidKey()
{
    SecAccInstance.unsuccessfulAttempts++;
    if (SecAccInstance.unsuccessfulAttempts > ATT_PER_TIMEOUT)
    {
        SecAccInstance.challengeState = CHALLENGE_TIMEOUT;
        SecAccInstance.timerCounter = ENOA_TIMEOUT;
        SendNrc(SEC_ACC, NRC_ENOA);
        return;
    }

    SendNrc(SEC_ACC, NRC_IK);
}

void VerifyKey(uint8_t *pData, uint16_t size)
{
    uint8_t EncChallenge[CHALLENGE_LEN];
    bool correct = true;

    if (SecAccInstance.challengeState != CHALLENGE_SENT)
    {
        SendNrc(SEC_ACC, NRC_RSE);
        return;
    }

    chacha20_xor_stream(EncChallenge, SecAccInstance.challenge,
                        CHALLENGE_LEN, SecParams.SymmetricKey,
                        ChaChaNonce, 0);

    for (int i = 0; i < CHALLENGE_LEN; i++)
    {
        if (EncChallenge[i] != pData[i+2])
        {
            correct = false;
        }
    }

    SecAccInstance.challengeState = WAITING_CHALLENGE;

    if (correct == false)
    {
        InvalidKey();
        return;
    }

    SecAccInstance.currentAccessLevel = SAT_RSD;
    SecAccInstance.timerCounter = PRIV_TIME;

    pOutData->curData[0] = SEC_ACC + RESP_OFFSET;
    pOutData->curData[1] = SAT_SK;
    StartIsoTpOut();
}

void SecurityAccess(uint8_t *pData, uint16_t size)
{
    uint8_t subFunc;

    if (size != 2 && size != (CHALLENGE_LEN + 2))
    {
        SendNrc(pData[SID_POS], NRC_ILF);
        return;
    }

    subFunc = pData[SUBFUNC_POS];

    if ((subFunc == SAT_RSD && size != 2) ||
        (subFunc == SAT_SK && size != 18))
    {
        SendNrc(pData[SID_POS], NRC_ILF);
        return;
    }

    switch (subFunc)
    {
    case SAT_RSD:
        SendChallenge();
        break;
    case SAT_SK:
        VerifyKey(pData, size);
        break;
    default:
        SendNrc(pData[SID_POS], NRC_SNS);
    }
}

void WriteById(uint8_t *pData, uint16_t size)
{
    uint16_t id;
    uint32_t val;
    int retval;

    if (size != WRITEID_SZ)
    {
        SendNrc(pData[0], NRC_ILF);
    }

    id = pData[1] << 8 | pData[2];
    val = pData[3] << 24 | pData[4] << 16 | pData[5] << 8 | pData[6];

    retval = WriteId(id, val, false);
    if (retval != DATASTORE_OK)
    {
        SendRwIdNrc(WRITE_BY_ID, retval);
        return;
    }

    pOutData->curData[0] = WRITE_BY_ID + RESP_OFFSET;
    pOutData->curData[1] = pData[1];
    pOutData->curData[2] = pData[2];
    pOutData->targetLength = 3;
    StartIsoTpOut();
}

void TesterPresent(uint8_t *pData, uint16_t size)
{
    if (size != TP_SIZE)
    {
        SendNrc(pData[0], NRC_ILF);
        return;
    }

    if (pData[SUBFUNC_POS] != 0)
    {
        SendNrc(pData[0], NRC_SNS);
        return;
    }

    if (SecAccInstance.currentAccessLevel > 0)
    {
        SecAccInstance.timerCounter = PRIV_TIME;
    }

    pOutData->curData[0] = TST_PRSNT + RESP_OFFSET;
    pOutData->curData[1] = 0;
    pOutData->targetLength = 2;
    StartIsoTpOut();
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
    case SEC_ACC:
        SecurityAccess(pData, size);
        break;
    case WRITE_BY_ID:
        WriteById(pData, size);
        break;
    case TST_PRSNT:
        TesterPresent(pData, size);
        break;
    default:
        SendUnsupportedSid(pData, size);
    }
}

uint8_t GetSecurityLevel()
{
    return SecAccInstance.currentAccessLevel;
}

void TickUDSTimer()
{
    if(SecAccInstance.timerCounter == 0)
    {
        return;
    }

    SecAccInstance.timerCounter--;

    if(SecAccInstance.timerCounter > 0)
    {
        return;
    }

    switch (SecAccInstance.challengeState)
    {
    case WAITING_CHALLENGE:
        return;
    case CHALLENGE_SENT:
        SecAccInstance.challengeState = WAITING_CHALLENGE;
        break;
    case CHALLENGE_TIMEOUT:
        SecAccInstance.challengeState = WAITING_CHALLENGE;
        SecAccInstance.unsuccessfulAttempts = 0;
        break;
    }

    if (SecAccInstance.currentAccessLevel > 0)
    {
        SecAccInstance.currentAccessLevel = 0;
    }
}

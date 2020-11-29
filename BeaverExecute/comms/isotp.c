#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"

#include "hardware/can.h"
#include "isotp.h"
#include "obd2.h"
#include "uds.h"

#define FRAME_SIZE      8
#define HIGHERNIBBLE    0xF0
#define LOWERNIBBLE     0x0F

#define SINGLEFRAME     0x00
#define FIRSTFRAME      0x10
#define CONSECFRAME     0x20
#define FLOWFRAME       0x30

#define DATA_IN_SF      7
#define DATA_IN_FF      6
#define DATA_IN_CF      7

#define ISOTP_CONT      0
#define ISOTP_WAIT      1
#define ISOTP_ABORT     2

#define TIMER_PRESCALE  255
#define TIMER_MS(x) ((SysCtlClockGet()/(TIMER_PRESCALE+1)) / 1000.0 * x)

#define DEFAULT_SEP     10
#define MAX_FLOW_WAIT   127
#define IN_FLOW_WAIT    200

#define NextIndex(x) ((x + 1) & LOWERNIBBLE)

IsoTpInstance inData, outData;

uint8_t inFrame[FRAME_SIZE];
uint8_t outFrame[FRAME_SIZE];
bool flowFrameRecv = false;

bool inFrameTimeout = false;
bool outFrameReady = false;


void InitTimers()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER5);
    TimerConfigure(TIMER5_BASE, TIMER_CFG_SPLIT_PAIR |
                   TIMER_CFG_A_PERIODIC | TIMER_CFG_B_PERIODIC);
    IntEnable(INT_TIMER5A);
    IntEnable(INT_TIMER5B);
    TimerIntEnable(TIMER5_BASE, TIMER_TIMA_TIMEOUT | TIMER_TIMB_TIMEOUT);
    TimerPrescaleSet(TIMER5_BASE, TIMER_A, TIMER_PRESCALE);
    TimerPrescaleSet(TIMER5_BASE, TIMER_B, TIMER_PRESCALE);
}

void ResetIsoTpInstance(IsoTpInstance *pInstance)
{
    memset(pInstance->curData, 0, MAX_ISOTP_SIZE);
    pInstance->curLength = 0;
    pInstance->targetLength = 0;
    pInstance->curIndex = 0;
}

void InitIsoTP()
{
    ResetIsoTpInstance(&inData);
    ResetIsoTpInstance(&outData);
    flowFrameRecv = false;
    InitTimers();

    InitOBD2();
    InitUDS();
}

void SendFlowControl(uint8_t allowed)
{
    outFrame[0] = FLOWFRAME | allowed;
    outFrame[1] = 0;
    outFrame[2] = DEFAULT_SEP;

    SendCanFrame(outFrame, 3);
}

void StartFrameInTimer()
{
    TimerLoadSet(TIMER5_BASE, TIMER_A, TIMER_MS(IN_FLOW_WAIT));
    TimerEnable(TIMER5_BASE, TIMER_A);
    inFrameTimeout = false;
}

void ResetFrameInTimer()
{
    IntDisable(INT_TIMER5A);
    TimerLoadSet(TIMER5_BASE, TIMER_A, TIMER_MS(IN_FLOW_WAIT));
    IntEnable(INT_TIMER5A);
}

void StopFrameInTimer()
{
    TimerDisable(TIMER5_BASE, TIMER_A);
}

void InFrameTimeoutIsr()
{
    TimerIntClear(TIMER5_BASE, TIMER_A);
    inFrameTimeout = true;
    StopFrameInTimer();
}

void StartOutFrameTimer(uint16_t ms)
{
    uint32_t timeout = TIMER_MS(ms);
    TimerLoadSet(TIMER5_BASE, TIMER_B, timeout);
    TimerEnable(TIMER5_BASE, TIMER_B);
    outFrameReady = false;
}

void ResetOutFrameTimer(uint16_t ms)
{
    uint32_t timeout = TIMER_MS(ms);
    IntDisable(INT_TIMER5B);
    TimerLoadSet(TIMER5_BASE, TIMER_B, timeout);
    IntEnable(INT_TIMER5B);
}

void StopOutFrameTimer()
{
    TimerDisable(TIMER5_BASE, TIMER_B);
}

void OutFrameSendIsr()
{
    TimerIntClear(TIMER5_BASE, TIMER_B);
    outFrameReady = true;
}

void RouteIsoTpData(uint8_t *pData, uint16_t dataSize)
{
    uint8_t serviceID = pData[0];

    if (serviceID >= MIN_OBD2_SID && serviceID < MIN_UDS_SID)
    {
        ProcessOBD2Data(pData, dataSize);
    }
    else if (serviceID >= MIN_UDS_SID)
    {
        ProcessUDSData(pData, dataSize);
    }
    else
    {
        SendFlowControl(ISOTP_ABORT);
    }
}

void ProcessSingleFrame(uint8_t *pFrame)
{
    uint8_t dataSize = pFrame[0] & LOWERNIBBLE;

    RouteIsoTpData(&pFrame[1], dataSize);
}

void ProcessFirstFrame(uint8_t *pFrame)
{
    uint16_t targetSize;

    if (inData.targetLength != 0)
    {
        SendFlowControl(ISOTP_WAIT);
    }

    targetSize = ((pFrame[0] & LOWERNIBBLE) << 8) + pFrame[1];
    if (targetSize > MAX_ISOTP_SIZE)
    {
        SendFlowControl(ISOTP_ABORT);
    }

    inData.targetLength = targetSize;
    memcpy(inData.curData, &pFrame[2], DATA_IN_FF);
    inData.curLength = DATA_IN_FF;

    SendFlowControl(ISOTP_CONT);
    StartFrameInTimer();
}

void ProcessConsecFrame(uint8_t *pFrame)
{
    if (inData.targetLength == 0)
    {
        SendFlowControl(ISOTP_ABORT);
    }

    uint8_t index = pFrame[0] & LOWERNIBBLE;
    if (NextIndex(inData.curIndex) != index)
    {
        ResetIsoTpInstance(&inData);
        SendFlowControl(ISOTP_ABORT);
        StopFrameInTimer();
    }

    memcpy(&inData.curData[inData.curLength], &pFrame[1], DATA_IN_CF);
    inData.curLength += DATA_IN_CF;
    inData.curIndex = NextIndex(inData.curIndex);
    if (inData.curLength >= inData.targetLength)
    {
        RouteIsoTpData(inData.curData, inData.targetLength);
        ResetIsoTpInstance(&inData);
        flowFrameRecv = true;
        StopFrameInTimer();
    }
    else
    {
        ResetFrameInTimer();
    }
}

void ProcessFlowFrame(uint8_t *pFrame)
{
    uint8_t flowCmd = pFrame[0] & LOWERNIBBLE;
    uint8_t blockSize = pFrame[1];
    uint8_t seperationTime = pFrame[2];

    if (blockSize != 0)
    {
        //UNSUPPORTED
        StopOutFrameTimer();
        ResetIsoTpInstance(&outData);
        flowFrameRecv = false;
        return;
    }

    if (flowCmd == ISOTP_CONT)
    {
        if (seperationTime < DEFAULT_SEP)
        {
            seperationTime = DEFAULT_SEP;
        }
        flowFrameRecv = true;
        ResetOutFrameTimer(seperationTime);
    }
    else
    {
        StopOutFrameTimer();
        ResetIsoTpInstance(&outData);
        flowFrameRecv = false;
    }
}

void ProcessFrame(uint8_t *pFrame)
{
    uint8_t frameType = pFrame[0] & HIGHERNIBBLE;
    memcpy(inFrame, pFrame, 8);

    switch (frameType)
    {
    case SINGLEFRAME:
        ProcessSingleFrame(inFrame);
        break;
    case FIRSTFRAME:
        ProcessFirstFrame(inFrame);
        break;
    case CONSECFRAME:
        ProcessConsecFrame(inFrame);
        break;
    case FLOWFRAME:
        ProcessFlowFrame(inFrame);
        break;
    default:
        SendFlowControl(ISOTP_ABORT);
    }
}

IsoTpInstance *GetOutInstance()
{
    return &outData;
}

void StartIsoTpOut()
{
    if (outData.targetLength <= DATA_IN_SF)
    {
        outFrame[0] = SINGLEFRAME | outData.targetLength;
        memcpy(&outFrame[1], outData.curData, outData.targetLength);
        SendCanFrame(outFrame, outData.targetLength+1);
        ResetIsoTpInstance(&outData);
        flowFrameRecv = false;
    }
    else
    {
        outFrame[0] = FIRSTFRAME | (outData.targetLength>>8);
        outFrame[1] = outData.targetLength & 0xFF;
        memcpy(&outFrame[2], outData.curData, DATA_IN_FF);
        SendCanFrame(outFrame, FRAME_SIZE);
        outData.curLength += DATA_IN_FF;
        flowFrameRecv = false;
        StartOutFrameTimer(MAX_FLOW_WAIT);
    }
}

void InFrameTimeout()
{
    ResetIsoTpInstance(&inData);
    SendFlowControl(ISOTP_ABORT);
    inFrameTimeout = false;
}

void SendOutFrame()
{
    uint8_t numBytes = outData.targetLength - outData.curLength;

    if (flowFrameRecv == false)
    {
        StopOutFrameTimer();
        ResetIsoTpInstance(&outData);
        outFrameReady = false;
        return;
    }

    if (numBytes > 7)
    {
        numBytes = 7;
    }

    outFrame[0] = CONSECFRAME | outData.curIndex;
    memcpy(&outFrame[1], &outData.curData[outData.curLength], numBytes);
    outData.curLength += numBytes;

    SendCanFrame(outFrame, numBytes+1);

    outData.curIndex = NextIndex(outData.curIndex);
    if (outData.curLength >= outData.targetLength)
    {
        StopOutFrameTimer();
        ResetIsoTpInstance(&outData);
        flowFrameRecv = false;
    }

    outFrameReady = false;
}

void ProcessIsoTp()
{
    if (inFrameTimeout == true)
    {
        InFrameTimeout();
    }

    if (outFrameReady == true)
    {
        SendOutFrame();
    }
}

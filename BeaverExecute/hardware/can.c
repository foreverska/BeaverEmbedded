#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "inc/hw_can.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/can.h"
#include "driverlib/interrupt.h"

#include "comms/isotp.h"

#define CAN0RXID    0x7E2
#define CAN0TXID    0x7EA
#define RXOBJECT    1
#define TXOBJECT    2

uint32_t rxMsgCount = 0;
bool rxReady = false;

tCANMsgObject can0RxMessage;
tCANMsgObject can0TxMessage;

#define MAX_CAN_SIZE    8

uint8_t rxMsgData[MAX_CAN_SIZE];
uint8_t txMsgData[MAX_CAN_SIZE];

void CanSetMessageObjects(void)
{
    can0RxMessage.ui32MsgID = CAN0RXID;
    can0RxMessage.ui32MsgIDMask = 0;
    can0RxMessage.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
    can0RxMessage.pui8MsgData = rxMsgData;
    can0RxMessage.ui32MsgLen = MAX_CAN_SIZE;

    CANMessageSet(CAN0_BASE, RXOBJECT, &can0RxMessage, MSG_OBJ_TYPE_RX);

    can0TxMessage.ui32MsgID = CAN0TXID;
    can0TxMessage.ui32MsgIDMask = 0;
    can0TxMessage.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
    can0TxMessage.pui8MsgData = txMsgData;
    can0TxMessage.ui32MsgLen = MAX_CAN_SIZE;
}

void InitCan(void)
{
    rxMsgCount = 0;
    rxReady = false;

    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);
    CANInit(CAN0_BASE);
    CANBitRateSet(CAN0_BASE, SysCtlClockGet(), 1000000);
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_STATUS | CAN_INT_ERROR);
    IntEnable(INT_CAN0);

    CANEnable(CAN0_BASE);

    CanSetMessageObjects();
    InitIsoTP();
}

void Can0ISR(void)
{
    uint32_t ui32Status;

    ui32Status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);

    if (ui32Status == RXOBJECT)
    {
        CANIntClear(CAN0_BASE, RXOBJECT);
        rxMsgCount++;
        rxReady = true;
    }
    else
    {
        CANIntClear(CAN0_BASE, ui32Status);
    }
}

void ProcessCan()
{
    while (rxMsgCount > 0)
    {
        rxMsgCount--;
        CANMessageGet(CAN0_BASE, RXOBJECT, &can0RxMessage, true);
        ProcessFrame(rxMsgData);
    }
    ProcessIsoTp();
}

void SendCanFrame(uint8_t *pFrame, uint16_t length)
{
    memset(txMsgData, '\0', MAX_CAN_SIZE);
    memcpy(txMsgData, pFrame, length);
    can0TxMessage.ui32MsgLen = MAX_CAN_SIZE;

    CANMessageSet(CAN0_BASE, TXOBJECT, &can0TxMessage, MSG_OBJ_TYPE_TX);
}



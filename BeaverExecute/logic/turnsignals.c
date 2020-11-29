#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/timer.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"

#include <logic/turnsignals.h>
#include <datastore/datastore.h>

uint8_t turnState = NO_TURN;
uint8_t flashState = 0x0;

#define LIGHTSOFF           0xFF
#define LIGHTSON            0x00

void InitTurnsignal()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet()/3);
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
}

void UpdateDatastore()
{
    uint32_t turnsignals = 0x0;

    if (turnState == LEFT_TURN)
    {
        turnsignals |= ((OUT_FLTS | OUT_RLTS) & flashState);
    }
    else if (turnState == RIGHT_TURN)
    {
        turnsignals |= ((OUT_FRTS | OUT_RRTS) & flashState);
    }

    MaskedWriteId(ENTRY_OUTPUTS, turnsignals, OUT_TURN_MASK, true);
}

void ProcessTurnsignal()
{
    flashState = ~flashState;

    UpdateDatastore();

    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
}

void TimerReset()
{
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet()/3);
    TimerEnable(TIMER0_BASE, TIMER_A);
}

void SetupTurn(uint8_t turn)
{
    if (turn == turnState)
    {
        return;
    }
    turnState = turn;
    flashState = 0xFF;

    UpdateDatastore();
    TimerReset();
}

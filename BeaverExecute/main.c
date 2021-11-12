#include <stdint.h>
#include <stdbool.h>

#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/can.h"
#include "driverlib/systick.h"
#include "inc/hw_memmap.h"

#ifdef DEBUG
#include "inc/hw_nvic.h"
#endif

#include "pinout.h"

#include "hardware/can.h"
#include "datastore/datastore.h"
#include "logic/turnsignals.h"
#include "logic/headlights.h"
#include "hardware/inputs.h"
#include "hardware/outputs.h"
#include "hardware/resetdog.h"
#include "hardware/swrng.h"
#include "comms/uds.h"

#define TOGGLE_TIME 1000000

static bool systickInt;

void SystickISR()
{
    systickInt = true;
}

void InitSystick()
{
    systickInt = false;

    SysTickPeriodSet(SysCtlClockGet()/100);
    SysTickIntEnable();
    SysTickEnable();
}

int main(void)
{
#ifdef DEBUG
    extern uint32_t g_pfnVectors;
    *((uint32_t*) NVIC_VTABLE) = (uint32_t) &g_pfnVectors;
#endif

    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    PinoutSet();
    InitDatastore();
    InitCan();
    InitTurnsignal();
    InitHeadlights();
    InitInputs();
    InitOutputs();
    InitResetWatchdog();
    InitSystick();

    InitSwRNG();

    while(1)
    {
        if (systickInt == true)
        {
            systickInt = false;

            ProcessCan();
            ProcessHeadlights();
            ProcessOutputs();
            FeedWatchdog();
            TickUDSTimer();
        }

        SysCtlSleep();
    }
}

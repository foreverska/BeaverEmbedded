#include <stdint.h>
#include <stdbool.h>

#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/watchdog.h"
#include "driverlib/interrupt.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

static bool reset;

void InitResetWatchdog()
{
    reset = false;

    //DIRTY HACKY Requirement to enable PIOSC since wdog1 sysctl enable doesn't
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG1);

#ifdef DEBUG
    WatchdogStallEnable(WATCHDOG1_BASE);
#endif
    WatchdogResetEnable(WATCHDOG1_BASE);
    WatchdogReloadSet(WATCHDOG1_BASE, SysCtlClockGet());
    WatchdogEnable(WATCHDOG1_BASE);
    WatchdogLock(WATCHDOG1_BASE);
}

void FeedWatchdog()
{
    if (reset == true || WatchdogIntStatus(WATCHDOG1_BASE, true) == 0x00)
    {
        return;
    }

    WatchdogIntClear(WATCHDOG1_BASE);
}

void RequestReset()
{
    reset = true;
}

uint32_t GetWatchdogCount()
{
    return WatchdogValueGet(WATCHDOG1_BASE);
}

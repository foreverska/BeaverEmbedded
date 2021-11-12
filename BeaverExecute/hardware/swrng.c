#include <stdbool.h>
#include <stdint.h>

#include "utils/random.h"
#include "utils/ustdlib.h"
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"

#include "hardware/resetdog.h"

#define RESEED_RESET    (200)
static uint32_t reseedCount;

void AddEntropy()
{
    uint32_t entropy = GetWatchdogCount() ^ SysTickValueGet();
    RandomAddEntropy(entropy);
}

void InitSwRNG()
{
    reseedCount = RESEED_RESET;

    for (int i = 0; i < 64; i++)
    {
        AddEntropy();
        SysCtlDelay(SysCtlClockGet()/1000);
    }
    usrand(RandomSeed());
}

uint32_t GetSwRand()
{
    reseedCount--;
    if (reseedCount == 0)
    {
        reseedCount = RESEED_RESET;
        AddEntropy();
        usrand(RandomSeed());
    }

    return urand();
}

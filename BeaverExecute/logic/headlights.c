#include <stdint.h>
#include <stdbool.h>

#include "headlights.h"
#include "datastore/datastore.h"

uint8_t headlightStatus = 0x00;

void InitHeadlights(void)
{

}

void ProcessHeadlights(void)
{
    uint32_t headlights = 0x0;

    if ((headlightStatus & LOWBEAM) == LOWBEAM)
    {
        headlights |= (OUT_LLB | OUT_RLB);
    }

    if ((headlightStatus & HIGHBEAM) == HIGHBEAM)
    {
        headlights |= (OUT_LHB | OUT_RHB);
    }

    MaskedWriteId(ENTRY_OUTPUTS, headlights, OUT_HEADLIGHT_MASK, true);
}

void SetHeadlightStatus(uint8_t status)
{
    headlightStatus = status;
}

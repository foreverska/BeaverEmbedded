#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"

#include "outputs.h"
#include "datastore/datastore.h"

#define FL_TURN         GPIO_PIN_4
#define RL_TURN         GPIO_PIN_2
#define FR_TURN         GPIO_PIN_3
#define RR_TURN         GPIO_PIN_4
#define LEFT_LOWBEAM    GPIO_PIN_6
#define RIGHT_LOWBEAM   GPIO_PIN_7
#define LEFT_HIGHBEAM   GPIO_PIN_1
#define RIGHT_HIGHBEAM  GPIO_PIN_2

#define LIGHTON         0x00
#define LIGHTOFF        0xFF


void InitOutputs()
{
    GPIOPinWrite(GPIO_PORTA_BASE, LEFT_LOWBEAM | RIGHT_LOWBEAM |
                 RL_TURN | FR_TURN | RR_TURN, LIGHTOFF);
    GPIOPinWrite(GPIO_PORTE_BASE, LEFT_HIGHBEAM | RIGHT_HIGHBEAM, LIGHTOFF);
    GPIOPinWrite(GPIO_PORTF_BASE, FL_TURN, LIGHTOFF);
}

void ProcessPortA(uint32_t outputs)
{
    uint8_t pins = 0x0;

    if ((outputs & OUT_LLB) == OUT_LLB)
    {
        pins |= LEFT_LOWBEAM;
    }
    if ((outputs & OUT_RLB) == OUT_RLB)
    {
        pins |= RIGHT_LOWBEAM;
    }
    if ((outputs & OUT_RLTS) == OUT_RLTS)
    {
        pins |= RL_TURN;
    }
    if ((outputs & OUT_FRTS) == OUT_FRTS)
    {
        pins |= FR_TURN;
    }
    if ((outputs & OUT_RRTS) == OUT_RRTS)
    {
        pins |= RR_TURN;
    }

    GPIOPinWrite(GPIO_PORTA_BASE, LEFT_LOWBEAM | RIGHT_LOWBEAM |
                     RL_TURN | FR_TURN | RR_TURN, LIGHTOFF);
    GPIOPinWrite(GPIO_PORTA_BASE, pins, LIGHTON);
}

void ProcessPortE(uint32_t outputs)
{
    uint8_t pins = 0x0;

    if ((outputs & OUT_LHB) == OUT_LHB)
    {
        pins |= LEFT_HIGHBEAM;
    }
    if ((outputs & OUT_RHB) == OUT_RHB)
    {
        pins |= RIGHT_HIGHBEAM;
    }

    GPIOPinWrite(GPIO_PORTE_BASE, LEFT_HIGHBEAM | RIGHT_HIGHBEAM, LIGHTOFF);
    GPIOPinWrite(GPIO_PORTE_BASE, pins, LIGHTON);
}

void ProcessPortF(uint32_t outputs)
{
    uint8_t pins = 0x0;

    if ((outputs & OUT_FLTS) == OUT_FLTS)
    {
        pins |= FL_TURN;
    }

    GPIOPinWrite(GPIO_PORTF_BASE, FL_TURN, LIGHTOFF);
    GPIOPinWrite(GPIO_PORTF_BASE, pins, LIGHTON);
}

void ProcessOutputs()
{
    uint32_t outputs = 0x0;
    uint32_t internal = 0x0;
    uint32_t holds = 0x0;
    uint32_t holdvals = 0x0;

    int retval = 0;

    retval = ReadId(ENTRY_OUTPUTS, &internal, true);
    retval |= ReadId(ENTRY_OUTHOLDS, &holds, true);
    retval |= ReadId(ENTRY_OUTHOLDVALS, &holdvals, true);
    if (retval != DATASTORE_OK)
    {
        return;
    }

    outputs = (internal & ~holds) | (holdvals & holds);

    ProcessPortA(outputs);
    ProcessPortE(outputs);
    ProcessPortF(outputs);
}

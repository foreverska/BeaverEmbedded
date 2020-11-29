#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"

#include "inputs.h"
#include "logic/turnsignals.h"
#include "logic/headlights.h"

#define TURNSTALK_LEFT      GPIO_PIN_6
#define TURNSTALK_RIGHT     GPIO_PIN_7

#define LOWBEAM_SWITCH  GPIO_PIN_2
#define HIGHBEAM_SWITCH GPIO_PIN_3

void SetupPortB(void)
{
    GPIOIntTypeSet(GPIO_PORTB_BASE,
                   TURNSTALK_LEFT | TURNSTALK_RIGHT |
                   HIGHBEAM_SWITCH | LOWBEAM_SWITCH,
                   GPIO_BOTH_EDGES);
    GPIOIntEnable(GPIO_PORTB_BASE, TURNSTALK_LEFT | TURNSTALK_RIGHT |
                  HIGHBEAM_SWITCH | LOWBEAM_SWITCH);


    IntEnable(INT_GPIOB);
}

void SetupPortF(void)
{

}

void InitInputs(void)
{
    SetupPortB();
    SetupPortF();
}

void ReadTurnstalk(uint8_t inputs)
{
    inputs &= TURNSTALK_LEFT | TURNSTALK_RIGHT;
    if (inputs == TURNSTALK_LEFT)
    {
        SetupTurn(LEFT_TURN);
    }
    else if (inputs == TURNSTALK_RIGHT)
    {
        SetupTurn(RIGHT_TURN);
    }
    else
    {
        SetupTurn(NO_TURN);
    }
}

void ReadHeadlights(uint8_t inputs)
{
    uint8_t headlights = 0x0;

    if ((inputs & LOWBEAM_SWITCH) == LOWBEAM_SWITCH)
    {
        headlights |= LOWBEAM;
    }

    if ((inputs & HIGHBEAM_SWITCH) == HIGHBEAM_SWITCH)
    {
        headlights |= HIGHBEAM;
    }

    SetHeadlightStatus(headlights);
}

void PortBInputISR()
{
    int32_t inputs;
    inputs = GPIOPinRead(GPIO_PORTB_BASE,
                         TURNSTALK_LEFT | TURNSTALK_RIGHT |
                         HIGHBEAM_SWITCH | LOWBEAM_SWITCH);

    ReadTurnstalk(inputs);
    ReadHeadlights(inputs);

    GPIOIntClear(GPIO_PORTB_BASE,
                 TURNSTALK_LEFT | TURNSTALK_RIGHT |
                 HIGHBEAM_SWITCH | LOWBEAM_SWITCH);
}

void PortFInputISR()
{

}

#ifndef __TURNSIGNALS_H__
#define __TURNSIGNALS_H__

#define LEFT_TURN   0xAA
#define RIGHT_TURN  0x55
#define NO_TURN     0x33

void InitTurnsignal(void);

void ProcessTurnsignal(void);
void SetupTurn(uint8_t turn);

#endif

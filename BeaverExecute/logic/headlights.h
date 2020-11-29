#ifndef __HEADLIGHTS_H__
#define __HEADLIGHTS_H__

#define HIGHBEAM    0x04
#define LOWBEAM     0x40

void InitHeadlights(void);

void ProcessHeadlights(void);
void SetHeadlightStatus(uint8_t status);

#endif

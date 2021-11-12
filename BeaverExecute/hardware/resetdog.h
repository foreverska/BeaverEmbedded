#ifndef HARDWARE_RESETDOG_H_
#define HARDWARE_RESETDOG_H_

#include <stdint.h>

void InitResetWatchdog();
void FeedWatchdog();
void RequestReset();
uint32_t GetWatchdogCount();

#endif /* HARDWARE_RESETDOG_H_ */

/*
Copyright (c) 2016, Dust Networks. All rights reserved.

Watchdog timer configuration for the SAM C21 Xplained Pro.

\license See attached DN_LICENSE.txt.
*/


#ifndef WATCHDOG_TIMER_H_
#define WATCHDOG_TIMER_H_

#include <wdt.h>

#define WATCHDOG_TIMEOUT_CLK	WDT_PERIOD_8192CLK;
#define WATCHDOG_TIMEOUT_MS		8000 // Timeout in ms corresponding to the above ticks
#define WATCHDOG_WARNING_CLK	WDT_PERIOD_4096CLK;
#define WATCHDOG_WARNING_MS		4000 // Warning in ms corresponding to the above ticks


#ifdef __cplusplus
extern "C" {
#endif

void watchdog_warning_callback(void);
void configure_wdt(void);

#ifdef __cplusplus
}
#endif


#endif /* WATCHDOG_TIMER_H_ */
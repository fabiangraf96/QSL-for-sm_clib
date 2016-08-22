/*
Copyright (c) 2016, Dust Networks. All rights reserved.

Port of the watchdog module to the SAM C21 Xplained Pro.

\license See attached DN_LICENSE.txt.
*/

#include <wdt.h>

#include "dn_watchdog.h"

//=========================== variables =======================================

//=========================== prototypes ======================================

//=========================== public ==========================================

void dn_watchdog_feed(void)
{
	// Reset watchdog timer
	wdt_reset_count();
}

//=========================== private =========================================

//=========================== helpers =========================================
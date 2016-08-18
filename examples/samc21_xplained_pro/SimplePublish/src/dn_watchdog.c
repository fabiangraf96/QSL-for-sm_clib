/*
Copyright (c) 2016, Dust Networks. All rights reserved.

\license See attached DN_LICENSE.txt.
*/

#include "dn_watchdog.h"
#include "wdt.h"

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
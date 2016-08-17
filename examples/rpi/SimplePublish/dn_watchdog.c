/*
Copyright (c) 2016, Dust Networks. All rights reserved.

Port of the watchdog module to the Raspberry Pi.

\license See attached DN_LICENSE.txt.
*/

/* 
 * File:   dn_watchdog.c
 * Author: jhbr@datarespons.no
 *
 * Created on 23. juni 2016, 12:40
 */

#include "dn_watchdog.h"

//=========================== variables =======================================

//=========================== prototypes ======================================

//=========================== public ==========================================

void dn_watchdog_feed(void)
{
	// Nothing to do since we have not implemented watchdog on Raspberry Pi
}

//=========================== private =========================================

//=========================== helpers =========================================
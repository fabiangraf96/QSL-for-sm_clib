/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   dn_watchdog.c
 * Author: jhbr@datarespons.no
 *
 * Created on 9. aug 2016, 10:30
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
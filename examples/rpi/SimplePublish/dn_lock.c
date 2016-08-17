/*
Copyright (c) 2016, Dust Networks. All rights reserved.

Port of the lock module to the Raspberry Pi.

\license See attached DN_LICENSE.txt.
*/

/* 
 * File:   dn_lock.c
 * Author: jhbr@datarespons.no
 *
 * Created on 21. juni 2016, 09:05
 */

#include "dn_lock.h"

//=========================== variables =======================================

//=========================== prototypes ======================================

//=========================== public ==========================================

void dn_lock(void) {
   // this sample is single threaded, no need to lock.
}

void dn_unlock(void) {
   // this sample is single threaded, no need to lock.
}

//=========================== private =========================================

//=========================== helpers =========================================
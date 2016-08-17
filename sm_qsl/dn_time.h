/*
Copyright (c) 2016, Dust Networks. All rights reserved.

\license See attached DN_LICENSE.txt.
*/

/* 
 * File:   dn_time.h
 * Author: jhbr@datarespons.no
 *
 * Created on 23. juni 2016, 12:36
 */

#ifndef DN_TIME_H
#define DN_TIME_H

#include "dn_common.h"

//=========================== defines =========================================

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

#ifdef __cplusplus
 extern "C" {
#endif

uint32_t dn_time_ms(void);
void dn_sleep_ms(uint32_t milliseconds);

#ifdef __cplusplus
}
#endif

#endif
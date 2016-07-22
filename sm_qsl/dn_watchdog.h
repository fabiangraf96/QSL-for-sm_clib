/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   dn_watchdog.h
 * Author: jhbr@datarespons.no
 *
 * Created on 23. juni 2016, 12:36
 */

#ifndef DN_WATCHDOG_H
#define DN_WATCHDOG_H

#include "dn_common.h"

//=========================== defines =========================================

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

#ifdef __cplusplus
 extern "C" {
#endif

void dn_watchdog_feed(void);

#ifdef __cplusplus
}
#endif

#endif
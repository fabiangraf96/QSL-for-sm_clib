/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   dn_qsl_api.h
 * Author: jhbr
 *
 * Created on 23. juni 2016, 12:36
 */

#ifndef DN_QSL_API_H
#define DN_QSL_API_H

#include "dn_common.h"

//=========================== defines =========================================

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

#ifdef __cplusplus
 extern "C" {
#endif

bool dn_qsl_init(void);
bool dn_qsl_isConnected(void);
bool dn_qsl_connect(uint16_t netID, uint8_t* joinKey, uint32_t service_ms);
bool dn_qsl_send(uint8_t* payload, uint8_t payloadSize_B, uint8_t* destIP);
uint8_t dn_qsl_read(uint8_t* payloadBuffer);

#ifdef __cplusplus
}
#endif

#endif
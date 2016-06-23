/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   dn_fsm.h
 * Author: jhbr
 *
 * Created on 22. juni 2016, 15:15
 */

#ifndef DN_FSM_H
#define DN_FSM_H

#include "dn_common.h"

//=========================== defines =========================================

#define MOTE_STATE_IDLE           0x01
#define MOTE_STATE_SEARCHING      0x02
#define MOTE_STATE_NEGOCIATING    0x03
#define MOTE_STATE_CONNECTED      0x04
#define MOTE_STATE_OPERATIONAL    0x05

#define FSM_STATE_DISCONNECTED	0x00
#define FSM_STATE_PRE_JOIN		0x01
#define FSM_STATE_JOINING		0x02
#define FSM_STATE_CONNECTED		0x03
#define FSM_STATE_READY			0x04

#define MOTE_EVENT_MASK_NONE			0x0000
#define MOTE_EVENT_MASK_BOOT			0x0001
#define MOTE_EVENT_MASK_ALARM_CHANGE	0x0002
#define MOTE_EVENT_MASK_TIME_CHANGE		0x0004
#define MOTE_EVENT_MASK_JOIN_FAIL		0x0008
#define MOTE_EVENT_MASK_DISCONNECTED	0x0010
#define MOTE_EVENT_MASK_OPERATIONAL		0x0020
#define MOTE_EVENT_MASK_SVC_CHANGE		0x0080
#define MOTE_EVENT_MASK_JOIN_STARTED	0x0100

#define CMD_PERIOD				1000	// number of ms between two commands being sent
#define SERIAL_RESPONSE_TIMEOUT	500		// max number of ms to wait for response

//=========================== typedef =========================================

typedef void (*fsm_timer_callback)(void);
typedef void (*fsm_reply_callback)(void);


//=========================== variables =======================================

//=========================== prototypes ======================================

#ifdef __cplusplus
extern "C"
{
#endif

void dn_fsm_init(void);
void dn_fsm_run(void);


#ifdef __cplusplus
}
#endif

#endif /* DN_FSM_H */


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

#define FSM_STATE_NOT_INITIALIZED	0x00
#define FSM_STATE_DISCONNECTED		0x01
#define FSM_STATE_CONNECTING		0x02
#define FSM_STATE_JOINING			0x03
#define FSM_STATE_CONNECTED			0x04
#define FSM_STATE_CONNECT_FAILED	0x05
#define FSM_STATE_REQ_SERVICE		0x06
#define FSM_STATE_READY				0x0f
#define FSM_STATE_SENDING			0x10
#define FSM_STATE_SEND_FAILED		0x11

#define MOTE_EVENT_MASK_NONE			0x0000
#define MOTE_EVENT_MASK_BOOT			0x0001
#define MOTE_EVENT_MASK_ALARM_CHANGE	0x0002
#define MOTE_EVENT_MASK_TIME_CHANGE		0x0004
#define MOTE_EVENT_MASK_JOIN_FAIL		0x0008
#define MOTE_EVENT_MASK_DISCONNECTED	0x0010
#define MOTE_EVENT_MASK_OPERATIONAL		0x0020
#define MOTE_EVENT_MASK_SVC_CHANGE		0x0080
#define MOTE_EVENT_MASK_JOIN_STARTED	0x0100

#define RC_OK					0x00
#define RC_ERROR				0x01
#define RC_BUSY					0x03
#define RC_INVALID_LEN			0x04
#define RC_INVALID_STATE		0x05
#define RC_UNSUPPORTED			0x06
#define RC_UNKNOWN_PARAM		0x07
#define RC_UNKNOWN_CMD			0x08
#define RC_WRITE_FAIL			0x09
#define RC_READ_FAIL			0x0a
#define RC_LOW_VOLTAGE			0x0b
#define RC_NO_RESOURCES			0x0c
#define RC_INCOMPLETE_JOIN_INFO	0x0d
#define RC_NOT_FOUND			0x0e
#define RC_INVALID_VALUE		0x0f
#define RC_ACCESS_DENIED		0x10
#define RC_ERASE_FAIL			0x12

#define FSM_RUN_INTERVAL_MS			25
#define BACKOFF_AFTER_RESET_MS		5000
#define CMD_PERIOD_MS				1000	// number of ms between two commands being sent
#define SERIAL_RESPONSE_TIMEOUT_MS	500		// max number of ms to wait for response
#define CONNECT_TIMEOUT_S			120
#define SEND_TIMEOUT_S				5

#define SERVICE_TYPE_BW		0x00
#define SERVICE_ADDRESS		0xFFFE // Manager (only value currently supported)

#define SERVICE_STATE_COMPLETED	0x00
#define SERVICE_STATE_PENDING	0x01

#define PACKET_PRIORITY_LOW		0x00
#define PACKET_PRIORITY_MEDIUM	0x01
#define PACKET_PRIORITY_HIGH	0x02

#define SRC_PORT	0xf0b8
#define DST_PORT	0xf0b8

//=========================== typedef =========================================

typedef void (*fsm_timer_callback)(void);
typedef void (*fsm_reply_callback)(void);


//=========================== variables =======================================

//=========================== prototypes ======================================

#ifdef __cplusplus
extern "C"
{
#endif

void dn_fsm_run(void);


#ifdef __cplusplus
}
#endif

#endif /* DN_FSM_H */


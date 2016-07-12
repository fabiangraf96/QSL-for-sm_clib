/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   dn_fsm.h
 * Author: jhbr@datarespons.no
 *
 * Created on 22. juni 2016, 15:15
 */

#ifndef DN_FSM_H
#define DN_FSM_H

#include "dn_common.h"
#include "dn_defaults.h"

//=========================== defines =========================================

//===== FSM states
#define DN_FSM_STATE_NOT_INITIALIZED	0x00
#define DN_FSM_STATE_DISCONNECTED		0x01
#define DN_FSM_STATE_PRE_JOIN			0x02
#define DN_FSM_STATE_JOINING			0x03
#define DN_FSM_STATE_REQ_SERVICE		0x04
#define DN_FSM_STATE_RESETTING			0x05
#define DN_FSM_STATE_PROMISCUOUS		0x06
#define DN_FSM_STATE_CONNECTED			0x0f
#define DN_FSM_STATE_SENDING			0x10
#define DN_FSM_STATE_SEND_FAILED		0x11

//===== Mote states
#define DN_MOTE_STATE_IDLE           0x01
#define DN_MOTE_STATE_SEARCHING      0x02
#define DN_MOTE_STATE_NEGOCIATING    0x03
#define DN_MOTE_STATE_CONNECTED      0x04
#define DN_MOTE_STATE_OPERATIONAL    0x05

//===== Mote events
#define DN_MOTE_EVENT_MASK_NONE			0x0000
#define DN_MOTE_EVENT_MASK_BOOT			0x0001
#define DN_MOTE_EVENT_MASK_ALARM_CHANGE	0x0002
#define DN_MOTE_EVENT_MASK_TIME_CHANGE	0x0004
#define DN_MOTE_EVENT_MASK_JOIN_FAIL	0x0008
#define DN_MOTE_EVENT_MASK_DISCONNECTED	0x0010
#define DN_MOTE_EVENT_MASK_OPERATIONAL	0x0020
#define DN_MOTE_EVENT_MASK_SVC_CHANGE	0x0080
#define DN_MOTE_EVENT_MASK_JOIN_STARTED	0x0100

//===== Mote response codes
#define DN_RC_OK					0x00
#define DN_RC_ERROR					0x01
#define DN_RC_BUSY					0x03
#define DN_RC_INVALID_LEN			0x04
#define DN_RC_INVALID_STATE			0x05
#define DN_RC_UNSUPPORTED			0x06
#define DN_RC_UNKNOWN_PARAM			0x07
#define DN_RC_UNKNOWN_CMD			0x08
#define DN_RC_WRITE_FAIL			0x09
#define DN_RC_READ_FAIL				0x0a
#define DN_RC_LOW_VOLTAGE			0x0b
#define DN_RC_NO_RESOURCES			0x0c
#define DN_RC_INCOMPLETE_JOIN_INFO	0x0d
#define DN_RC_NOT_FOUND				0x0e
#define DN_RC_INVALID_VALUE			0x0f
#define DN_RC_ACCESS_DENIED			0x10
#define DN_RC_ERASE_FAIL			0x12

//===== Timing
#define DN_FSM_RUN_INTERVAL_MS			10 // Decides how often to kick the FSM
#define DN_MIN_TX_INTERPACKET_DELAY_MS	20 // Minimum delay between each packet sent to the mote (according to LTC5800-IPM spec)
#define DN_CMD_PERIOD_MS				DN_MIN_TX_INTERPACKET_DELAY_MS * 5 // Delay between each command sent to mote
#define DN_SERIAL_RESPONSE_TIMEOUT_MS	500 // Very conservative; commands are expected to be answered within 125 ms
#define DN_CONNECT_TIMEOUT_S			180 // Usually takes 10-60 s, but service req. and promiscuous search can add 60 s each.
#define DN_SEND_TIMEOUT_MS				1000 // Usually takes < 20 ms

//===== Connect
#define DN_PROTOCOL_TYPE_UDP	0x00 // Only currently supported protocol type

#define DN_SERVICE_TYPE_BW		0x00 // Only currently supported service type
#define DN_SERVICE_ADDRESS		0xFFFE // Manager; only currently supported address

#define DN_SERVICE_STATE_COMPLETED	0x00
#define DN_SERVICE_STATE_PENDING	0x01

//===== Send
#define DN_PACKET_PRIORITY_LOW		0x00
#define DN_PACKET_PRIORITY_MEDIUM	0x01 // Recommended for data traffic
#define DN_PACKET_PRIORITY_HIGH		0x02

#define DN_PACKET_ID_NO_NOTIF	0xffff // Do not generate txDone notification

//===== Read
#define DN_INBOX_SIZE	10 // Max number of buffered downstream messages

//===== Reset/disconnect
/*
 Disconnecting will be more graceful, as the mote first notifies neighbors of
 its imminent software reset. It does, however, take much longer:
 About 20 seconds for disconnect vs 5 seconds for reset only.
 */
#define DN_MOTE_DISCONNECT_BEFORE_RESET TRUE

//=========================== typedef =========================================

typedef void (*dn_fsm_timer_cbt)(void);
typedef void (*dn_fsm_reply_cbt)(void);

typedef struct
{
	uint8_t pktBuf[DN_INBOX_SIZE][DN_DEFAULT_PAYLOAD_SIZE_LIMIT];
	uint8_t pktSize[DN_INBOX_SIZE];
	uint8_t head;
	uint8_t tail;
	uint8_t unreadPackets;
} dn_inbox_t;

//=========================== variables =======================================

//=========================== prototypes ======================================

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#endif /* DN_FSM_H */


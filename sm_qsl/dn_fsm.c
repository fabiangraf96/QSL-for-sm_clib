/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "dn_fsm.h"
#include "dn_ipmt.h"
#include "dn_time.h"
#include "dn_watchdog.h"
#include "dn_qsl_api.h"

//=========================== variables =======================================

typedef struct
{
	uint8_t replyBuf[MAX_FRAME_LENGTH];
	uint8_t notifBuf[MAX_FRAME_LENGTH];
	uint8_t socketId;
	uint16_t networkId;
	uint8_t joinKey[JOIN_KEY_LEN];
	uint16_t service_ms;
	fsm_reply_callback replyCb;
	fsm_timer_callback fsmCb;
	uint32_t fsmEventScheduled_ms;
	uint16_t fsmDelay_ms;
	uint32_t events;
	uint8_t state;
} dn_fsm_vars_t;

dn_fsm_vars_t dn_fsm_vars;


//=========================== prototypes ======================================

void dn_ipmt_notif_cb(uint8_t cmdId, uint8_t subCmdId);
void dn_ipmt_reply_cb(uint8_t cmdId);
void fsm_scheduleEvent(uint16_t delay, fsm_timer_callback cb);
void fsm_cancelEvent(void);
void fsm_setReplyCallback(fsm_reply_callback cb);
void api_response_timeout(void);
void api_getMoteStatus(void);
void api_getMoteStatus_reply(void);
void api_openSocket(void);
void api_openSocket_reply(void);
void api_bindSocket(void);
void api_bindSocket_reply(void);
void api_join(void);
void api_join_reply(void);

//=========================== public ==========================================

bool dn_qsl_init(void)
{
	// Reset local variables
	memset(&dn_fsm_vars, 0, sizeof (dn_fsm_vars));

	// Initialize the ipmt module
	dn_ipmt_init
			(
			dn_ipmt_notif_cb,
			dn_fsm_vars.notifBuf,
			sizeof (dn_fsm_vars.notifBuf),
			dn_ipmt_reply_cb
			);

	dn_fsm_vars.state = FSM_STATE_DISCONNECTED;
}

bool dn_qsl_connect(uint16_t netID, uint8_t* joinKey, uint32_t req_service_ms)
{
	uint32_t cmdStart_ms = dn_time_ms();
	switch (dn_fsm_vars.state)
	{
	case FSM_STATE_DISCONNECTED:
		fsm_scheduleEvent(CMD_PERIOD_MS, api_getMoteStatus);
		// Get mote status: IDLE
		// Open socket
		// Bind socket
		// Goto: PRE_JOIN
		break;
	case FSM_STATE_PRE_JOIN:
		// Set netID
		// Set join key
		// join
		// Goto: JOINING
		break;
	case FSM_STATE_JOINING:
		// Join timeout: Retry
		// OPERATIONAL event
		// Goto: CONNECTED
		break;
	case FSM_STATE_CONNECTED:
		// Request service
		// Get service changed
		// Goto: READY
		break;
	case FSM_STATE_READY:
		// Return TRUE
		// New connect in this state; Goto: CONNECTED
		break;
	default:
		break;
	}

	while (dn_fsm_vars.state != FSM_STATE_READY)
	{
		if ((dn_time_ms() - cmdStart_ms) > CONNECT_TIMEOUT_S * 1000)
		{
			printf("Connect timeout\n");
			dn_fsm_vars.state = FSM_STATE_DISCONNECTED;
			dn_fsm_vars.replyCb = NULL;
			fsm_cancelEvent();
			return FALSE;
		} else
		{
			dn_fsm_run();
			usleep(FSM_RUN_INTERVAL_MS * 1000);
		}
	}
	return TRUE;
}

void dn_fsm_run(void)
{
	uint32_t currentTime_ms = dn_time_ms();
	if (dn_fsm_vars.fsmDelay_ms > 0 && (currentTime_ms - dn_fsm_vars.fsmEventScheduled_ms > dn_fsm_vars.fsmDelay_ms))
	{
		dn_fsm_vars.fsmDelay_ms = 0;
		if (dn_fsm_vars.fsmCb != NULL)
		{
			dn_fsm_vars.fsmCb();
		}
	}
}

//=========================== private =========================================

void fsm_scheduleEvent(uint16_t delay_ms, fsm_timer_callback cb)
{
	dn_fsm_vars.fsmEventScheduled_ms = dn_time_ms();
	dn_fsm_vars.fsmDelay_ms = delay_ms;
	dn_fsm_vars.fsmCb = cb;
	printf("Event scheduled\n");
}

void fsm_cancelEvent(void)
{
	dn_fsm_vars.fsmDelay_ms = 0;
	dn_fsm_vars.fsmCb = NULL;
	printf("Event canceled\n");
}

void fsm_setReplyCallback(fsm_reply_callback cb)
{
	dn_fsm_vars.replyCb = cb;
}

void dn_ipmt_notif_cb(uint8_t cmdId, uint8_t subCmdId)
{
	//dn_ipmt_timeIndication_nt* notif_timeIndication;
	dn_ipmt_events_nt* notif_events;
	//dn_ipmt_receive_nt* notif_receive;
	//dn_ipmt_macRx_nt* notif_macRx;
	//dn_ipmt_txDone_nt* notif_txDone;
	//dn_ipmt_advReceived_nt* notif_advReceived;

	printf("Got notification: cmdId; %#x (%u), subCmdId; %#x (%u)\n",
			cmdId, cmdId, subCmdId, subCmdId);

	switch (cmdId)
	{
	case CMDID_TIMEINDICATION:
		break;
	case CMDID_EVENTS:
		notif_events = (dn_ipmt_events_nt*) dn_fsm_vars.notifBuf;
		printf("State: %#x\n", notif_events->state);
		switch (notif_events->state)
		{
		case MOTE_STATE_IDLE:
			fsm_scheduleEvent(CMD_PERIOD_MS, api_getMoteStatus);
			break;
		case MOTE_STATE_OPERATIONAL:
			dn_fsm_vars.state = FSM_STATE_READY;
			break;
		}
		dn_fsm_vars.events |= notif_events->events;

		break;
	case CMDID_RECEIVE:
		break;
	case CMDID_MACRX:
		break;
	case CMDID_TXDONE:
		break;
	case CMDID_ADVRECEIVED:
		break;
	default:
		// Unknown notification ID
		break;
	}


}

void dn_ipmt_reply_cb(uint8_t cmdId)
{
	printf("Got reply: cmdId; %#x (%u)\n", cmdId, cmdId);
	if (dn_fsm_vars.replyCb == NULL)
	{
		// No reply callback registered
		printf("dn_fsm: Reply callback empty\n");
		return;
	}
	dn_fsm_vars.replyCb();
}

void api_response_timeout(void)
{
	printf("Response timeout\n");
	dn_ipmt_cancelTx();
	dn_fsm_vars.replyCb = NULL;
	fsm_scheduleEvent(CMD_PERIOD_MS, api_getMoteStatus);
}

void api_getMoteStatus(void)
{
	printf("Mote status\n");
	// arm callback
	fsm_setReplyCallback(api_getMoteStatus_reply);

	// issue function
	dn_ipmt_getParameter_moteStatus(
			(dn_ipmt_getParameter_moteStatus_rpt*) (dn_fsm_vars.replyBuf)
			);

	// schedule timeout event
	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, api_response_timeout);
}

void api_getMoteStatus_reply(void)
{
	dn_ipmt_getParameter_moteStatus_rpt* reply;
	printf("Mote status reply\n");

	// cancel timeout
	fsm_cancelEvent();

	// parse reply
	reply = (dn_ipmt_getParameter_moteStatus_rpt*) dn_fsm_vars.replyBuf;
	printf("State: %#x\n", reply->state);

	// choose next step
	switch (reply->state)
	{
	case MOTE_STATE_IDLE:
		fsm_scheduleEvent(CMD_PERIOD_MS, api_openSocket);
		break;
	case MOTE_STATE_OPERATIONAL:
		dn_fsm_vars.state = FSM_STATE_READY;
		break;
	default:
		fsm_scheduleEvent(CMD_PERIOD_MS, api_getMoteStatus);
		break;
	}
}

void api_openSocket(void)
{
	printf("Open socket\n");
	// arm callback
	fsm_setReplyCallback(api_openSocket_reply);

	// issue function
	dn_ipmt_openSocket(
			0, // protocol
			(dn_ipmt_openSocket_rpt*) (dn_fsm_vars.replyBuf) // reply
			);

	// schedule timeout event
	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, api_response_timeout);
}

void api_openSocket_reply(void)
{
	dn_ipmt_openSocket_rpt* reply;
	printf("Open socket reply\n");

	// cancel timeout
	fsm_cancelEvent();

	// parse reply
	reply = (dn_ipmt_openSocket_rpt*) dn_fsm_vars.replyBuf;

	// store the socketID
	dn_fsm_vars.socketId = reply->socketId;

	// choose next step
	fsm_scheduleEvent(CMD_PERIOD_MS, api_bindSocket);
}

void api_bindSocket(void)
{
	printf("Bind socket\n");
	// arm callback
	fsm_setReplyCallback(api_bindSocket_reply);

	// issue function
	dn_ipmt_bindSocket(
			dn_fsm_vars.socketId, // socketId
			SRC_PORT, // port
			(dn_ipmt_bindSocket_rpt*) (dn_fsm_vars.replyBuf) // reply
			);

	// schedule timeout event
	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, api_response_timeout);
}

void api_bindSocket_reply(void)
{
	printf("Bind socket reply\n");
	// cancel timeout
	fsm_cancelEvent();

	// choose next step
	fsm_scheduleEvent(CMD_PERIOD_MS, api_join);
}

void api_join(void)
{
	printf("Join\n");
	// arm callback
	fsm_setReplyCallback(api_join_reply);

	// issue function
	dn_ipmt_join(
			(dn_ipmt_join_rpt*) (dn_fsm_vars.replyBuf) // reply
			);

	// schedule timeout event
	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, api_response_timeout);
}

void api_join_reply(void)
{
	printf("Join reply\n");
	// cancel timeout
	fsm_cancelEvent();

	// choose next step
	// no next step at this point. FSM will advance when we received a "joined"
	// notification
}

//=========================== helpers =========================================
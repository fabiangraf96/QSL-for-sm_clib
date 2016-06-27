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
#include "dn_debug.h"

//=========================== variables =======================================

typedef struct
{
	uint8_t replyBuf[MAX_FRAME_LENGTH];
	uint8_t notifBuf[MAX_FRAME_LENGTH];
	int8_t socketId;
	uint16_t networkId;
	uint8_t joinKey[JOIN_KEY_LEN];
	uint16_t service_ms;
	fsm_reply_callback replyCb;
	fsm_timer_callback fsmCb;
	uint32_t fsmEventScheduled_ms;
	uint16_t fsmDelay_ms;
	uint32_t events;
	uint8_t state;
	uint8_t payloadBuff[PAYLOAD_LIMIT];
	uint8_t payloadSize;
	uint8_t destIPv6[IPv6ADDR_LEN];
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
void api_sendTo(void);
void api_sendTo_reply(void);

//=========================== public ==========================================

bool dn_qsl_init(void)
{
	// Reset local variables
	memset(&dn_fsm_vars, 0, sizeof (dn_fsm_vars));
	dn_fsm_vars.socketId = -1;

	// Initialize the ipmt module
	dn_ipmt_init // Should be augmented with return value to know if successful...
			(
			dn_ipmt_notif_cb,
			dn_fsm_vars.notifBuf,
			sizeof (dn_fsm_vars.notifBuf),
			dn_ipmt_reply_cb
			);

	dn_fsm_vars.state = FSM_STATE_DISCONNECTED;
	return TRUE;
}

bool dn_qsl_connect(uint16_t netID, uint8_t* joinKey, uint32_t req_service_ms)
{
	uint32_t cmdStart_ms = dn_time_ms();
	switch (dn_fsm_vars.state)
	{
	case FSM_STATE_NOT_INITIALIZED:
		log_warn("dn_qsl_connect run without being initialized");
		return FALSE;
	case FSM_STATE_DISCONNECTED:
		// Check arguments vs default
		fsm_scheduleEvent(CMD_PERIOD_MS, api_getMoteStatus);
	case FSM_STATE_READY:
		// Check if args are different than current; act accordingly
		break;
	default:
		log_err("Undefined state");
		break;
	}

	while (dn_fsm_vars.state != FSM_STATE_READY && dn_fsm_vars.state != FSM_STATE_CONNECT_FAILED)
	{
		if ((dn_time_ms() - cmdStart_ms) > CONNECT_TIMEOUT_S * 1000)
		{
			debug("Connect timeout");
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
	return dn_fsm_vars.state == FSM_STATE_READY;
}

bool dn_qsl_send(uint8_t* payload, uint8_t payloadSize_B, uint8_t* destIP)
{
	uint32_t cmdStart_ms = dn_time_ms();
	switch (dn_fsm_vars.state)
	{
	case FSM_STATE_READY:
		if (payloadSize_B > PAYLOAD_LIMIT)
		{
			log_warn("Payload size (%u) exceeds limit (%u)", payloadSize_B, PAYLOAD_LIMIT);
			return FALSE;
		}
		memcpy(dn_fsm_vars.payloadBuff, payload, payloadSize_B);
		dn_fsm_vars.payloadSize = payloadSize_B;
		if (destIP == NULL)
		{
			memcpy(dn_fsm_vars.destIPv6, ipv6Addr_manager, IPv6ADDR_LEN);
		}
		else
		{
			memcpy(dn_fsm_vars.destIPv6, destIP, IPv6ADDR_LEN);
		}
		dn_fsm_vars.state = FSM_STATE_SENDING;
		fsm_scheduleEvent(CMD_PERIOD_MS, api_sendTo);
		break;
	default:
		return FALSE;
	}
	
	while (dn_fsm_vars.state == FSM_STATE_SENDING)
	{
		if ((dn_time_ms() - cmdStart_ms) > SEND_TIMEOUT_S * 1000)
		{
			debug("Send timeout");
			dn_fsm_vars.state = FSM_STATE_READY;
			dn_fsm_vars.replyCb = NULL;
			fsm_cancelEvent();
			return FALSE;
		} else
		{
			dn_fsm_run();
			usleep(FSM_RUN_INTERVAL_MS * 1000);
		}
	}
	return dn_fsm_vars.state == FSM_STATE_READY;
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
}

void fsm_cancelEvent(void)
{
	dn_fsm_vars.fsmDelay_ms = 0;
	dn_fsm_vars.fsmCb = NULL;
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

	debug("Got notification: cmdId; %#x (%u), subCmdId; %#x (%u)",
			cmdId, cmdId, subCmdId, subCmdId);

	switch (cmdId)
	{
	case CMDID_TIMEINDICATION:
		break;
	case CMDID_EVENTS:
		notif_events = (dn_ipmt_events_nt*) dn_fsm_vars.notifBuf;
		debug("State: %#x", notif_events->state);
		switch (notif_events->state)
		{
		case MOTE_STATE_IDLE:
			fsm_scheduleEvent(CMD_PERIOD_MS, api_getMoteStatus);
			break;
		case MOTE_STATE_OPERATIONAL:
			dn_fsm_vars.state = FSM_STATE_CONNECTED;
			fsm_scheduleEvent(CMD_PERIOD_MS, api_openSocket);
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
	debug("Got reply: cmdId; %#x (%u)", cmdId, cmdId);
	if (dn_fsm_vars.replyCb == NULL)
	{
		// No reply callback registered
		debug("Reply callback empty");
		return;
	}
	dn_fsm_vars.replyCb();
}

void api_response_timeout(void)
{
	debug("Response timeout");
	dn_ipmt_cancelTx();
	dn_fsm_vars.replyCb = NULL;
	
	switch (dn_fsm_vars.state)
	{
	case FSM_STATE_SENDING:
		dn_fsm_vars.state = FSM_STATE_SEND_FAILED;
		break;
	default:
		fsm_scheduleEvent(CMD_PERIOD_MS, api_getMoteStatus);
		break;
	}
}

void api_getMoteStatus(void)
{
	debug("Mote status");
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
	debug("Mote status reply");

	// cancel timeout
	fsm_cancelEvent();

	// parse reply
	reply = (dn_ipmt_getParameter_moteStatus_rpt*) dn_fsm_vars.replyBuf;
	debug("State: %#x", reply->state);

	// choose next step
	switch (reply->state)
	{
	case MOTE_STATE_IDLE:
		fsm_scheduleEvent(CMD_PERIOD_MS, api_join);
		break;
	case MOTE_STATE_OPERATIONAL:
		dn_fsm_vars.state = FSM_STATE_CONNECTED;
		fsm_scheduleEvent(CMD_PERIOD_MS, api_openSocket);
		break;
	default:
		fsm_scheduleEvent(CMD_PERIOD_MS, api_getMoteStatus);
		break;
	}
}

void api_openSocket(void)
{
	debug("Open socket");
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
	debug("Open socket reply");

	// cancel timeout
	fsm_cancelEvent();

	// parse reply
	reply = (dn_ipmt_openSocket_rpt*) dn_fsm_vars.replyBuf;
	switch (reply->RC)
	{
	case RC_OK:
		debug("Socket %d opened successfully", reply->socketId);
		dn_fsm_vars.socketId = reply->socketId;
		fsm_scheduleEvent(CMD_PERIOD_MS, api_bindSocket);
		break;
	case RC_NO_RESOURCES:
		debug("Couldn't create socket due to resource availability");
		dn_fsm_vars.state = FSM_STATE_CONNECT_FAILED;
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		dn_fsm_vars.state = FSM_STATE_CONNECT_FAILED;
		break;
	}
}

void api_bindSocket(void)
{
	debug("Bind socket");
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
	dn_ipmt_bindSocket_rpt* reply;
	debug("Bind socket reply");
	// cancel timeout
	fsm_cancelEvent();

	reply = (dn_ipmt_bindSocket_rpt*) dn_fsm_vars.replyBuf;
	switch (reply->RC)
	{
	case RC_OK:
		debug("Socket bound successfully");
		dn_fsm_vars.state = FSM_STATE_READY;
		break;
	case RC_BUSY:
		debug("Port already bound");
		dn_fsm_vars.state = FSM_STATE_CONNECT_FAILED;
		break;
	case RC_NOT_FOUND:
		debug("Invalid socket ID");
		dn_fsm_vars.state = FSM_STATE_CONNECT_FAILED;
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		dn_fsm_vars.state = FSM_STATE_CONNECT_FAILED;
		break;
	}
}

void api_join(void)
{
	debug("Join");
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
	debug("Join reply");
	// cancel timeout
	fsm_cancelEvent();

	// choose next step
	// no next step at this point. FSM will advance when we received a "joined"
	// notification
}

void api_sendTo(void)
{
	dn_err_t err;
	debug("Send");
	// arm callback
	fsm_setReplyCallback(api_sendTo_reply);
	
	// issue function
	err = dn_ipmt_sendTo
			(
			dn_fsm_vars.socketId, // socketId
			dn_fsm_vars.destIPv6, // destIP
			DST_PORT, // destPort
			0, // serviceType
			1, // priority
			0xffff, // packetId
			dn_fsm_vars.payloadBuff, // payload
			dn_fsm_vars.payloadSize, // payloadLen
			(dn_ipmt_sendTo_rpt*) (dn_fsm_vars.replyBuf) // reply
			);
	if (err != DN_ERR_NONE)
	{
		debug("Send error: %u", err);
	}

	// schedule timeout event
	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, api_response_timeout);
}

void api_sendTo_reply(void)
{
	dn_ipmt_sendTo_rpt* reply;
	debug("Send reply");
	
   // cancel timeout
   fsm_cancelEvent();
   
   reply = (dn_ipmt_sendTo_rpt*)dn_fsm_vars.replyBuf;
   
   switch (reply->RC)
   {
   case RC_OK:
	   dn_fsm_vars.state = FSM_STATE_READY;
	   break;
   case RC_NO_RESOURCES:
	   debug("Send failed: NO RESOURCES");
	   dn_fsm_vars.state = FSM_STATE_SEND_FAILED;
	   break;
   default:
	   log_warn("Unexpected response code: %#x", reply->RC);
	   dn_fsm_vars.state = FSM_STATE_SEND_FAILED;
	   break;
   }   
}

//=========================== helpers =========================================
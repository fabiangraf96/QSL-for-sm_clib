/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "dn_fsm.h"
#include "dn_ipmt.h"
#include "dn_time.h"
#include "dn_watchdog.h"
#include "dn_qsl_api.h"
#include "dn_debug.h"

//=========================== variables =======================================

typedef struct
{
	// api
	fsm_reply_callback replyCb;
	fsm_timer_callback fsmCb;
	uint8_t replyBuf[MAX_FRAME_LENGTH];
	uint8_t notifBuf[MAX_FRAME_LENGTH];
	// connect
	uint8_t socketId;
	uint16_t networkId;
	uint8_t joinKey[JOIN_KEY_LEN];
	uint32_t service_ms;
	// fsm
	uint32_t fsmEventScheduled_ms;
	uint16_t fsmDelay_ms;
	uint32_t events;
	uint8_t state;
	// send
	uint8_t payloadBuff[PAYLOAD_LIMIT];
	uint8_t payloadSize;
	uint8_t destIPv6[IPv6ADDR_LEN];
	uint16_t destPort;
	// read
	uint8_t inboxBuf[INBOX_SIZE][PAYLOAD_LIMIT];
	uint8_t inboxSize[INBOX_SIZE];
	uint8_t inboxLength;
	uint8_t inboxHead;
	uint8_t inboxTail;
} dn_fsm_vars_t;

dn_fsm_vars_t dn_fsm_vars;


//=========================== prototypes ======================================

void dn_ipmt_notif_cb(uint8_t cmdId, uint8_t subCmdId);
void dn_ipmt_reply_cb(uint8_t cmdId);
void fsm_run(void);
void fsm_scheduleEvent(uint16_t delay, fsm_timer_callback cb);
void fsm_cancelEvent(void);
void fsm_setReplyCallback(fsm_reply_callback cb);

void api_response_timeout(void);
void api_reset(void);
void api_reset_reply(void);
void api_disconnect(void);
void api_disconnect_reply(void);
void api_getMoteStatus(void);
void api_getMoteStatus_reply(void);
void api_openSocket(void);
void api_openSocket_reply(void);
void api_bindSocket(void);
void api_bindSocket_reply(void);
void api_setJoinKey(void);
void api_setJoinKey_reply(void);
void api_setNetworkId(void);
void api_setNetworkId_reply(void);
void api_join(void);
void api_join_reply(void);
void api_requestService(void);
void api_requestService_reply(void);
void api_getServiceInfo(void);
void api_getServiceInfo_reply(void);
void api_sendTo(void);
void api_sendTo_reply(void);

static dn_err_t checkAndSaveNetConfig(uint16_t netID, uint8_t* joinKey, uint32_t req_service_ms);
static void fsm_enterState(uint8_t newState, uint16_t spesificDelay);

//=========================== public ==========================================

bool dn_qsl_init(void)
{
	// Reset local variables
	memset(&dn_fsm_vars, 0, sizeof (dn_fsm_vars));

	// Initialize the ipmt module
	dn_ipmt_init // Should be augmented with return value to know if successful...
			(
			dn_ipmt_notif_cb,
			dn_fsm_vars.notifBuf,
			sizeof (dn_fsm_vars.notifBuf),
			dn_ipmt_reply_cb
			);

	fsm_enterState(FSM_STATE_DISCONNECTED, 0);
	return TRUE;
}

bool dn_qsl_isConnected(void)
{
	return dn_fsm_vars.state == FSM_STATE_CONNECTED;
}

bool dn_qsl_connect(uint16_t netID, uint8_t* joinKey, uint32_t req_service_ms)
{
	uint32_t cmdStart_ms = dn_time_ms();
	dn_err_t err;
	switch (dn_fsm_vars.state)
	{
	case FSM_STATE_NOT_INITIALIZED:
		log_warn("dn_qsl_connect run without being initialized");
		return FALSE;
	case FSM_STATE_DISCONNECTED:
		debug("dn_qsl_connect while DISCONNECTED");
		err = checkAndSaveNetConfig(netID, joinKey, req_service_ms);
		if (err != DN_ERR_NONE)
		{
			return FALSE;
		}
		fsm_enterState(FSM_STATE_PRE_JOIN, 0);
		break;
	case FSM_STATE_CONNECTED:
		debug("dn_qsl_connect while CONNECTED");
		if ((netID > 0 && netID != dn_fsm_vars.networkId) ||
				(joinKey != NULL && memcmp(joinKey, dn_fsm_vars.joinKey, JOIN_KEY_LEN) != 0))
		{
			err = checkAndSaveNetConfig(netID, joinKey, req_service_ms);
			if (err != DN_ERR_NONE)
			{
				return FALSE;
			}
			debug("New network ID and/or join key; reconnecting...");
			fsm_enterState(FSM_STATE_RESETTING, 0);
		} else if (req_service_ms > 0 && req_service_ms != dn_fsm_vars.service_ms)
		{
			debug("New service request");
			dn_fsm_vars.service_ms = req_service_ms;
			fsm_enterState(FSM_STATE_REQ_SERVICE, 0);
		} else
		{
			debug("Already connected");
		}
		break;
	default:
		log_err("Undefined state");
		dn_fsm_vars.state = FSM_STATE_DISCONNECTED;
		return FALSE;
	}

	while (dn_fsm_vars.state != FSM_STATE_CONNECTED && dn_fsm_vars.state != FSM_STATE_DISCONNECTED)
	{
		if ((dn_time_ms() - cmdStart_ms) > CONNECT_TIMEOUT_S * 1000)
		{
			debug("Connect timeout");
			fsm_enterState(FSM_STATE_DISCONNECTED, 0);
			dn_fsm_vars.replyCb = NULL;
			fsm_cancelEvent();
			return FALSE;
		} else
		{
			fsm_run();
			dn_sleep_ms(FSM_RUN_INTERVAL_MS);
		}
	}
	return dn_fsm_vars.state == FSM_STATE_CONNECTED;
}

bool dn_qsl_send(uint8_t* payload, uint8_t payloadSize_B, uint16_t destPort)
{
	uint32_t cmdStart_ms = dn_time_ms();
	switch (dn_fsm_vars.state)
	{
	case FSM_STATE_CONNECTED:
		if (payloadSize_B > PAYLOAD_LIMIT)
		{
			log_warn("Payload size (%u) exceeds limit (%u)", payloadSize_B, PAYLOAD_LIMIT);
			return FALSE;
		}
		memcpy(dn_fsm_vars.payloadBuff, payload, payloadSize_B);
		dn_fsm_vars.payloadSize = payloadSize_B;
		memcpy(dn_fsm_vars.destIPv6, DEST_IP, IPv6ADDR_LEN);
		if (destPort == 0)
		{
			dn_fsm_vars.destPort = DEFAULT_DEST_PORT;
		} else
		{
			dn_fsm_vars.destPort = destPort;
		}
		fsm_enterState(FSM_STATE_SENDING, 0);
		break;
	default:
		return FALSE;
	}

	while (dn_fsm_vars.state == FSM_STATE_SENDING)
	{
		if ((dn_time_ms() - cmdStart_ms) > SEND_TIMEOUT_S * 1000)
		{
			debug("Send timeout");
			fsm_enterState(FSM_STATE_CONNECTED, 0);
			dn_fsm_vars.replyCb = NULL;
			fsm_cancelEvent();
			return FALSE;
		} else
		{
			fsm_run();
			dn_sleep_ms(FSM_RUN_INTERVAL_MS);
		}
	}
	if (dn_fsm_vars.state == FSM_STATE_SEND_FAILED)
	{
		fsm_enterState(FSM_STATE_CONNECTED, 0);
		return FALSE;
	}
	return dn_fsm_vars.state == FSM_STATE_CONNECTED;
}

uint8_t dn_qsl_read(uint8_t* readBuffer)
{
	uint8_t bytesRead = 0;
	if (dn_fsm_vars.inboxLength > 0)
	{
		memcpy
				(
				readBuffer,
				dn_fsm_vars.inboxBuf[dn_fsm_vars.inboxHead],
				dn_fsm_vars.inboxSize[dn_fsm_vars.inboxHead]
				);
		bytesRead = dn_fsm_vars.inboxSize[dn_fsm_vars.inboxHead];
		dn_fsm_vars.inboxHead = (dn_fsm_vars.inboxHead + 1) % INBOX_SIZE;
		dn_fsm_vars.inboxLength--;
		debug("Read %u bytes from inbox", bytesRead);
	} else
	{
		debug("Inbox empty");
	}
	return bytesRead;
}

//=========================== private =========================================

//=== FSM ===

void fsm_run(void)
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

void fsm_scheduleEvent(uint16_t delay_ms, fsm_timer_callback cb)
{
	dn_fsm_vars.fsmEventScheduled_ms = dn_time_ms(); // TODO: Move to each cmd
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

//=== C Library API ===

void dn_ipmt_notif_cb(uint8_t cmdId, uint8_t subCmdId)
{
	//dn_ipmt_timeIndication_nt* notif_timeIndication;
	dn_ipmt_events_nt* notif_events;
	dn_ipmt_receive_nt* notif_receive;
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
		debug("State: %#x | Events: %#x", notif_events->state, notif_events->events);
		
		switch (dn_fsm_vars.state)
		{
		case FSM_STATE_JOINING:
			if (notif_events->events & MOTE_EVENT_MASK_OPERATIONAL)
			{
				if (dn_fsm_vars.service_ms > 0)
				{
					fsm_enterState(FSM_STATE_REQ_SERVICE, 0);
				} else
				{
					fsm_enterState(FSM_STATE_CONNECTED, 0);
				}
				return;
			}
		case FSM_STATE_REQ_SERVICE:
			if (notif_events->events & MOTE_EVENT_MASK_SVC_CHANGE)
			{
				fsm_scheduleEvent(CMD_PERIOD_MS, api_getServiceInfo);
				return;
			}
		}
		
		switch (notif_events->state)
		{
		case MOTE_STATE_IDLE:
			switch (dn_fsm_vars.state)
			{
			case FSM_STATE_PRE_JOIN:
			case FSM_STATE_JOINING:
			case FSM_STATE_REQ_SERVICE:
			case FSM_STATE_RESETTING:
				fsm_enterState(FSM_STATE_PRE_JOIN, 0);
				break;
			case FSM_STATE_CONNECTED:
			case FSM_STATE_SENDING:
				fsm_enterState(FSM_STATE_DISCONNECTED, 0);
				break;
			}
			break;
		case MOTE_STATE_OPERATIONAL:
			switch (dn_fsm_vars.state)
			{
			case FSM_STATE_PRE_JOIN:
				fsm_enterState(FSM_STATE_RESETTING, 0);
				break;						
			}
			break;
		}

		//dn_fsm_vars.events |= notif_events->events; // If events to be detected elsewhere

		break;
	case CMDID_RECEIVE:
		notif_receive = (dn_ipmt_receive_nt*)dn_fsm_vars.notifBuf;
		debug("Received downstream data");
		
		if (dn_fsm_vars.inboxLength < INBOX_SIZE)
		{
			memcpy
					(
					dn_fsm_vars.inboxBuf[dn_fsm_vars.inboxTail],
					notif_receive->payload,
					notif_receive->payloadLen
					);
			dn_fsm_vars.inboxSize[dn_fsm_vars.inboxTail] = notif_receive->payloadLen;
			dn_fsm_vars.inboxTail = (dn_fsm_vars.inboxTail + 1) % INBOX_SIZE;
			dn_fsm_vars.inboxLength++;
		} else
		{
			log_warn("Inbox full");
		}
		
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
		fsm_enterState(FSM_STATE_SEND_FAILED, 0);
		break;
	default:
		fsm_enterState(FSM_STATE_PRE_JOIN, 0);
		break;
	}
}

void api_reset(void)
{
	debug("Reset");

	fsm_setReplyCallback(api_reset_reply);

	dn_ipmt_reset
			(
			(dn_ipmt_reset_rpt*) dn_fsm_vars.replyBuf
			);

	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, api_response_timeout);
}

void api_reset_reply(void)
{
	dn_ipmt_reset_rpt* reply;
	debug("Reset reply");

	fsm_cancelEvent();

	reply = (dn_ipmt_reset_rpt*) dn_fsm_vars.replyBuf;
	switch (reply->RC)
	{
	case RC_OK:
		debug("Mote soft-reset initiated");
		//fsm_enterState(FSM_STATE_PRE_JOIN, BACKOFF_AFTER_RESET_MS);
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		fsm_enterState(FSM_STATE_DISCONNECTED, 0);
		break;
	}
}

void api_disconnect(void)
{
	debug("Disconnect");
	
	fsm_setReplyCallback(api_disconnect_reply);
	
	dn_ipmt_disconnect
			(
			(dn_ipmt_disconnect_rpt*)dn_fsm_vars.replyBuf
			);
	
	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, api_response_timeout);
}

void api_disconnect_reply(void)
{
	dn_ipmt_disconnect_rpt* reply;
	debug("Disconnect reply");
	
	fsm_cancelEvent();
	
	reply = (dn_ipmt_disconnect_rpt*)dn_fsm_vars.replyBuf;
	switch (reply->RC)
	{
	case RC_OK:
		debug("Mote disconnect initiated");
		//fsm_enterState(FSM_STATE_PRE_JOIN, BACKOFF_AFTER_DISCONNECT_MS);
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		fsm_scheduleEvent(CMD_PERIOD_MS, api_reset);
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
		fsm_scheduleEvent(CMD_PERIOD_MS, api_openSocket);
		break;
	case MOTE_STATE_OPERATIONAL:
		fsm_enterState(FSM_STATE_RESETTING, 0);
		break;
	default:
		fsm_enterState(FSM_STATE_RESETTING, 0);
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
		// Own state for disconnecting?
		fsm_enterState(FSM_STATE_RESETTING, 0);
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		fsm_enterState(FSM_STATE_DISCONNECTED, 0);
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
			INBOX_PORT, // port
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
		fsm_scheduleEvent(CMD_PERIOD_MS, api_setJoinKey);
		break;
	case RC_BUSY:
		debug("Port already bound");
		// Own state for disconnect?
		fsm_enterState(FSM_STATE_RESETTING, 0);
		break;
	case RC_NOT_FOUND:
		debug("Invalid socket ID");
		fsm_enterState(FSM_STATE_DISCONNECTED, 0);
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		fsm_enterState(FSM_STATE_DISCONNECTED, 0);
		break;
	}
}

void api_setJoinKey(void)
{
	debug("Set join key");

	fsm_setReplyCallback(api_setJoinKey_reply);

	dn_ipmt_setParameter_joinKey
			(
			dn_fsm_vars.joinKey,
			(dn_ipmt_setParameter_joinKey_rpt*) dn_fsm_vars.replyBuf
			);

	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, api_response_timeout);
}

void api_setJoinKey_reply(void)
{
	dn_ipmt_setParameter_joinKey_rpt* reply;
	debug("Set join key reply");

	fsm_cancelEvent();

	reply = (dn_ipmt_setParameter_joinKey_rpt*) dn_fsm_vars.replyBuf;
	switch (reply->RC)
	{
	case RC_OK:
		debug("Join key set");
		fsm_scheduleEvent(CMD_PERIOD_MS, api_setNetworkId);
		break;
	case RC_WRITE_FAIL:
		debug("Could not write the key to storage");
		fsm_enterState(FSM_STATE_DISCONNECTED, 0);
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		fsm_enterState(FSM_STATE_DISCONNECTED, 0);
		break;
	}
}

void api_setNetworkId(void)
{
	debug("Set network ID");

	fsm_setReplyCallback(api_setNetworkId_reply);

	dn_ipmt_setParameter_networkId
			(
			dn_fsm_vars.networkId,
			(dn_ipmt_setParameter_networkId_rpt*) dn_fsm_vars.replyBuf
			);

	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, api_response_timeout);
}

void api_setNetworkId_reply(void)
{
	dn_ipmt_setParameter_networkId_rpt* reply;
	debug("Set network ID reply");

	fsm_cancelEvent();

	reply = (dn_ipmt_setParameter_networkId_rpt*) dn_fsm_vars.replyBuf;
	switch (reply->RC)
	{
	case RC_OK:
		debug("Network ID set");
		fsm_enterState(FSM_STATE_JOINING, 0);
		break;
	case RC_WRITE_FAIL:
		debug("Could not write the network ID to storage");
		fsm_enterState(FSM_STATE_DISCONNECTED, 0);
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		fsm_enterState(FSM_STATE_DISCONNECTED, 0);
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
	dn_ipmt_join_rpt* reply;
	debug("Join reply");
	// cancel timeout
	fsm_cancelEvent();

	reply = (dn_ipmt_join_rpt*) dn_fsm_vars.replyBuf;
	switch (reply->RC)
	{
	case RC_OK:
		debug("Join operation started");
		// Will wait for operational event
		break;
	case RC_INVALID_STATE:
		debug("The mote is in an invalid state to start join operation");
		fsm_enterState(FSM_STATE_RESETTING, 0);
		break;
	case RC_INCOMPLETE_JOIN_INFO:
		debug("Incomplete configuration to start joining");
		fsm_enterState(FSM_STATE_RESETTING, 0);
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		fsm_enterState(FSM_STATE_DISCONNECTED, 0);
		break;
	}
}

void api_requestService(void)
{
	debug("Request service");

	fsm_setReplyCallback(api_requestService_reply);

	dn_ipmt_requestService
			(
			SERVICE_ADDRESS,
			SERVICE_TYPE_BW,
			dn_fsm_vars.service_ms,
			(dn_ipmt_requestService_rpt*) dn_fsm_vars.replyBuf
			);

	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, api_response_timeout);
}

void api_requestService_reply(void)
{
	dn_ipmt_requestService_rpt* reply;
	debug("Request service reply");

	fsm_cancelEvent();

	reply = (dn_ipmt_requestService_rpt*) dn_fsm_vars.replyBuf;
	switch (reply->RC)
	{
	case RC_OK:
		debug("Service request accepted");
		// Will wait for svcChanged notification
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		fsm_enterState(FSM_STATE_DISCONNECTED, 0);
		break;
	}
}

void api_getServiceInfo(void)
{
	debug("Get service info");

	fsm_setReplyCallback(api_getServiceInfo_reply);

	dn_ipmt_getServiceInfo
			(
			SERVICE_ADDRESS,
			SERVICE_TYPE_BW,
			(dn_ipmt_getServiceInfo_rpt*) dn_fsm_vars.replyBuf
			);

	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, api_response_timeout);
}

void api_getServiceInfo_reply(void)
{
	dn_ipmt_getServiceInfo_rpt* reply;
	debug("Get service info reply");

	fsm_cancelEvent();

	reply = (dn_ipmt_getServiceInfo_rpt*) dn_fsm_vars.replyBuf;
	switch (reply->RC)
	{
	case RC_OK:
		if (reply->state == SERVICE_STATE_COMPLETED)
		{
			if (reply->value <= dn_fsm_vars.service_ms)
			{
				log_info("Granted service of %u ms (requested %u ms)", reply->value, dn_fsm_vars.service_ms);
			} else
			{
				log_warn("Only granted service of %u ms (requested %u ms)", reply->value, dn_fsm_vars.service_ms);
				// Should maybe fail?
			}
			fsm_enterState(FSM_STATE_CONNECTED, 0);
		} else
		{
			debug("Service request still pending");
			fsm_scheduleEvent(CMD_PERIOD_MS, api_getServiceInfo);
		}
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		fsm_enterState(FSM_STATE_DISCONNECTED, 0);
		break;
	}
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
			dn_fsm_vars.destPort, // destPort
			SERVICE_TYPE_BW, // serviceType
			PACKET_PRIORITY_MEDIUM, // priority
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

	reply = (dn_ipmt_sendTo_rpt*) dn_fsm_vars.replyBuf;
	switch (reply->RC)
	{
	case RC_OK:
		debug("Packet was queued up for transmission");
		dn_fsm_vars.state = FSM_STATE_CONNECTED;
		break;
	case RC_NO_RESOURCES:
		debug("No queue space to accept the packet");
		fsm_enterState(FSM_STATE_SEND_FAILED, 0);
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		fsm_enterState(FSM_STATE_SEND_FAILED, 0);
		break;
	}
}

//=========================== helpers =========================================

static dn_err_t checkAndSaveNetConfig(uint16_t netID, uint8_t* joinKey, uint32_t req_service_ms)
{
	if (netID == 0)
	{
		debug("No network ID given; using default");
		dn_fsm_vars.networkId = DEFAULT_NET_ID;
	} else if (netID == 0xFFFF)
	{
		log_err("Invalid network ID: %#x (%u)", netID, netID);
		return DN_ERR_MALFORMED;
	} else
	{
		dn_fsm_vars.networkId = netID;
	}

	if (joinKey == NULL)
	{
		debug("No join key given; using default");
		memcpy(dn_fsm_vars.joinKey, DEFAULT_JOIN_KEY, JOIN_KEY_LEN);
	} else
	{
		memcpy(dn_fsm_vars.joinKey, joinKey, JOIN_KEY_LEN);
	}

	dn_fsm_vars.service_ms = req_service_ms;
	
	return DN_ERR_NONE;
}

static void fsm_enterState(uint8_t newState, uint16_t spesificDelay)
{
	uint16_t delay = CMD_PERIOD_MS;
	if (spesificDelay > 0)
	{
		delay = spesificDelay;
	}
	switch(newState)
	{
	case FSM_STATE_PRE_JOIN:
		fsm_scheduleEvent(delay, api_getMoteStatus);
		break;
	case FSM_STATE_JOINING:
		fsm_scheduleEvent(CMD_PERIOD_MS, api_join);
		break;
	case FSM_STATE_REQ_SERVICE:
		fsm_scheduleEvent(delay, api_requestService);
		break;
	case FSM_STATE_RESETTING:
		fsm_scheduleEvent(delay, api_reset); // Faster
		//fsm_scheduleEvent(delay, api_disconnect); // More graceful
		break;
	case FSM_STATE_SENDING:
		api_sendTo();
		break;
	case FSM_STATE_SEND_FAILED:
	case FSM_STATE_DISCONNECTED:
	case FSM_STATE_CONNECTED:
		break;
	default:
		log_warn("Attempt at entering unexpected state %u", newState);
		return;
	}
	debug("FSM state transition: %#x --> %#x", dn_fsm_vars.state, newState);
	dn_fsm_vars.state = newState;
}
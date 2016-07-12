/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   dn_fsm.c
 * Author: jhbr@datarespons.no
 *
 * Created on 21. juni 2016, 13:09
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
	// FSM
	uint32_t fsmEventScheduled_ms;
	uint16_t fsmDelay_ms;
	uint8_t state;
	// C Library API
	fsm_reply_callback replyCb;
	fsm_timer_callback fsmCb;
	uint8_t replyBuf[MAX_FRAME_LENGTH];
	uint8_t notifBuf[MAX_FRAME_LENGTH];
	// Connection
	uint8_t socketId;
	uint16_t networkId;
	uint8_t joinKey[JOIN_KEY_LEN];
	uint32_t service_ms;
	uint8_t payloadBuf[DEFAULT_PAYLOAD_SIZE_LIMIT];
	uint8_t payloadSize;
	uint8_t destIPv6[IPv6ADDR_LEN];
	uint16_t destPort;
	dn_inbox_t inbox;
} dn_fsm_vars_t;

static dn_fsm_vars_t dn_fsm_vars;


//=========================== prototypes ======================================
// FSM
static void fsm_run(void);
static void fsm_scheduleEvent(uint16_t delay, fsm_timer_callback cb);
static void fsm_cancelEvent(void);
static void fsm_setReplyCallback(fsm_reply_callback cb);
static void fsm_enterState(uint8_t newState, uint16_t spesificDelay);
static bool fsm_cmd_timeout(uint32_t cmdStart_ms, uint32_t cmdTimeout_ms);
// C Library API
static void dn_ipmt_notif_cb(uint8_t cmdId, uint8_t subCmdId);
static void dn_ipmt_reply_cb(uint8_t cmdId);
static void event_response_timeout(void); // TODO: Better prefix?
static void event_reset(void);
static void reply_reset(void);
static void event_disconnect(void);
static void reply_disconnect(void);
static void event_getMoteStatus(void);
static void reply_getMoteStatus(void);
static void event_openSocket(void);
static void reply_openSocket(void);
static void event_bindSocket(void);
static void reply_bindSocket(void);
static void event_setJoinKey(void);
static void reply_setJoinKey(void);
static void event_setNetworkId(void);
static void reply_setNetworkId(void);
static void event_search(void);
static void reply_search(void);
static void event_join(void);
static void reply_join(void);
static void event_requestService(void);
static void reply_requestService(void);
static void event_getServiceInfo(void);
static void reply_getServiceInfo(void);
static void event_sendTo(void);
static void reply_sendTo(void);
// helpers
static dn_err_t checkAndSaveNetConfig(uint16_t netID, uint8_t* joinKey, uint32_t req_service_ms);
static uint8_t getPayloadLimit(uint16_t destPort);

//=========================== public ==========================================

//========== QSL API

bool dn_qsl_init(void)
{
	debug("QSL: Init");
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
	debug("QSL: isConnected");
	return dn_fsm_vars.state == FSM_STATE_CONNECTED;
}

bool dn_qsl_connect(uint16_t netID, uint8_t* joinKey, uint32_t req_service_ms)
{
	uint32_t cmdStart_ms = dn_time_ms();
	dn_err_t err;
	debug("QSL: Connect");
	switch (dn_fsm_vars.state)
	{
	case FSM_STATE_NOT_INITIALIZED:
		log_warn("Can't connect; not initialized");
		// TODO: Could initialize for user?
		return FALSE;
	case FSM_STATE_DISCONNECTED:
		err = checkAndSaveNetConfig(netID, joinKey, req_service_ms);
		if (err != DN_ERR_NONE)
		{
			return FALSE;
		}
		debug("Starting connect process...");
		fsm_enterState(FSM_STATE_PRE_JOIN, 0);
		break;
	case FSM_STATE_CONNECTED:
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
			// Nothing to do
		}
		break;
	default:
		log_err("Unexpected state");
		fsm_enterState(FSM_STATE_DISCONNECTED, 0);
		return FALSE;
	}

	// Drive FSM until connect success/failure or timeout
	while (dn_fsm_vars.state != FSM_STATE_CONNECTED
			&& dn_fsm_vars.state != FSM_STATE_DISCONNECTED
			&& !fsm_cmd_timeout(cmdStart_ms, CONNECT_TIMEOUT_S * 1000))
	{
		dn_watchdog_feed();
		fsm_run();
	}

	return dn_fsm_vars.state == FSM_STATE_CONNECTED;
}

bool dn_qsl_send(uint8_t* payload, uint8_t payloadSize_B, uint16_t destPort)
{
	uint32_t cmdStart_ms = dn_time_ms();
	uint8_t maxPayloadSize;
	debug("QSL: Send");
	switch (dn_fsm_vars.state)
	{
	case FSM_STATE_CONNECTED:
		maxPayloadSize = getPayloadLimit(destPort);

		if (payloadSize_B > maxPayloadSize)
		{
			log_warn("Payload size (%u) exceeds limit (%u)", payloadSize_B, maxPayloadSize);
			return FALSE;
		}
		// Store outbound payload and parameters
		memcpy(dn_fsm_vars.payloadBuf, payload, payloadSize_B);
		dn_fsm_vars.payloadSize = payloadSize_B;
		memcpy(dn_fsm_vars.destIPv6, DEST_IP, IPv6ADDR_LEN);
		dn_fsm_vars.destPort = (destPort > 0) ? destPort : DEFAULT_DEST_PORT;
		// Start send process
		fsm_enterState(FSM_STATE_SENDING, 0);
		break;
	default:
		log_warn("Can't send; not connected");
		return FALSE;
	}

	// Drive FSM until send success/failure or timeout
	while (dn_fsm_vars.state == FSM_STATE_SENDING
			&& !fsm_cmd_timeout(cmdStart_ms, SEND_TIMEOUT_MS))
	{
		dn_watchdog_feed();
		fsm_run();
	}

	// Catch send failure
	if (dn_fsm_vars.state == FSM_STATE_SEND_FAILED)
	{
		debug("Send failed");
		fsm_enterState(FSM_STATE_CONNECTED, 0);
		return FALSE;
	}

	return dn_fsm_vars.state == FSM_STATE_CONNECTED;
}

uint8_t dn_qsl_read(uint8_t* readBuffer)
{
	uint8_t bytesRead = 0;
	debug("QSL: Read");
	if (dn_fsm_vars.inbox.unreadPackets > 0)
	{
		// Pop payload at head of inbox
		memcpy
				(
				readBuffer,
				dn_fsm_vars.inbox.pktBuf[dn_fsm_vars.inbox.head],
				dn_fsm_vars.inbox.pktSize[dn_fsm_vars.inbox.head]
				);
		bytesRead = dn_fsm_vars.inbox.pktSize[dn_fsm_vars.inbox.head];
		dn_fsm_vars.inbox.head = (dn_fsm_vars.inbox.head + 1) % INBOX_SIZE;
		dn_fsm_vars.inbox.unreadPackets--;
		debug("Read %u bytes from inbox", bytesRead);
	} else
	{
		debug("Inbox empty");
	}
	return bytesRead;
}

//=========================== private =========================================

//========== FSM

//===== run

/**
 Check if an event is scheduled and run it if due.
 */
static void fsm_run(void)
{
	if (dn_fsm_vars.fsmDelay_ms > 0 && (dn_time_ms() - dn_fsm_vars.fsmEventScheduled_ms > dn_fsm_vars.fsmDelay_ms))
	{
		// Scheduled event is due; execute it
		dn_fsm_vars.fsmDelay_ms = 0;
		if (dn_fsm_vars.fsmCb != NULL)
		{
			dn_fsm_vars.fsmCb();
		}
	} else
	{
		// Sleep to save CPU power
		dn_sleep_ms(FSM_RUN_INTERVAL_MS);
	}
}

//===== scheduleEvent

/**
 Schedule function to be called after a given delay.
 */
static void fsm_scheduleEvent(uint16_t delay_ms, fsm_timer_callback cb)
{
	dn_fsm_vars.fsmEventScheduled_ms = dn_time_ms(); // TODO: Move to each cmd?
	dn_fsm_vars.fsmDelay_ms = delay_ms;
	dn_fsm_vars.fsmCb = cb;
}

//===== cancelEvent

/**
 Cancel currently scheduled event.
 */
static void fsm_cancelEvent(void)
{
	dn_fsm_vars.fsmDelay_ms = 0;
	dn_fsm_vars.fsmCb = NULL;
}

//===== setReplyCallback

/**
 Set the callback function that the C Library will execute when the next reply
 is received and the reply buffer is ready to be parsed.
 */
static void fsm_setReplyCallback(fsm_reply_callback cb)
{
	dn_fsm_vars.replyCb = cb;
}

//===== setReplyCallback

/**
 Transition FSM to new state and schedule any default entry events.
 */
static void fsm_enterState(uint8_t newState, uint16_t spesificDelay)
{
	uint32_t now = dn_time_ms();
	uint16_t delay = CMD_PERIOD_MS;
	static uint32_t lastTransition = 0;
	if (lastTransition == 0)
		lastTransition = dn_time_ms();
	
	// Use default delay if none given
	if (spesificDelay > 0)
		delay = spesificDelay;

	// Schedule default events for transition into states
	switch (newState)
	{
	case FSM_STATE_PRE_JOIN:
		fsm_scheduleEvent(delay, event_getMoteStatus);
		break;
	case FSM_STATE_PROMISCUOUS:
		fsm_scheduleEvent(delay, event_search);
		break;
	case FSM_STATE_JOINING:
		fsm_scheduleEvent(delay, event_join);
		break;
	case FSM_STATE_REQ_SERVICE:
		fsm_scheduleEvent(delay, event_requestService);
		break;
	case FSM_STATE_RESETTING:
		if (MOTE_DISCONNECT_BEFORE_RESET)
			fsm_scheduleEvent(delay, event_disconnect); // More graceful
		else
			fsm_scheduleEvent(delay, event_reset); // Faster
		break;
	case FSM_STATE_SENDING:
		event_sendTo();
		break;
	case FSM_STATE_SEND_FAILED:
	case FSM_STATE_DISCONNECTED:
	case FSM_STATE_CONNECTED:
		// These states have no default entry events
		break;
	default:
		log_warn("Attempt at entering unexpected state %#.2x", newState);
		return;
	}

	debug("FSM state transition: %#.2x --> %#.2x (%u ms)",
			dn_fsm_vars.state, newState, now - lastTransition);
	lastTransition = now;
	dn_fsm_vars.state = newState;
}

//===== cmdTimeout

/**
 Correctly abort the current API command if time passed since the given start
 has exceeded the given timeout.
 */
static bool fsm_cmd_timeout(uint32_t cmdStart_ms, uint32_t cmdTimeout_ms)
{
	bool timeout = (dn_time_ms() - cmdStart_ms) > cmdTimeout_ms;
	if (timeout)
	{
		// Cancel any ongoing transmission or scheduled event and reset reply cb
		dn_ipmt_cancelTx();
		dn_fsm_vars.replyCb = NULL;
		fsm_cancelEvent();

		// Default timeout state is different while connecting vs sending
		switch (dn_fsm_vars.state)
		{
		case FSM_STATE_PRE_JOIN:
		case FSM_STATE_JOINING:
		case FSM_STATE_REQ_SERVICE:
		case FSM_STATE_RESETTING:
			debug("Connect timeout");
			fsm_enterState(FSM_STATE_DISCONNECTED, 0);
			break;
		case FSM_STATE_SENDING:
			debug("Send timeout");
			fsm_enterState(FSM_STATE_SEND_FAILED, 0);
			break;
		default:
			log_err("Command timeout in unexpected state: %#x", dn_fsm_vars.state);
			break;
		}
	}
	return timeout;
}

//========== C Library API

//===== notif_cb

/**
 This function is called whenever a notification is received through the
 SmartMesh C Library. The notification variables are than available through
 the notification buffer that can be cast to the correct type based on the
 given command ID (notification type).
 */
static void dn_ipmt_notif_cb(uint8_t cmdId, uint8_t subCmdId)
{
	//dn_ipmt_timeIndication_nt* notif_timeIndication;
	dn_ipmt_events_nt* notif_events;
	dn_ipmt_receive_nt* notif_receive;
	//dn_ipmt_macRx_nt* notif_macRx;
	//dn_ipmt_txDone_nt* notif_txDone;
	dn_ipmt_advReceived_nt* notif_advReceived;

	debug("Got notification: cmdId; %#.2x (%u), subCmdId; %#.2x (%u)",
			cmdId, cmdId, subCmdId, subCmdId);

	switch (cmdId)
	{
	case CMDID_TIMEINDICATION:
		// Not implemented
		break;
	case CMDID_EVENTS:
		notif_events = (dn_ipmt_events_nt*)dn_fsm_vars.notifBuf;
		debug("State: %#.2x | Events: %#.4x", notif_events->state, notif_events->events);

		// Check if in fsm state where we expect a certain mote event
		switch (dn_fsm_vars.state)
		{
		case FSM_STATE_JOINING:
			if (notif_events->events & MOTE_EVENT_MASK_OPERATIONAL)
			{
				// Join complete
				if (dn_fsm_vars.service_ms > 0)
				{
					fsm_enterState(FSM_STATE_REQ_SERVICE, 0);
				} else
				{
					fsm_enterState(FSM_STATE_CONNECTED, 0);
				}
				return;
			}
			break;
		case FSM_STATE_REQ_SERVICE:
			if (notif_events->events & MOTE_EVENT_MASK_SVC_CHANGE)
			{
				// Service request complete; check what we were granted
				fsm_scheduleEvent(CMD_PERIOD_MS, event_getServiceInfo);
				return;
			}
			break;
		}

		// Check if reported mote state should trigger fsm state transition
		switch (notif_events->state)
		{
		case MOTE_STATE_IDLE:
			switch (dn_fsm_vars.state)
			{
			case FSM_STATE_PRE_JOIN:
			case FSM_STATE_JOINING:
			case FSM_STATE_REQ_SERVICE:
			case FSM_STATE_RESETTING:
				// Restart during connect; retry
				fsm_enterState(FSM_STATE_PRE_JOIN, 0);
				break;
			case FSM_STATE_CONNECTED:
			case FSM_STATE_SENDING:
				// Disconnect/reset; set state accordingly
				fsm_enterState(FSM_STATE_DISCONNECTED, 0);
				break;
			}
			break;
		case MOTE_STATE_OPERATIONAL:
			switch (dn_fsm_vars.state)
			{
			case FSM_STATE_PRE_JOIN:
				/*
				 Early (and unexpected) operational (connected to network)
				 during connect; reset and retry
				 */
				fsm_enterState(FSM_STATE_RESETTING, 0);
				break;
			}
			break;
		}
		break;
	case CMDID_RECEIVE:
		notif_receive = (dn_ipmt_receive_nt*)dn_fsm_vars.notifBuf;
		debug("Received downstream data");

		if (dn_fsm_vars.inbox.unreadPackets < INBOX_SIZE)
		{
			// Push payload at tail of inbox
			memcpy
					(
					dn_fsm_vars.inbox.pktBuf[dn_fsm_vars.inbox.tail],
					notif_receive->payload,
					notif_receive->payloadLen
					);
			dn_fsm_vars.inbox.pktSize[dn_fsm_vars.inbox.tail] = notif_receive->payloadLen;
			dn_fsm_vars.inbox.tail = (dn_fsm_vars.inbox.tail + 1) % INBOX_SIZE;
			dn_fsm_vars.inbox.unreadPackets++;
		} else
		{
			log_warn("Inbox overflow");
		}
		break;
	case CMDID_MACRX:
		// Not implemented
		break;
	case CMDID_TXDONE:
		// Not implemented
		break;
	case CMDID_ADVRECEIVED:
		notif_advReceived = (dn_ipmt_advReceived_nt*)dn_fsm_vars.notifBuf;
		debug("Received network advertisement");
		
		if (dn_fsm_vars.state == FSM_STATE_PROMISCUOUS
				&& dn_fsm_vars.networkId == PROMISCUOUS_NET_ID)
		{
			debug("Saving network ID: %#.4x (%u)",
					notif_advReceived->netId, notif_advReceived->netId);
			dn_fsm_vars.networkId = notif_advReceived->netId;
			fsm_scheduleEvent(CMD_PERIOD_MS, event_setNetworkId);
		}
		
		break;
	default:
		log_warn("Unknown notification ID");
		break;
	}
}

//===== reply_cb

/**
 This function is called whenever a reply is received through the SmartMesh.
 C Library. It calls the reply function that was armed at the start of the
 current event.
 */
static void dn_ipmt_reply_cb(uint8_t cmdId)
{
	debug("Got reply: cmdId; %#.2x (%u)", cmdId, cmdId);
	if (dn_fsm_vars.replyCb == NULL)
	{
		debug("Reply callback empty");
		return;
	}
	dn_fsm_vars.replyCb();
}

//===== response_timeout

/**
 This event is scheduled after each mote API command is sent, effectively
 placing a timeout for the mote to reply.
 */
static void event_response_timeout(void)
{
	debug("Response timeout");

	// Cancel any ongoing transmission and reset reply cb
	dn_ipmt_cancelTx();
	dn_fsm_vars.replyCb = NULL;

	switch (dn_fsm_vars.state)
	{
	case FSM_STATE_PRE_JOIN:
	case FSM_STATE_JOINING:
	case FSM_STATE_REQ_SERVICE:
	case FSM_STATE_RESETTING:
		// Response timeout during connect; retry
		fsm_enterState(FSM_STATE_PRE_JOIN, 0);
		break;
	case FSM_STATE_SENDING:
		// Response timeout during send; fail (TODO: Try again instead?)
		fsm_enterState(FSM_STATE_SEND_FAILED, 0);
		break;
	default:
		log_err("Response timeout in unexpected state: %#x", dn_fsm_vars.state);
		break;
	}
}

//===== reset

/**
 Initiates a soft-reset of the mote. Its reply simply checks that
 the command was accepted, as the FSM will wait for the ensuing boot event.
 */
static void event_reset(void)
{
	debug("Reset");
	// Arm reply callback
	fsm_setReplyCallback(reply_reset);

	// Issue mote API command
	dn_ipmt_reset
			(
			(dn_ipmt_reset_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, event_response_timeout);
}

static void reply_reset(void)
{
	dn_ipmt_reset_rpt* reply;
	debug("Reset reply");

	// Cancel reply timeout
	fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_reset_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case RC_OK:
		debug("Mote soft-reset initiated");
		// Will wait for notification of reboot
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		fsm_enterState(FSM_STATE_DISCONNECTED, 0);
		break;
	}
}

//===== disconnect

/**
 This event does much the same as reset, however it uses the disconnect command
 instead, where the mote first spends a couple of seconds notifying its
 neighbors of its imminent soft-reset. If the reply is anything but success,
 it will schedule a simple reset event instead.
 */
static void event_disconnect(void)
{
	debug("Disconnect");

	// Arm reply callback
	fsm_setReplyCallback(reply_disconnect);

	// Issue mote API command
	dn_ipmt_disconnect
			(
			(dn_ipmt_disconnect_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, event_response_timeout);
}

static void reply_disconnect(void)
{
	dn_ipmt_disconnect_rpt* reply;
	debug("Disconnect reply");

	// Cancel reply timeout
	fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_disconnect_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case RC_OK:
		debug("Mote disconnect initiated");
		// Will wait for notification of reboot
		break;
	case RC_INVALID_STATE:
		debug("The mote is in an invalid state to disconnect; resetting");
		fsm_scheduleEvent(CMD_PERIOD_MS, event_reset);
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		fsm_scheduleEvent(CMD_PERIOD_MS, event_reset);
		break;
	}
}

//===== getMoteStatus

/**
 Asks the mote for its status, and the reply will use the reported
 mote state to decide whether or not it is ready to proceed with pre-join
 configurations or if a reset is needed first.
 */
static void event_getMoteStatus(void)
{
	debug("Mote status");

	// Arm reply callback
	fsm_setReplyCallback(reply_getMoteStatus);

	// Issue mote API command
	dn_ipmt_getParameter_moteStatus
			(
			(dn_ipmt_getParameter_moteStatus_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, event_response_timeout);
}

static void reply_getMoteStatus(void)
{
	dn_ipmt_getParameter_moteStatus_rpt* reply;
	debug("Mote status reply");

	// Cancel reply timeout
	fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_getParameter_moteStatus_rpt*)dn_fsm_vars.replyBuf;
	debug("State: %#x", reply->state);

	// Choose next event or state transition
	switch (reply->state)
	{
	case MOTE_STATE_IDLE:
		fsm_scheduleEvent(CMD_PERIOD_MS, event_openSocket);
		break;
	case MOTE_STATE_OPERATIONAL:
		fsm_enterState(FSM_STATE_RESETTING, 0);
		break;
	default:
		fsm_enterState(FSM_STATE_RESETTING, 0);
		break;
	}
}

//===== openSocket

/**
 Tells the mote to open a socket, and the reply saves the reported
 socket ID before scheduling its binding. If no sockets are available, a mote
 reset is scheduled and the connect process starts over.
 */
static void event_openSocket(void)
{
	debug("Open socket");

	// Arm reply callback
	fsm_setReplyCallback(reply_openSocket);

	// Issue mote API command
	dn_ipmt_openSocket
			(
			PROTOCOL_TYPE_UDP,
			(dn_ipmt_openSocket_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, event_response_timeout);
}

static void reply_openSocket(void)
{
	dn_ipmt_openSocket_rpt* reply;
	debug("Open socket reply");

	// Cancel reply timeout
	fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_openSocket_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case RC_OK:
		debug("Socket %d opened successfully", reply->socketId);
		dn_fsm_vars.socketId = reply->socketId;
		fsm_scheduleEvent(CMD_PERIOD_MS, event_bindSocket);
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

//===== bindSocket

/**
 Binds the previously opened socket to a port. If said port is already bound,
 a mote reset is scheduled and the connect process starts over.
 */
static void event_bindSocket(void)
{
	debug("Bind socket");

	// Arm reply callback
	fsm_setReplyCallback(reply_bindSocket);

	// Issue mote API command
	dn_ipmt_bindSocket
			(
			dn_fsm_vars.socketId,
			SRC_PORT,
			(dn_ipmt_bindSocket_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, event_response_timeout);
}

static void reply_bindSocket(void)
{
	dn_ipmt_bindSocket_rpt* reply;
	debug("Bind socket reply");

	// Cancel reply timeout
	fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_bindSocket_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case RC_OK:
		debug("Socket bound successfully");
		fsm_scheduleEvent(CMD_PERIOD_MS, event_setJoinKey);
		break;
	case RC_BUSY:
		debug("Port already bound");
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

//===== setJoinKey

/**
 Configures the join key that the mote should use when attempting to join a
 network.
 */
static void event_setJoinKey(void)
{
	debug("Set join key");

	// Arm reply callback
	fsm_setReplyCallback(reply_setJoinKey);

	// Issue mote API command
	dn_ipmt_setParameter_joinKey
			(
			dn_fsm_vars.joinKey,
			(dn_ipmt_setParameter_joinKey_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, event_response_timeout);
}

static void reply_setJoinKey(void)
{
	dn_ipmt_setParameter_joinKey_rpt* reply;
	debug("Set join key reply");

	// Cancel reply timeout
	fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_setParameter_joinKey_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case RC_OK:
		debug("Join key set");
		if (dn_fsm_vars.networkId == PROMISCUOUS_NET_ID)
		{
			// Promiscuous netID set; search for new first
			fsm_enterState(FSM_STATE_PROMISCUOUS, 0);
			/*
			 As of version 1.4.x, a network ID of 0xFFFF can be used to indicate
			 that the mote should join the first network heard. Thus, searching
			 before joining will not be necessary.
			*/
		} else
		{
			fsm_scheduleEvent(CMD_PERIOD_MS, event_setNetworkId);
		}
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

//===== setNetworkId

/**
 Configures the ID of the network that the mote should should try to join.
 */
static void event_setNetworkId(void)
{
	debug("Set network ID");

	// Arm reply callback
	fsm_setReplyCallback(reply_setNetworkId);

	// Issue mote API command
	dn_ipmt_setParameter_networkId
			(
			dn_fsm_vars.networkId,
			(dn_ipmt_setParameter_networkId_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, event_response_timeout);
}

static void reply_setNetworkId(void)
{
	dn_ipmt_setParameter_networkId_rpt* reply;
	debug("Set network ID reply");

	// Cancel reply timeout
	fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_setParameter_networkId_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
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

//===== search

/**
 Tells the mote to start listening for network advertisements. The mote will
 then report the network ID (among other things) of any advertisements heard.
 Upon a successful reply, the FSM will wait for an advReceived notification,
 before attempting to join the reported network.
 */
static void event_search(void)
{
	debug("Search");
	
	// Arm reply callback
	fsm_setReplyCallback(reply_search);

	// Issue mote API command
	dn_ipmt_search
			(
			(dn_ipmt_search_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, event_response_timeout);
}

static void reply_search(void)
{
	dn_ipmt_search_rpt* reply;
	debug("Search reply");

	// Cancel reply timeout
	fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_search_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case RC_OK:
		debug("Searching for network advertisements");
		// Will wait for notification of advertisement received
		break;
	case RC_INVALID_STATE:
		debug("The mote is in an invalid state to start searching");
		fsm_enterState(FSM_STATE_RESETTING, 0);
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		fsm_enterState(FSM_STATE_DISCONNECTED, 0);
		break;
	}
}

//===== join

/**
 Requests that the mote start searching for the previously configured network
 and attempt to join with the configured join key. If the mote is in an invalid
 state to join or lacks configuration to start joining, a reset is scheduled and
 the connect procedure starts over. Otherwise the FSM will wait for the ensuing
 operational event when the mote has finished joining.
 */
static void event_join(void)
{
	debug("Join");

	// Arm reply callback
	fsm_setReplyCallback(reply_join);

	// Issue mote API command
	dn_ipmt_join
			(
			(dn_ipmt_join_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, event_response_timeout);
}

static void reply_join(void)
{
	dn_ipmt_join_rpt* reply;
	debug("Join reply");

	// Cancel reply timeout
	fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_join_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case RC_OK:
		debug("Join operation started");
		// Will wait for join complete notification (operational event)
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

//===== requestService

/**
 The mote is told to request a new service level from the manager. Its reply
 simply checks that the command was accepted, as the FSM will wait for the
 ensuing svcChange event when the service allocation has changed.
 */
static void event_requestService(void)
{
	debug("Request service");

	// Arm reply callback
	fsm_setReplyCallback(reply_requestService);

	// Issue mote API command
	dn_ipmt_requestService
			(
			SERVICE_ADDRESS,
			SERVICE_TYPE_BW,
			dn_fsm_vars.service_ms,
			(dn_ipmt_requestService_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, event_response_timeout);
}

static void reply_requestService(void)
{
	dn_ipmt_requestService_rpt* reply;
	debug("Request service reply");

	// Cancel reply timeout
	fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_requestService_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
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

//===== getServiceInfo

/**
 Requests details about the service currently allocated to the mote. Its reply
 checks that we have been granted a service equal to or better than what was
 requested (smaller value equals better).
 */
static void event_getServiceInfo(void)
{
	debug("Get service info");

	// Arm reply callback
	fsm_setReplyCallback(reply_getServiceInfo);

	// Issue mote API command
	dn_ipmt_getServiceInfo
			(
			SERVICE_ADDRESS,
			SERVICE_TYPE_BW,
			(dn_ipmt_getServiceInfo_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, event_response_timeout);
}

static void reply_getServiceInfo(void)
{
	dn_ipmt_getServiceInfo_rpt* reply;
	debug("Get service info reply");

	// Cancel reply timeout
	fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_getServiceInfo_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
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
				// TODO: Should maybe fail?
			}
			fsm_enterState(FSM_STATE_CONNECTED, 0);
		} else
		{
			debug("Service request still pending");
			fsm_scheduleEvent(CMD_PERIOD_MS, event_getServiceInfo);
		}
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		fsm_enterState(FSM_STATE_DISCONNECTED, 0);
		break;
	}
}

//===== sendTo

/**
 This event sends a packet into the network, and its reply checks that it was
 accepted and queued up for transmission.
 */
static void event_sendTo(void)
{
	dn_err_t err;
	debug("Send");

	// Arm reply callback
	fsm_setReplyCallback(reply_sendTo);

	// Issue mote API command
	err = dn_ipmt_sendTo
			(
			dn_fsm_vars.socketId,
			dn_fsm_vars.destIPv6,
			dn_fsm_vars.destPort,
			SERVICE_TYPE_BW,
			PACKET_PRIORITY_MEDIUM,
			PACKET_ID_NO_NOTIF,
			dn_fsm_vars.payloadBuf,
			dn_fsm_vars.payloadSize,
			(dn_ipmt_sendTo_rpt*)dn_fsm_vars.replyBuf
			);
	if (err != DN_ERR_NONE)
	{
		debug("Send error: %u", err);
		fsm_enterState(FSM_STATE_SEND_FAILED, 0);
	}

	// Schedule timeout for reply
	fsm_scheduleEvent(SERIAL_RESPONSE_TIMEOUT_MS, event_response_timeout);
}

static void reply_sendTo(void)
{
	dn_ipmt_sendTo_rpt* reply;
	debug("Send reply");

	// Cancel reply timeout
	fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_sendTo_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case RC_OK:
		debug("Packet was queued up for transmission");
		fsm_enterState(FSM_STATE_CONNECTED, 0);
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
	} else if (netID == PROMISCUOUS_NET_ID)
	{
		debug("Promiscuous network ID given; will search for and join first network advertised");
		dn_fsm_vars.networkId = netID;
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

static uint8_t getPayloadLimit(uint16_t destPort)
{
	bool destIsF0Bx = (destPort >= WELL_KNOWN_PORT_1 && destPort <= WELL_KNOWN_PORT_8);
	bool srcIsF0Bx = (SRC_PORT >= WELL_KNOWN_PORT_1 && SRC_PORT <= WELL_KNOWN_PORT_8);
	int8_t destIsMng = memcmp(DEST_IP, DEFAULT_DEST_IP, IPv6ADDR_LEN);
	
	if (destIsMng == 0)
	{
		if (destIsF0Bx && srcIsF0Bx)
			return PAYLOAD_SIZE_LIMIT_MNG_HIGH;
		else if (destIsF0Bx || srcIsF0Bx)
			return PAYLOAD_SIZE_LIMIT_MNG_MED;
		else
			return PAYLOAD_SIZE_LIMIT_MNG_LOW;
	} else
	{
		if (destIsF0Bx && srcIsF0Bx)
			return PAYLOAD_SIZE_LIMIT_IP_HIGH;
		else if (destIsF0Bx || srcIsF0Bx)
			return PAYLOAD_SIZE_LIMIT_IP_MED;
		else
			return PAYLOAD_SIZE_LIMIT_IP_LOW;
	}
}

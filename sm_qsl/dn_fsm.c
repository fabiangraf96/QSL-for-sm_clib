/*
Copyright (c) 2016, Dust Networks. All rights reserved.

Finite State Machine for the QuickStart Library.

\license See attached DN_LICENSE.txt.
*/

#include "dn_fsm.h"
#include "dn_ipmt.h"
#include "dn_time.h"
#include "dn_watchdog.h"
#include "dn_qsl_api.h"
#include "dn_debug.h"
#include <zephyr/kernel.h>

#define debug(fmt, ...) printk("DEBUG: " fmt "\n", ##__VA_ARGS__)

//=========================== variables =======================================

typedef struct
{
	// FSM
	uint32_t fsmEventScheduled_ms;
	uint16_t fsmDelay_ms;
	bool fsmArmed;
	uint8_t state;
	// C Library API
	dn_fsm_reply_cbt replyCb;
	dn_fsm_timer_cbt fsmCb;
	uint8_t replyBuf[MAX_FRAME_LENGTH];
	uint8_t notifBuf[MAX_FRAME_LENGTH];
	// Connection
	uint8_t socketId;
	uint16_t networkId;
	uint8_t joinKey[DN_JOIN_KEY_LEN];
	uint16_t srcPort;
	uint32_t service_ms;
	uint8_t payloadBuf[DN_DEFAULT_PAYLOAD_SIZE_LIMIT];
	uint8_t payloadSize;
	uint8_t destIPv6[DN_IPv6ADDR_LEN];
	uint16_t destPort;
	dn_inbox_t inbox;
} dn_fsm_vars_t;

static dn_fsm_vars_t dn_fsm_vars;


//=========================== prototypes ======================================
// FSM
static void dn_fsm_run(void);
static void dn_fsm_scheduleEvent(uint16_t delay, dn_fsm_timer_cbt cb);
static void dn_fsm_cancelEvent(void);
static void dn_fsm_setReplyCallback(dn_fsm_reply_cbt cb);
static void dn_fsm_enterState(uint8_t newState, uint16_t spesificDelay);
static bool dn_fsm_cmd_timeout(uint32_t cmdStart_ms, uint32_t cmdTimeout_ms);
// C Library API
static void dn_ipmt_notif_cb(uint8_t cmdId, uint8_t subCmdId);
static void dn_ipmt_reply_cb(uint8_t cmdId);
static void dn_event_responseTimeout(void);
static void dn_event_reset(void);
static void dn_reply_reset(void);
static void dn_event_disconnect(void);
static void dn_reply_disconnect(void);
static void dn_event_getMoteStatus(void);
static void dn_reply_getMoteStatus(void);
static void dn_event_openSocket(void);
static void dn_reply_openSocket(void);
static void dn_event_bindSocket(void);
static void dn_reply_bindSocket(void);
static void dn_event_setJoinKey(void);
static void dn_reply_setJoinKey(void);
static void dn_event_setNetworkId(void);
static void dn_reply_setNetworkId(void);
static void dn_event_search(void);
static void dn_reply_search(void);
static void dn_event_join(void);
static void dn_reply_join(void);
static void dn_event_requestService(void);
static void dn_reply_requestService(void);
static void dn_event_getServiceInfo(void);
static void dn_reply_getServiceInfo(void);
static void dn_event_sendTo(void);
static void dn_reply_sendTo(void);
// helpers
static dn_err_t checkAndSaveNetConfig(uint16_t netID, const uint8_t* joinKey, uint16_t srcPort, uint32_t req_service_ms);
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

	dn_fsm_enterState(DN_FSM_STATE_DISCONNECTED, 0);
	return TRUE;
}

bool dn_qsl_isConnected(void)
{
	debug("QSL: isConnected");
	return dn_fsm_vars.state == DN_FSM_STATE_CONNECTED;
}

bool dn_qsl_connect(uint16_t netID, const uint8_t* joinKey, uint16_t srcPort, uint32_t req_service_ms)
{
	uint32_t cmdStart_ms = dn_time_ms();
	dn_err_t err;
	debug("QSL: Connect");
	switch (dn_fsm_vars.state)
	{
	case DN_FSM_STATE_NOT_INITIALIZED:
		log_warn("Can't connect; not initialized");
		return FALSE;
	case DN_FSM_STATE_DISCONNECTED:
		err = checkAndSaveNetConfig(netID, joinKey, srcPort, req_service_ms);
		if (err != DN_ERR_NONE)
		{
			return FALSE;
		}
		debug("Starting connect process...");
		dn_fsm_enterState(DN_FSM_STATE_PRE_JOIN, 0);
		break;
	case DN_FSM_STATE_CONNECTED:
		if ((netID > 0 && netID != dn_fsm_vars.networkId)
				|| (joinKey != NULL && memcmp(joinKey, dn_fsm_vars.joinKey, DN_JOIN_KEY_LEN) != 0)
				|| (srcPort > 0 && srcPort != dn_fsm_vars.srcPort))
		{
			err = checkAndSaveNetConfig(netID, joinKey, srcPort, req_service_ms);
			if (err != DN_ERR_NONE)
			{
				return FALSE;
			}
			debug("New network ID, join key and/or source port; reconnecting...");
			dn_fsm_enterState(DN_FSM_STATE_RESETTING, 0);
		} else if (req_service_ms > 0 && req_service_ms != dn_fsm_vars.service_ms)
		{
			debug("New service request");
			dn_fsm_vars.service_ms = req_service_ms;
			dn_fsm_enterState(DN_FSM_STATE_REQ_SERVICE, 0);
		} else
		{
			debug("Already connected");
			// Nothing to do
		}
		break;
	default:
		log_err("Unexpected state");
		dn_fsm_enterState(DN_FSM_STATE_DISCONNECTED, 0);
		return FALSE;
	}

	// Drive FSM until connect success/failure or timeout
	while (dn_fsm_vars.state != DN_FSM_STATE_CONNECTED
			&& dn_fsm_vars.state != DN_FSM_STATE_DISCONNECTED
			&& !dn_fsm_cmd_timeout(cmdStart_ms, DN_CONNECT_TIMEOUT_S * 1000))
	{
		dn_watchdog_feed();
		dn_fsm_run();
	}

	return dn_fsm_vars.state == DN_FSM_STATE_CONNECTED;
}

bool dn_qsl_send(const uint8_t* payload, uint8_t payloadSize_B, uint16_t destPort)
{
	uint32_t cmdStart_ms = dn_time_ms();
	uint8_t maxPayloadSize;
	debug("QSL: Send");
	switch (dn_fsm_vars.state)
	{
	case DN_FSM_STATE_CONNECTED:
		maxPayloadSize = getPayloadLimit(destPort);

		if (payloadSize_B > maxPayloadSize)
		{
			log_warn("Payload size (%u) exceeds limit (%u)", payloadSize_B, maxPayloadSize);
			return FALSE;
		}
		// Store outbound payload and parameters
		memcpy(dn_fsm_vars.payloadBuf, payload, payloadSize_B);
		dn_fsm_vars.payloadSize = payloadSize_B;
		memcpy(dn_fsm_vars.destIPv6, DN_DEST_IP, DN_IPv6ADDR_LEN);
		dn_fsm_vars.destPort = (destPort > 0) ? destPort : DN_DEFAULT_DEST_PORT;
		// Start send process
		dn_fsm_enterState(DN_FSM_STATE_SENDING, 0);
		break;
	default:
		log_warn("Can't send; not connected");
		return FALSE;
	}

	// Drive FSM until send success/failure or timeout
	while (dn_fsm_vars.state == DN_FSM_STATE_SENDING
			&& !dn_fsm_cmd_timeout(cmdStart_ms, DN_SEND_TIMEOUT_MS))
	{
		dn_watchdog_feed();
		dn_fsm_run();
	}

	// Catch send failure
	if (dn_fsm_vars.state == DN_FSM_STATE_SEND_FAILED)
	{
		debug("Send failed");
		dn_fsm_enterState(DN_FSM_STATE_CONNECTED, 0);
		return FALSE;
	}

	return dn_fsm_vars.state == DN_FSM_STATE_CONNECTED;
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
		dn_fsm_vars.inbox.head = (dn_fsm_vars.inbox.head + 1) % DN_INBOX_SIZE;
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
static void dn_fsm_run(void)
{
	uint32_t timePassed_ms = dn_time_ms() - dn_fsm_vars.fsmEventScheduled_ms; // Handle dn_time_ms wrap around
	if (dn_fsm_vars.fsmArmed && (timePassed_ms > dn_fsm_vars.fsmDelay_ms))
	{
		// Scheduled event is due; execute it
		dn_fsm_vars.fsmArmed = FALSE;
		if (dn_fsm_vars.fsmCb != NULL)
		{
			dn_fsm_vars.fsmCb();
		}
	} else
	{
		// Sleep to save CPU power
		dn_sleep_ms(DN_FSM_RUN_INTERVAL_MS);
	}
}

//===== scheduleEvent

/**
 Schedule function to be called after a given delay.
 */
static void dn_fsm_scheduleEvent(uint16_t delay_ms, dn_fsm_timer_cbt cb)
{
	dn_fsm_vars.fsmEventScheduled_ms = dn_time_ms();
	dn_fsm_vars.fsmDelay_ms = delay_ms;
	dn_fsm_vars.fsmCb = cb;
	dn_fsm_vars.fsmArmed = TRUE;
}

//===== cancelEvent

/**
 Cancel currently scheduled event.
 */
static void dn_fsm_cancelEvent(void)
{
	dn_fsm_vars.fsmDelay_ms = 0;
	dn_fsm_vars.fsmCb = NULL;
	dn_fsm_vars.fsmArmed = FALSE;
}

//===== setReplyCallback

/**
 Set the callback function that the C Library will execute when the next reply
 is received and the reply buffer is ready to be parsed.
 */
static void dn_fsm_setReplyCallback(dn_fsm_reply_cbt cb)
{
	dn_fsm_vars.replyCb = cb;
}

//===== enterState

/**
 Transition FSM to new state and schedule any default entry events.
 */
static void dn_fsm_enterState(uint8_t newState, uint16_t spesificDelay)
{
	uint32_t now = dn_time_ms();
	uint16_t delay = DN_CMD_PERIOD_MS;
	static uint32_t lastTransition = 0;
	if (lastTransition == 0)
		lastTransition = now;

	// Use default delay if none given
	if (spesificDelay > 0)
		delay = spesificDelay;

	// Schedule default events for transition into states
	switch (newState)
	{
	case DN_FSM_STATE_PRE_JOIN:
		dn_fsm_scheduleEvent(delay, dn_event_getMoteStatus);
		break;
	case DN_FSM_STATE_PROMISCUOUS:
		dn_fsm_scheduleEvent(delay, dn_event_search);
		break;
	case DN_FSM_STATE_JOINING:
		dn_fsm_scheduleEvent(delay, dn_event_join);
		break;
	case DN_FSM_STATE_REQ_SERVICE:
		dn_fsm_scheduleEvent(delay, dn_event_requestService);
		break;
	case DN_FSM_STATE_RESETTING:
		if (DN_MOTE_DISCONNECT_BEFORE_RESET)
			dn_fsm_scheduleEvent(delay, dn_event_disconnect); // More graceful
		else
			dn_fsm_scheduleEvent(delay, dn_event_reset); // Faster
		break;
	case DN_FSM_STATE_SENDING:
		/*
		 Send is scheduled immediately because it is the users responsibility
		 to implement the necessary backoff and not exceed the granted bandwidth.
		 */
		dn_fsm_scheduleEvent(0, dn_event_sendTo);
		break;
	case DN_FSM_STATE_SEND_FAILED:
	case DN_FSM_STATE_DISCONNECTED:
	case DN_FSM_STATE_CONNECTED:
		// These states have no default entry events
		break;
	default:
		log_warn("Attempt at entering unexpected state %#.2x", newState);
		return;
	}

	debug("FSM state transition: %#.2x --> %#.2x (%u ms)",
			dn_fsm_vars.state, newState, (uint32_t)(now - lastTransition));
	lastTransition = now;
	dn_fsm_vars.state = newState;
}

//===== cmdTimeout

/**
 Correctly abort the current API command if time passed since the given start
 has exceeded the given timeout.
 */
static bool dn_fsm_cmd_timeout(uint32_t cmdStart_ms, uint32_t cmdTimeout_ms)
{
	uint32_t timePassed_ms = dn_time_ms() - cmdStart_ms; // Handle dn_time_ms wrap around
	bool timeout = timePassed_ms > cmdTimeout_ms;
	if (timeout)
	{
		// Cancel any ongoing transmission or scheduled event and reset reply cb
		dn_ipmt_cancelTx();
		dn_fsm_vars.replyCb = NULL;
		dn_fsm_cancelEvent();

		// Default timeout state is different while connecting vs sending
		switch (dn_fsm_vars.state)
		{
		case DN_FSM_STATE_PRE_JOIN:
		case DN_FSM_STATE_JOINING:
		case DN_FSM_STATE_REQ_SERVICE:
		case DN_FSM_STATE_RESETTING:
			debug("Connect timeout");
			dn_fsm_enterState(DN_FSM_STATE_DISCONNECTED, 0);
			break;
		case DN_FSM_STATE_SENDING:
			debug("Send timeout");
			dn_fsm_enterState(DN_FSM_STATE_SEND_FAILED, 0);
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
		case DN_FSM_STATE_JOINING:
			if (notif_events->events & DN_MOTE_EVENT_MASK_OPERATIONAL)
			{
				// Join complete
				if (dn_fsm_vars.service_ms > 0)
				{
					dn_fsm_enterState(DN_FSM_STATE_REQ_SERVICE, 0);
				} else
				{
					dn_fsm_enterState(DN_FSM_STATE_CONNECTED, 0);
				}
				return;
			}
			break;
		case DN_FSM_STATE_REQ_SERVICE:
			if (notif_events->events & DN_MOTE_EVENT_MASK_SVC_CHANGE)
			{
				// Service request complete; check what we were granted
				dn_fsm_scheduleEvent(DN_CMD_PERIOD_MS, dn_event_getServiceInfo);
				return;
			}
			break;
		}

		// Check if reported mote state should trigger fsm state transition
		switch (notif_events->state)
		{
		case DN_MOTE_STATE_IDLE:
			switch (dn_fsm_vars.state)
			{
			case DN_FSM_STATE_PRE_JOIN:
			case DN_FSM_STATE_JOINING:
			case DN_FSM_STATE_REQ_SERVICE:
			case DN_FSM_STATE_RESETTING:
			case DN_FSM_STATE_PROMISCUOUS:
				// Restart during connect; retry
				dn_fsm_enterState(DN_FSM_STATE_PRE_JOIN, 0);
				break;
			case DN_FSM_STATE_CONNECTED:
			case DN_FSM_STATE_SENDING:
			case DN_FSM_STATE_SEND_FAILED:
				// Disconnect/reset; set state accordingly
				dn_fsm_enterState(DN_FSM_STATE_DISCONNECTED, 0);
				break;
			}
			break;
		case DN_MOTE_STATE_OPERATIONAL:
			switch (dn_fsm_vars.state)
			{
			case DN_FSM_STATE_PRE_JOIN:
			case DN_FSM_STATE_PROMISCUOUS:
				/*
				 Early (and unexpected) operational (connected to network)
				 during connect; reset and retry
				 */
				dn_fsm_enterState(DN_FSM_STATE_RESETTING, 0);
				break;
			}
			break;
		}
		break;
	case CMDID_RECEIVE:
		notif_receive = (dn_ipmt_receive_nt*)dn_fsm_vars.notifBuf;
		debug("Received downstream data");

		// Push payload at tail of inbox
		memcpy
				(
				dn_fsm_vars.inbox.pktBuf[dn_fsm_vars.inbox.tail],
				notif_receive->payload,
				notif_receive->payloadLen
				);
		dn_fsm_vars.inbox.pktSize[dn_fsm_vars.inbox.tail] = notif_receive->payloadLen;
		dn_fsm_vars.inbox.tail = (dn_fsm_vars.inbox.tail + 1) % DN_INBOX_SIZE;
		if(dn_fsm_vars.inbox.unreadPackets == DN_INBOX_SIZE)
		{
			log_warn("Inbox overflow; oldest packet dropped");
		} else
		{
			dn_fsm_vars.inbox.unreadPackets++;
		}
		debug("Inbox capacity at %u / %u", dn_fsm_vars.inbox.unreadPackets, DN_INBOX_SIZE);

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

		if (dn_fsm_vars.state == DN_FSM_STATE_PROMISCUOUS
				&& dn_fsm_vars.networkId == DN_PROMISCUOUS_NET_ID)
		{
			debug("Saving network ID: %#.4x (%u)",
					notif_advReceived->netId, notif_advReceived->netId);
			dn_fsm_vars.networkId = notif_advReceived->netId;
			dn_fsm_scheduleEvent(DN_CMD_PERIOD_MS, dn_event_setNetworkId);
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
static void dn_event_responseTimeout(void)
{
	debug("Response timeout");

	// Cancel any ongoing transmission and reset reply cb
	dn_ipmt_cancelTx();
	dn_fsm_vars.replyCb = NULL;

	switch (dn_fsm_vars.state)
	{
	case DN_FSM_STATE_PRE_JOIN:
	case DN_FSM_STATE_JOINING:
	case DN_FSM_STATE_REQ_SERVICE:
	case DN_FSM_STATE_RESETTING:
	case DN_FSM_STATE_PROMISCUOUS:
		// Response timeout during connect; retry
		dn_fsm_enterState(DN_FSM_STATE_PRE_JOIN, 0);
		break;
	case DN_FSM_STATE_SENDING:
		// Response timeout during send; fail
		dn_fsm_enterState(DN_FSM_STATE_SEND_FAILED, 0);
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
static void dn_event_reset(void)
{
	debug("Reset");
	// Arm reply callback
	dn_fsm_setReplyCallback(dn_reply_reset);

	// Issue mote API command
	dn_ipmt_reset
			(
			(dn_ipmt_reset_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	dn_fsm_scheduleEvent(DN_SERIAL_RESPONSE_TIMEOUT_MS, dn_event_responseTimeout);
}

static void dn_reply_reset(void)
{
	dn_ipmt_reset_rpt* reply;
	debug("Reset reply");

	// Cancel reply timeout
	dn_fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_reset_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case DN_RC_OK:
		debug("Mote soft-reset initiated");
		// Will wait for notification of reboot
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		dn_fsm_enterState(DN_FSM_STATE_DISCONNECTED, 0);
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
static void dn_event_disconnect(void)
{
	debug("Disconnect");

	// Arm reply callback
	dn_fsm_setReplyCallback(dn_reply_disconnect);

	// Issue mote API command
	dn_ipmt_disconnect
			(
			(dn_ipmt_disconnect_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	dn_fsm_scheduleEvent(DN_SERIAL_RESPONSE_TIMEOUT_MS, dn_event_responseTimeout);
}

static void dn_reply_disconnect(void)
{
	dn_ipmt_disconnect_rpt* reply;
	debug("Disconnect reply");

	// Cancel reply timeout
	dn_fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_disconnect_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case DN_RC_OK:
		debug("Mote disconnect initiated");
		// Will wait for notification of reboot
		break;
	case DN_RC_INVALID_STATE:
		debug("The mote is in an invalid state to disconnect; resetting");
		dn_fsm_scheduleEvent(DN_CMD_PERIOD_MS, dn_event_reset);
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		dn_fsm_scheduleEvent(DN_CMD_PERIOD_MS, dn_event_reset);
		break;
	}
}

//===== getMoteStatus

/**
 Asks the mote for its status, and the reply will use the reported
 mote state to decide whether or not it is ready to proceed with pre-join
 configurations or if a reset is needed first.
 */
static void dn_event_getMoteStatus(void)
{
	debug("Mote status");

	// Arm reply callback
	dn_fsm_setReplyCallback(dn_reply_getMoteStatus);

	// Issue mote API command
	dn_ipmt_getParameter_moteStatus
			(
			(dn_ipmt_getParameter_moteStatus_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	dn_fsm_scheduleEvent(DN_SERIAL_RESPONSE_TIMEOUT_MS, dn_event_responseTimeout);
}

static void dn_reply_getMoteStatus(void)
{
	dn_ipmt_getParameter_moteStatus_rpt* reply;
	debug("Mote status reply");

	// Cancel reply timeout
	dn_fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_getParameter_moteStatus_rpt*)dn_fsm_vars.replyBuf;
	debug("Mote state: %#.2x", reply->state);

	// Choose next event or state transition
	switch (reply->state)
	{
	case DN_MOTE_STATE_IDLE:
		dn_fsm_scheduleEvent(DN_CMD_PERIOD_MS, dn_event_openSocket);
		break;
	case DN_MOTE_STATE_OPERATIONAL:
		dn_fsm_enterState(DN_FSM_STATE_RESETTING, 0);
		break;
	default:
		dn_fsm_enterState(DN_FSM_STATE_RESETTING, 0);
		break;
	}
}

//===== openSocket

/**
 Tells the mote to open a socket, and the reply saves the reported
 socket ID before scheduling its binding. If no sockets are available, a mote
 reset is scheduled and the connect process starts over.
 */
static void dn_event_openSocket(void)
{
	debug("Open socket");

	// Arm reply callback
	dn_fsm_setReplyCallback(dn_reply_openSocket);

	// Issue mote API command
	dn_ipmt_openSocket
			(
			DN_PROTOCOL_TYPE_UDP,
			(dn_ipmt_openSocket_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	dn_fsm_scheduleEvent(DN_SERIAL_RESPONSE_TIMEOUT_MS, dn_event_responseTimeout);
}

static void dn_reply_openSocket(void)
{
	dn_ipmt_openSocket_rpt* reply;
	debug("Open socket reply");

	// Cancel reply timeout
	dn_fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_openSocket_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case DN_RC_OK:
		debug("Socket %d opened successfully", reply->socketId);
		dn_fsm_vars.socketId = reply->socketId;
		dn_fsm_scheduleEvent(DN_CMD_PERIOD_MS, dn_event_bindSocket);
		break;
	case DN_RC_NO_RESOURCES:
		debug("Couldn't create socket due to resource availability");
		dn_fsm_enterState(DN_FSM_STATE_RESETTING, 0);
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		dn_fsm_enterState(DN_FSM_STATE_DISCONNECTED, 0);
		break;
	}
}

//===== bindSocket

/**
 Binds the previously opened socket to a port. If said port is already bound,
 a mote reset is scheduled and the connect process starts over.
 */
static void dn_event_bindSocket(void)
{
	debug("Bind socket");

	// Arm reply callback
	dn_fsm_setReplyCallback(dn_reply_bindSocket);

	// Issue mote API command
	dn_ipmt_bindSocket
			(
			dn_fsm_vars.socketId,
			dn_fsm_vars.srcPort,
			(dn_ipmt_bindSocket_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	dn_fsm_scheduleEvent(DN_SERIAL_RESPONSE_TIMEOUT_MS, dn_event_responseTimeout);
}

static void dn_reply_bindSocket(void)
{
	dn_ipmt_bindSocket_rpt* reply;
	debug("Bind socket reply");

	// Cancel reply timeout
	dn_fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_bindSocket_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case DN_RC_OK:
		debug("Socket bound successfully");
		dn_fsm_scheduleEvent(DN_CMD_PERIOD_MS, dn_event_setJoinKey);
		break;
	case DN_RC_BUSY:
		debug("Port already bound");
		dn_fsm_enterState(DN_FSM_STATE_RESETTING, 0);
		break;
	case DN_RC_NOT_FOUND:
		debug("Invalid socket ID");
		dn_fsm_enterState(DN_FSM_STATE_DISCONNECTED, 0);
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		dn_fsm_enterState(DN_FSM_STATE_DISCONNECTED, 0);
		break;
	}
}

//===== setJoinKey

/**
 Configures the join key that the mote should use when attempting to join a
 network.
 */
static void dn_event_setJoinKey(void)
{
	debug("Set join key");

	// Arm reply callback
	dn_fsm_setReplyCallback(dn_reply_setJoinKey);

	// Issue mote API command
	dn_ipmt_setParameter_joinKey
			(
			dn_fsm_vars.joinKey,
			(dn_ipmt_setParameter_joinKey_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	dn_fsm_scheduleEvent(DN_SERIAL_RESPONSE_TIMEOUT_MS, dn_event_responseTimeout);
}

static void dn_reply_setJoinKey(void)
{
	dn_ipmt_setParameter_joinKey_rpt* reply;
	debug("Set join key reply");

	// Cancel reply timeout
	dn_fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_setParameter_joinKey_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case DN_RC_OK:
		debug("Join key set");
		if (dn_fsm_vars.networkId == DN_PROMISCUOUS_NET_ID)
		{
			// Promiscuous netID set; search for new first
			dn_fsm_enterState(DN_FSM_STATE_PROMISCUOUS, 0);
			/*
			 As of version 1.4.x, a network ID of 0xFFFF can be used to indicate
			 that the mote should join the first network heard. Thus, searching
			 before joining will not be necessary.
			*/
		} else
		{
			dn_fsm_scheduleEvent(DN_CMD_PERIOD_MS, dn_event_setNetworkId);
		}
		break;
	case DN_RC_WRITE_FAIL:
		debug("Could not write the key to storage");
		dn_fsm_enterState(DN_FSM_STATE_DISCONNECTED, 0);
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		dn_fsm_enterState(DN_FSM_STATE_DISCONNECTED, 0);
		break;
	}
}

//===== setNetworkId

/**
 Configures the ID of the network that the mote should should try to join.
 */
static void dn_event_setNetworkId(void)
{
	debug("Set network ID");

	// Arm reply callback
	dn_fsm_setReplyCallback(dn_reply_setNetworkId);

	// Issue mote API command
	dn_ipmt_setParameter_networkId
			(
			dn_fsm_vars.networkId,
			(dn_ipmt_setParameter_networkId_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	dn_fsm_scheduleEvent(DN_SERIAL_RESPONSE_TIMEOUT_MS, dn_event_responseTimeout);
}

static void dn_reply_setNetworkId(void)
{
	dn_ipmt_setParameter_networkId_rpt* reply;
	debug("Set network ID reply");

	// Cancel reply timeout
	dn_fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_setParameter_networkId_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case DN_RC_OK:
		debug("Network ID set");
		dn_fsm_enterState(DN_FSM_STATE_JOINING, 0);
		break;
	case DN_RC_WRITE_FAIL:
		debug("Could not write the network ID to storage");
		dn_fsm_enterState(DN_FSM_STATE_DISCONNECTED, 0);
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		dn_fsm_enterState(DN_FSM_STATE_DISCONNECTED, 0);
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
static void dn_event_search(void)
{
	debug("Search");

	// Arm reply callback
	dn_fsm_setReplyCallback(dn_reply_search);

	// Issue mote API command
	dn_ipmt_search
			(
			(dn_ipmt_search_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	dn_fsm_scheduleEvent(DN_SERIAL_RESPONSE_TIMEOUT_MS, dn_event_responseTimeout);
}

static void dn_reply_search(void)
{
	dn_ipmt_search_rpt* reply;
	debug("Search reply");

	// Cancel reply timeout
	dn_fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_search_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case DN_RC_OK:
		debug("Searching for network advertisements");
		// Will wait for notification of advertisement received
		break;
	case DN_RC_INVALID_STATE:
		debug("The mote is in an invalid state to start searching");
		dn_fsm_enterState(DN_FSM_STATE_RESETTING, 0);
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		dn_fsm_enterState(DN_FSM_STATE_DISCONNECTED, 0);
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
static void dn_event_join(void)
{
	debug("Join");

	// Arm reply callback
	dn_fsm_setReplyCallback(dn_reply_join);

	// Issue mote API command
	dn_ipmt_join
			(
			(dn_ipmt_join_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	dn_fsm_scheduleEvent(DN_SERIAL_RESPONSE_TIMEOUT_MS, dn_event_responseTimeout);
}

static void dn_reply_join(void)
{
	dn_ipmt_join_rpt* reply;
	debug("Join reply");

	// Cancel reply timeout
	dn_fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_join_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case DN_RC_OK:
		debug("Join operation started");
		// Will wait for join complete notification (operational event)
		break;
	case DN_RC_INVALID_STATE:
		debug("The mote is in an invalid state to start join operation");
		dn_fsm_enterState(DN_FSM_STATE_RESETTING, 0);
		break;
	case DN_RC_INCOMPLETE_JOIN_INFO:
		debug("Incomplete configuration to start joining");
		dn_fsm_enterState(DN_FSM_STATE_RESETTING, 0);
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		dn_fsm_enterState(DN_FSM_STATE_DISCONNECTED, 0);
		break;
	}
}

//===== requestService

/**
 The mote is told to request a new service level from the manager. Its reply
 simply checks that the command was accepted, as the FSM will wait for the
 ensuing svcChange event when the service allocation has changed.
 */
static void dn_event_requestService(void)
{
	debug("Request service");

	// Arm reply callback
	dn_fsm_setReplyCallback(dn_reply_requestService);

	// Issue mote API command
	dn_ipmt_requestService
			(
			DN_SERVICE_ADDRESS,
			DN_SERVICE_TYPE_BW,
			dn_fsm_vars.service_ms,
			(dn_ipmt_requestService_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	dn_fsm_scheduleEvent(DN_SERIAL_RESPONSE_TIMEOUT_MS, dn_event_responseTimeout);
}

static void dn_reply_requestService(void)
{
	dn_ipmt_requestService_rpt* reply;
	debug("Request service reply");

	// Cancel reply timeout
	dn_fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_requestService_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case DN_RC_OK:
		debug("Service request accepted");
		// Will wait for svcChanged notification
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		dn_fsm_enterState(DN_FSM_STATE_DISCONNECTED, 0);
		break;
	}
}

//===== getServiceInfo

/**
 Requests details about the service currently allocated to the mote. Its reply
 checks that we have been granted a service equal to or better than what was
 requested (smaller value equals better).
 */
static void dn_event_getServiceInfo(void)
{
	debug("Get service info");

	// Arm reply callback
	dn_fsm_setReplyCallback(dn_reply_getServiceInfo);

	// Issue mote API command
	dn_ipmt_getServiceInfo
			(
			DN_SERVICE_ADDRESS,
			DN_SERVICE_TYPE_BW,
			(dn_ipmt_getServiceInfo_rpt*)dn_fsm_vars.replyBuf
			);

	// Schedule timeout for reply
	dn_fsm_scheduleEvent(DN_SERIAL_RESPONSE_TIMEOUT_MS, dn_event_responseTimeout);
}

static void dn_reply_getServiceInfo(void)
{
	dn_ipmt_getServiceInfo_rpt* reply;
	debug("Get service info reply");

	// Cancel reply timeout
	dn_fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_getServiceInfo_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case DN_RC_OK:
		if (reply->state == DN_SERVICE_STATE_COMPLETED)
		{
			if (reply->value <= dn_fsm_vars.service_ms)
			{
				debug("Granted service of %u ms (requested %u ms)", reply->value, dn_fsm_vars.service_ms);
				dn_fsm_enterState(DN_FSM_STATE_CONNECTED, 0);
			} else
			{
				log_warn("Only granted service of %u ms (requested %u ms)", reply->value, dn_fsm_vars.service_ms);
				dn_fsm_enterState(DN_FSM_STATE_DISCONNECTED, 0);
			}

		} else
		{
			debug("Service request still pending");
			dn_fsm_scheduleEvent(DN_CMD_PERIOD_MS, dn_event_getServiceInfo);
		}
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		dn_fsm_enterState(DN_FSM_STATE_DISCONNECTED, 0);
		break;
	}
}

//===== sendTo

/**
 This event sends a packet into the network, and its reply checks that it was
 accepted and queued up for transmission.
 */
static void dn_event_sendTo(void)
{
	dn_err_t err;
	debug("Send");

	// Arm reply callback
	dn_fsm_setReplyCallback(dn_reply_sendTo);

	// Issue mote API command
	err = dn_ipmt_sendTo
			(
			dn_fsm_vars.socketId,
			dn_fsm_vars.destIPv6,
			dn_fsm_vars.destPort,
			DN_SERVICE_TYPE_BW,
			DN_PACKET_PRIORITY_MEDIUM,
			DN_PACKET_ID_NO_NOTIF,
			dn_fsm_vars.payloadBuf,
			dn_fsm_vars.payloadSize,
			(dn_ipmt_sendTo_rpt*)dn_fsm_vars.replyBuf
			);
	if (err != DN_ERR_NONE)
	{
		debug("Send error: %u", err);
		dn_fsm_enterState(DN_FSM_STATE_SEND_FAILED, 0);
	}

	// Schedule timeout for reply
	dn_fsm_scheduleEvent(DN_SERIAL_RESPONSE_TIMEOUT_MS, dn_event_responseTimeout);
}

static void dn_reply_sendTo(void)
{
	dn_ipmt_sendTo_rpt* reply;
	debug("Send reply");

	// Cancel reply timeout
	dn_fsm_cancelEvent();

	// Parse reply
	reply = (dn_ipmt_sendTo_rpt*)dn_fsm_vars.replyBuf;

	// Choose next event or state transition
	switch (reply->RC)
	{
	case DN_RC_OK:
		debug("Packet was queued up for transmission");
		dn_fsm_enterState(DN_FSM_STATE_CONNECTED, 0);
		break;
	case DN_RC_NO_RESOURCES:
		debug("No queue space to accept the packet");
		dn_fsm_enterState(DN_FSM_STATE_SEND_FAILED, 0);
		break;
	default:
		log_warn("Unexpected response code: %#x", reply->RC);
		dn_fsm_enterState(DN_FSM_STATE_SEND_FAILED, 0);
		break;
	}
}

//=========================== helpers =========================================

static dn_err_t checkAndSaveNetConfig(uint16_t netID, const uint8_t* joinKey, uint16_t srcPort, uint32_t req_service_ms)
{
	if (netID == 0)
	{
		debug("No network ID given; using default");
		dn_fsm_vars.networkId = DN_DEFAULT_NET_ID;
	} else if (netID == DN_PROMISCUOUS_NET_ID)
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
		memcpy(dn_fsm_vars.joinKey, DN_DEFAULT_JOIN_KEY, DN_JOIN_KEY_LEN);
	} else
	{
		memcpy(dn_fsm_vars.joinKey, joinKey, DN_JOIN_KEY_LEN);
	}

	if (srcPort == 0)
	{
		debug("No source port given; using default");
		dn_fsm_vars.srcPort = DN_DEFAULT_SRC_PORT;
	} else
	{
		dn_fsm_vars.srcPort = srcPort;
	}

	if (req_service_ms == 0)
	{
		debug("No service requested; will only be granted base bandwidth");
	}
	dn_fsm_vars.service_ms = req_service_ms;

	return DN_ERR_NONE;
}

static uint8_t getPayloadLimit(uint16_t destPort)
{
	bool destIsF0Bx = (destPort >= DN_WELL_KNOWN_PORT_1 && destPort <= DN_WELL_KNOWN_PORT_8);
	bool srcIsF0Bx = (dn_fsm_vars.srcPort >= DN_WELL_KNOWN_PORT_1 && dn_fsm_vars.srcPort <= DN_WELL_KNOWN_PORT_8);
	int8_t destIsMng = memcmp(DN_DEST_IP, DN_DEFAULT_DEST_IP, DN_IPv6ADDR_LEN);

	if (destIsMng == 0)
	{
		if (destIsF0Bx && srcIsF0Bx)
			return DN_PAYLOAD_SIZE_LIMIT_MNG_HIGH;
		else if (destIsF0Bx || srcIsF0Bx)
			return DN_PAYLOAD_SIZE_LIMIT_MNG_MED;
		else
			return DN_PAYLOAD_SIZE_LIMIT_MNG_LOW;
	} else
	{
		if (destIsF0Bx && srcIsF0Bx)
			return DN_PAYLOAD_SIZE_LIMIT_IP_HIGH;
		else if (destIsF0Bx || srcIsF0Bx)
			return DN_PAYLOAD_SIZE_LIMIT_IP_MED;
		else
			return DN_PAYLOAD_SIZE_LIMIT_IP_LOW;
	}
}

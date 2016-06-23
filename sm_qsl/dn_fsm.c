/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdio.h>
#include <stdlib.h>


#include "dn_fsm.h"
#include "dn_ipmt.h"


//=========================== variables =======================================

typedef struct
{
	uint8_t				replyBuf[MAX_FRAME_LENGTH];
	uint8_t				notifBuf[MAX_FRAME_LENGTH];
} dn_fsm_vars_t;

dn_fsm_vars_t dn_fsm_vars;


//=========================== prototypes ======================================

void dn_ipmt_notif_cb(uint8_t cmdId, uint8_t subCmdId);
void dn_ipmt_reply_cb(uint8_t cmdId);

//=========================== public ==========================================

void dn_fsm_run(void)
{
	dn_err_t rc = DN_ERR_NONE;
	// Reset local variables
	memset(&dn_fsm_vars, 0, sizeof(dn_fsm_vars));
	
	// Initialize the ipmt module
	dn_ipmt_init
			(
			dn_ipmt_notif_cb,
			dn_fsm_vars.notifBuf,
			sizeof(dn_fsm_vars.notifBuf),
			dn_ipmt_reply_cb
			);
	
	sleep(5);
	
	rc = dn_ipmt_getParameter_moteStatus
			(
			(dn_ipmt_getParameter_moteStatus_rpt*)dn_fsm_vars.replyBuf
			);
	printf("Requested moteStatus, rc: %u\n", rc);
	
	sleep(10);
}

//=========================== private =========================================

void dn_ipmt_notif_cb(uint8_t cmdId, uint8_t subCmdId)
{
	printf("Got notification: cmdId; %u, subCmdId; %u\n", cmdId, subCmdId);
}

void dn_ipmt_reply_cb(uint8_t cmdId)
{
	printf("Got reply: cmdId; %u\n", cmdId);
}

//=========================== helpers =========================================
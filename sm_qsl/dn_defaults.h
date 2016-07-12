/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   dn_defaults.h
 * Author: jhbr@datarespons.no
 *
 * Created on 4. juli 2016, 16:31
 */

#ifndef DN_DEFAULTS_H
#define DN_DEFAULTS_H

#include "dn_common.h"

//=========================== defines =========================================

#define IPv6ADDR_LEN	16
#define JOIN_KEY_LEN	16

static const uint8_t default_joinKey[JOIN_KEY_LEN] = {
   0x44,0x55,0x53,0x54,0x4E,0x45,0x54,0x57,
   0x4F,0x52,0x4B,0x53,0x52,0x4F,0x43,0x4B
};
static const uint8_t default_manager_ipv6Addr[IPv6ADDR_LEN] = {
   0xff,0x02,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02
};

/*
 UDP ports in this range are most efficiently compressed inside the mesh,
 and should be used whenever possible to maximize usable payload.
 */
#define WELL_KNOWN_PORT_1	0xf0b8
#define WELL_KNOWN_PORT_2	0xf0b9
#define WELL_KNOWN_PORT_3	0xf0ba
#define WELL_KNOWN_PORT_4	0xf0bb
#define WELL_KNOWN_PORT_5	0xf0bc
#define WELL_KNOWN_PORT_6	0xf0bd
#define WELL_KNOWN_PORT_7	0xf0be
#define WELL_KNOWN_PORT_8	0xf0bf

/*
 The payload size limit varies based on the destination address and whether
 source and destination are set to one of the well-known ports.
 */
#define PAYLOAD_SIZE_LIMIT_MNG_HIGH	90
#define PAYLOAD_SIZE_LIMIT_MNG_MED	88
#define PAYLOAD_SIZE_LIMIT_MNG_LOW	87
#define PAYLOAD_SIZE_LIMIT_IP_HIGH	74
#define PAYLOAD_SIZE_LIMIT_IP_MED	72
#define PAYLOAD_SIZE_LIMIT_IP_LOW	71

/*
 This is not a valid network ID, but will instead cause the QSL to to take the
 mote through a promiscuous listen state to identify the ID of advertising
 networks, further attempting to join the first one found.
 */
#define PROMISCUOUS_NET_ID	0xffff

#define DEFAULT_NET_ID				1229
#define DEFAULT_JOIN_KEY			(uint8_t*)default_joinKey
#define DEFAULT_DEST_PORT			WELL_KNOWN_PORT_1
#define DEFAULT_DEST_IP				(uint8_t*)default_manager_ipv6Addr
#define DEFAULT_SRC_PORT			WELL_KNOWN_PORT_1
#define DEFAULT_PAYLOAD_SIZE_LIMIT	PAYLOAD_SIZE_LIMIT_MNG_HIGH
#define DEFAULT_SERVICE_MS			9000 // Base bandwidth provided by manager

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

#ifdef __cplusplus
extern "C"
{
#endif




#ifdef __cplusplus
}
#endif

#endif /* DN_DEFAULTS_H */


/*
Copyright (c) 2016, Dust Networks. All rights reserved.

Default values and common definitions for the QuickStart Library.

\license See attached DN_LICENSE.txt.
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

#define DN_IPv6ADDR_LEN	16
#define DN_JOIN_KEY_LEN	16

static const uint8_t dn_default_joinKey[DN_JOIN_KEY_LEN] = {
   0x44,0x55,0x53,0x54,0x4E,0x45,0x54,0x57,
   0x4F,0x52,0x4B,0x53,0x52,0x4F,0x43,0x4B
};
static const uint8_t dn_default_manager_ipv6Addr[DN_IPv6ADDR_LEN] = {
   0xff,0x02,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02
};

/*
 UDP ports in this range are most efficiently compressed inside the mesh,
 and should be used whenever possible to maximize usable payload.
 */
#define DN_WELL_KNOWN_PORT_1	0xf0b8
#define DN_WELL_KNOWN_PORT_2	0xf0b9
#define DN_WELL_KNOWN_PORT_3	0xf0ba
#define DN_WELL_KNOWN_PORT_4	0xf0bb
#define DN_WELL_KNOWN_PORT_5	0xf0bc
#define DN_WELL_KNOWN_PORT_6	0xf0bd
#define DN_WELL_KNOWN_PORT_7	0xf0be
#define DN_WELL_KNOWN_PORT_8	0xf0bf

/*
 The payload size limit varies based on the destination address and whether
 source and destination are set to one of the well-known ports.
 */
#define DN_PAYLOAD_SIZE_LIMIT_MNG_HIGH	90
#define DN_PAYLOAD_SIZE_LIMIT_MNG_MED	88
#define DN_PAYLOAD_SIZE_LIMIT_MNG_LOW	87
#define DN_PAYLOAD_SIZE_LIMIT_IP_HIGH	74
#define DN_PAYLOAD_SIZE_LIMIT_IP_MED	72
#define DN_PAYLOAD_SIZE_LIMIT_IP_LOW	71

/*
 This is not a valid network ID, but will instead cause the QSL to to take the
 mote through a promiscuous listen state to identify the ID of advertising
 networks, further attempting to join the first one found.
 */
#define DN_PROMISCUOUS_NET_ID	0xffff

#define DN_DEFAULT_NET_ID				1229
#define DN_DEFAULT_JOIN_KEY				(uint8_t*)dn_default_joinKey
#define DN_DEFAULT_DEST_PORT			DN_WELL_KNOWN_PORT_1
#define DN_DEFAULT_DEST_IP				(uint8_t*)dn_default_manager_ipv6Addr
#define DN_DEFAULT_SRC_PORT				DN_WELL_KNOWN_PORT_1
#define DN_DEFAULT_PAYLOAD_SIZE_LIMIT	DN_PAYLOAD_SIZE_LIMIT_MNG_HIGH
#define DN_DEFAULT_SERVICE_MS			9000 // Base bandwidth provided by manager

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


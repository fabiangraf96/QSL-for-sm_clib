/*
Copyright (c) 2016, Dust Networks. All rights reserved.

QuickStart Library API.

\license See attached DN_LICENSE.txt.
*/

#ifndef DN_QSL_API_H
#define DN_QSL_API_H

#include "dn_common.h"
#include "dn_defaults.h"
#include "dn_ipmt.h"

//=========================== defines =========================================

#define DN_DEST_IP	DN_DEFAULT_DEST_IP

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

#ifdef __cplusplus
 extern "C" {
#endif

//===== init

/**
 \brief Initialize the necessary structures and underlying C Library.
 
 Has to be run once before any of the other functions below is run.
 
 \return A boolean indicating the success of the initialization.
 */
bool dn_qsl_init(void);


//===== isConnected

/**
 \brief Return TRUE if the mote is connected to a network.
 
 Simply checks if the FSM is in the CONNECTED state.
 
 \return A boolean indicating if the mote is connected to a network.
 */
bool dn_qsl_isConnected(void);


//===== connect

/**
 \brief Attempt to make the mote connect to a network with the given parameters.
 
 If the passed network ID, join key or source port is zero/NULL, default values
 are used. If non-zero, the passed service will be requested after joining a
 network. A new service can be requested later by calling connect again with the
 same network ID, join key and source port. Further, if the requested service is
 also indifferent, the function simply returns TRUE. 
 
 \param netID The ID of the network to attempt to connect with.
 \param joinKey The join key to use in the connection attempt.
 \param srcPort The port that you expect to get downstream data on.
 \param service_ms The service to request after establishing a connection, given in milliseconds.
 \return A boolean indicating if the connection attempt and/or service request was successful.
 */
bool dn_qsl_connect(uint16_t netID, const uint8_t* joinKey, uint16_t srcPort, uint32_t service_ms);


//===== send

/**
 \brief Send a packet into the network.
 
 Tells the mote send a packet with the given payload into the network.
 Said packet is sent to the provided destination port (default if 0) and the
 IPv6 address defined by DEST_IP (default is the well-known manager address).
 The FSM waits for a confirmation from the mote that the packet was accepted
 and queued up for transmission. Note that end-to-end delivery is not
 guaranteed with the utilized UDP, but reliability is typically > 99.9 %.
 
 \param payload Pointer to a byte array containing the payload.
 \param payloadSize_B Byte size of the payload.
 \param destPort The destination port for the packet.
 \return A boolean indicating if the packet was queued up for transmission.
 */
bool dn_qsl_send(const uint8_t* payload, uint8_t payloadSize_B, uint16_t destPort);


//===== read

/**
 \brief Read the FIFO buffered inbox of downstream messages.
 
 As the payload of downstream messages are pushed into a buffered FIFO inbox,
 calling this function will pop the first one (oldest) stored into the provided
 buffer and return the byte size. An empty inbox will simply return 0.
 
 \param readBuffer Pointer to a byte array to store the read message payload.
 \return The number of bytes read into the provided buffer.
 */
uint8_t dn_qsl_read(uint8_t* readBuffer);

//===== getTime

/**
 \brief Get the current time from the mote.
 
 This function will block until the time is received or a timeout occurs.
 
 \param out Pointer to a structure to store the received time information.
 \return A boolean indicating if the time retrieval was successful.
 */
bool dn_qsl_getTime(dn_ipmt_getParameter_time_rpt* out);

//===== setTxPower

/**
 \brief Set the transmission power of the mote.
 
 This function will block until the transmission power is set or a timeout occurs.
 
 \param out Pointer to a structure to store the response information.
 \param txPower The desired transmission power level.
 \return A boolean indicating if the transmission power was set successfully.
 */
bool dn_qsl_setTxPower(dn_ipmt_setParameter_txPower_rpt* out, uint8_t txPower);

#ifdef __cplusplus
}
#endif

#endif
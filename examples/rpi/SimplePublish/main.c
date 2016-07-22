/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: jhbr@datarespons.no
 *
 * Created on 21. juni 2016, 13:09
 */

#include <stdlib.h>

#include "dn_qsl_api.h"
#include "dn_debug.h"
#include "dn_endianness.h"
#include "dn_time.h"

static uint16_t randomWalk(void);
static void parsePayload(const uint8_t *payload, uint8_t size);

/*
 * 
 */
int main(int argc, char** argv)
{
	uint16_t netID = 1230; // TODO: Use default in release
	uint8_t joinKey[DN_JOIN_KEY_LEN] = // TODO: Use default in release
	{
		0x44,0x55,0x53,0x54,0x4E,0x45,0x54,0x57,
		0x4F,0x52,0x4B,0x53,0x52,0x4F,0x43,0x4A
	};
	uint16_t srcPort = 60000;
	uint32_t service_ms = 4000;
	uint16_t destPort = DN_DEFAULT_DEST_PORT;
	uint8_t payload[3];
	uint8_t inboxBuf[DN_DEFAULT_PAYLOAD_SIZE_LIMIT];
	uint8_t bytesRead;
	
	log_info("Initializing...");
	dn_qsl_init(); // Always returns TRUE atm

	while (TRUE)
	{
		if (dn_qsl_isConnected())
		{
			uint16_t val = randomWalk();
			static uint8_t count = 0;
			
			dn_write_uint16_t(payload, val);
			payload[2] = count;
			
			if (dn_qsl_send(payload, sizeof (payload), destPort))
			{
				log_info("Sent message nr %u: %u", count, val);
				count++;
			} else
			{
				log_info("Send failed");
			}

			do
			{
				bytesRead = dn_qsl_read(inboxBuf);
				parsePayload(inboxBuf, bytesRead);
			} while (bytesRead > 0);
			
			dn_sleep_ms(service_ms);
		} else
		{
			if (dn_qsl_connect(netID, joinKey, srcPort, service_ms))
			{
				log_info("Connected to network");
			} else
			{
				log_info("Failed to connect");
			}
		}
	}

	return (EXIT_SUCCESS);
}

static uint16_t randomWalk(void)
{
	static bool first = TRUE;
	static uint16_t lastValue = 0x7fff; // Start in middle of uint16 range
	const int powerLevel = 9001;
	
	// Seed random number generator on first call
	if (first)
	{
		first = FALSE;
		srand(dn_time_ms());
	}
	
	// Random walk within +/- powerLevel
	lastValue += rand() / (RAND_MAX / (2*powerLevel) + 1) - powerLevel;
	return lastValue;
}

static void parsePayload(const uint8_t *payload, uint8_t size)
{
	uint8_t i;
	char msg[size + 1];
	
	if (size == 0)
	{
		// Nothing to parse
		return;
	}
	
	// Parse bytes individually as well as together as a string
	log_info("Received downstream payload of %u bytes:", size);
	for (i = 0; i < size; i++)
	{
		msg[i] = payload[i];
		log_info("\tByte# %03u: %#.2x (%u)", i, payload[i], payload[i]);
	}
	msg[size] = '\0';
	log_info("\tMessage: %s", msg);
}

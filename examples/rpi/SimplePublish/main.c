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

#include "dn_qsl_api.h"		// Only really need this include
#include "dn_debug.h"		// Included to borrow debug macros
#include "dn_endianness.h"	// Included to borrow array copying
#include "dn_time.h"		// Included to borrow sleep function

#define NETID           0       // Factory default value used if zero (1229)
#define JOINKEY         NULL    // Factory default value used if NULL (44 55 53 54 4E 45 54 57 4F 52 4B 53 52 4F 43 4B)
#define BANDWIDTH_MS    5000    // Not changed if zero (default base bandwidth given by manager is 9 s)
#define SRC_PORT        60000   // Default port used if zero (0xf0b8)
#define DEST_PORT       0       // Default port used if zero (0xf0b8)

static uint16_t randomWalk(void);
static void parsePayload(const uint8_t *payload, uint8_t size);

/*
 * 
 */
int main(int argc, char** argv)
{
	uint8_t payload[3];
	uint8_t inboxBuf[DN_DEFAULT_PAYLOAD_SIZE_LIMIT];
	uint8_t bytesRead;
	
	log_info("Initializing...");
	dn_qsl_init(); // Always returns TRUE at the moment

	while (TRUE)
	{
		if (dn_qsl_isConnected())
		{
			uint16_t val = randomWalk();
			static uint8_t count = 0;
			
			dn_write_uint16_t(payload, val);
			payload[2] = count;
			
			if (dn_qsl_send(payload, sizeof (payload), DEST_PORT))
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
			
			dn_sleep_ms(BANDWIDTH_MS);
		} else
		{
			if (dn_qsl_connect(NETID, JOINKEY, SRC_PORT, BANDWIDTH_MS))
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

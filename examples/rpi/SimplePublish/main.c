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

static uint16_t nextValue(void);
static void parsePayload(uint8_t *payload, uint8_t size);

/*
 * 
 */
int main(int argc, char** argv)
{
	uint16_t netID = 1230;
	uint8_t joinKey[DN_JOIN_KEY_LEN] =
	{
		0x44,0x55,0x53,0x54,0x4E,0x45,0x54,0x57,
		0x4F,0x52,0x4B,0x53,0x52,0x4F,0x43,0x4A
	};
	uint32_t service_ms = 9000;
	uint16_t destPort = DN_DEFAULT_DEST_PORT;
	bool success = FALSE;
	uint8_t payload[2];
	uint8_t inboxBuf[DN_DEFAULT_PAYLOAD_SIZE_LIMIT];
	uint8_t readBytes;
	
	log_info("Initializing...");
	dn_qsl_init(); // Always returns TRUE atm

	while (TRUE)
	{
		if (dn_qsl_isConnected())
		{
			uint16_t val = nextValue();
			dn_write_uint16_t(payload, val);
			success = dn_qsl_send(payload, sizeof (val), destPort);
			if (success)
			{
				log_info("Sent message: %u", val);
			} else
			{
				log_info("Send failed");
			}

			do
			{
				readBytes = dn_qsl_read(inboxBuf);
				parsePayload(inboxBuf, readBytes);
			} while (readBytes > 0);
			
			dn_sleep_ms(service_ms);
		} else
		{
			success = dn_qsl_connect(netID, joinKey, 0);//service_ms);
			if (success)
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

static uint16_t nextValue(void)
{
	static uint16_t lastValue = 0x7fff;
	static bool first = TRUE;
	int N = 9001;
	if (first)
	{
		first = FALSE;
		srand(dn_time_ms());
	}
	lastValue += rand() / (RAND_MAX / N + 1) - N/2;
	return lastValue;
}

static void parsePayload(uint8_t *payload, uint8_t size)
{
	uint8_t cmd;
	uint16_t msg;
	if (size == 0)
	{
		// Nothing to parse
		return;
	}
	
	cmd = payload[0];
	if (size == 3)
	{
		dn_read_uint16_t(&msg, &payload[1]);
		log_info("Received downstream data: cmd %#x, msg %#x", cmd, msg);
	} else
	{
		log_warn("Received unexpected data %#x (%u)", cmd, cmd);
	}
}

/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: jhbr
 *
 * Created on 21. juni 2016, 13:09
 */

#include <stdlib.h>

#include "dn_qsl_api.h"
#include "dn_debug.h"
#include "dn_endianness.h"
#include "dn_time.h"

static uint16_t nextValue(void);

/*
 * 
 */
int main(int argc, char** argv)
{
	uint16_t netID = 1230;
	uint8_t joinKey[JOIN_KEY_LEN] =
	{
		0x44,0x55,0x53,0x54,0x4E,0x45,0x54,0x57,
		0x4F,0x52,0x4B,0x53,0x52,0x4F,0x43,0x4A
	};
	uint32_t service_ms = 2000;
	uint16_t destPort = 0xf0b8;
	bool success = FALSE;
	uint8_t payload[2];
	
	log_info("Initializing...");
	dn_qsl_init(); // Always returns TRUE atm

	while (dn_qsl_connect(netID, joinKey, service_ms))
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
		dn_sleep_ms(service_ms);
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


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
	uint32_t service_ms = 5000;
	uint16_t message = 0xabcd;
	uint8_t payload[2];
	bool success = FALSE;
	dn_write_uint16_t(payload, message);
	uint32_t lastPublish;
	
	debug("Initializing...");
	success = dn_qsl_init();
	if (success)
	{
		// Test default connect and sending a message
		debug("Connecting...");
		success = dn_qsl_connect(0, NULL, 0);
		if (success)
		{
			log_info("Connected to network");
			success = dn_qsl_send(payload, sizeof(message), NULL);
			if (success)
			{
				lastPublish = dn_time_ms();
				debug("Sent message: %#x", message);
			} else
			{
				debug("Send failed");
			}
		} else
		{
			log_info("Failed to connect");
		}
		
		// Test connect with new netID and join Key (will fail wo second mng)
		debug("Changing network ID and join key");
		success = dn_qsl_connect(netID, joinKey, 0);
		if (success)
		{
			log_info("Connected to network");
			success = dn_qsl_send(payload, sizeof(message), NULL);
			if (success)
			{
				lastPublish = dn_time_ms();
				debug("Sent message: %#x", message);
			} else
			{
				debug("Send failed");
			}
		} else
		{
			log_info("Failed to connect");
		}
		
		// Test change of service and start sending periodic messages
		debug("Changing service");
		success = dn_qsl_connect(0, NULL, service_ms);
		if (success)
		{
			log_info("Connected to network");
			
			while (dn_qsl_connect(0, NULL, 0))
			{
				success = dn_qsl_send(payload, sizeof (message), NULL);
				if (success)
				{
					lastPublish = dn_time_ms();
					debug("Sent message: %#x", message);
				} else
				{
					debug("Send failed");
				}
				while (lastPublish + service_ms > dn_time_ms())
				{
					usleep((lastPublish + service_ms - dn_time_ms())*1000);
				}
			}
		} else
		{
			log_info("Failed to connect");
		}
	} else
	{
		log_warn("Initialization failed");
	}

	return (EXIT_SUCCESS);
}


/*
Copyright (c) 2016, Dust Networks. All rights reserved.

SimplePublish example application for the SAM C21 Xplained Pro.

\license See attached DN_LICENSE.txt.
*/

#include <asf.h>
#include <stdlib.h>

#include "serial.h"
#include "watchdog_timer.h"
#include "dn_qsl_api.h"		// Only really need this include from QSL
#include "dn_debug.h"		// Included to borrow debug macros
#include "dn_endianness.h"	// Included to borrow array copying
#include "dn_time.h"		// Included to borrow sleep function

#define NETID			0		// Factory default value used if zero (1229)
#define JOINKEY			NULL	// Factory default value used if NULL (44 55 53 54 4E 45 54 57 4F 52 4B 53 52 4F 43 4B)
#define BANDWIDTH_MS	3000	// Not changed if zero (default base bandwidth given by manager is 9 s)
#define SRC_PORT		60000	// Default port used if zero (0xf0b8)
#define DEST_PORT		0		// Default port used if zero (0xf0b8)
#define DATA_PERIOD_MS	3000	// Should be longer than (or equal to) bandwidth

static uint16_t randomWalk(void);
static void parsePayload(const uint8_t *payload, uint8_t size);

int main(int argc, char** argv)
{
	uint8_t payload[3];
	uint8_t inboxBuf[DN_DEFAULT_PAYLOAD_SIZE_LIMIT];
	uint8_t bytesRead;

	// Initialize system clock and board
	system_init();

	// Initialize debug usart, enabling debug prints
	edbg_usart_clock_init();
	edbg_usart_pin_init();
	edbg_usart_init();

	// Initialize watchdog timer
	configure_wdt();

	// Watchdog and usart uses interrupts
	system_interrupt_enable_global();

	log_info("Initializing...");

	dn_qsl_init(); // Always returns TRUE at the moment
	
	while (TRUE)
	{
		wdt_reset_count();
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
				wdt_reset_count();
				bytesRead = dn_qsl_read(inboxBuf);
				parsePayload(inboxBuf, bytesRead);
			} while (bytesRead > 0);
			
			wdt_reset_count();
			dn_sleep_ms(DATA_PERIOD_MS);
		} else
		{
			log_info("Connecting...");
			LED_Off(LED_0_PIN); // Turn off yellow LED
			if (dn_qsl_connect(NETID, JOINKEY, SRC_PORT, BANDWIDTH_MS))
			{
				log_info("Connected to network");
				LED_On(LED_0_PIN); // Turn on yellow LED
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


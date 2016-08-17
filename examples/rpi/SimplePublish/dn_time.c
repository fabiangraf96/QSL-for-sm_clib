/*
Copyright (c) 2016, Dust Networks. All rights reserved.

Port of the lock module to the Raspberry Pi.

\license See attached DN_LICENSE.txt.
*/

/* 
 * File:   dn_time.c
 * Author: jhbr@datarespons.no
 *
 * Created on 23. juni 2016, 12:40
 */

#include <time.h>

#include "dn_time.h"
#include "dn_debug.h"

//=========================== variables =======================================

//=========================== prototypes ======================================

//=========================== public ==========================================

uint32_t dn_time_ms(void)
{
	uint32_t ms = 0;
	struct timespec spec;
	
	if (clock_gettime(CLOCK_MONOTONIC, &spec) == -1)
	{
		log_err("Get time failed");
	} else
	{
		ms = (uint32_t)spec.tv_sec * 1000 + (uint32_t)(spec.tv_nsec / 1.0e6);
	}
	return ms;
}

void dn_sleep_ms(uint32_t milliseconds)
{
	struct timespec remaining;
	
	remaining.tv_sec = (time_t)(milliseconds / 1000);
	remaining.tv_nsec = (milliseconds % 1000) * 1e6;
	
	// Sleep until the requested interval has elapsed
	while (nanosleep(&remaining, &remaining) == -1)
	{
		if (errno == EINTR)
		{
			continue;
		} else
		{
			log_err("Sleep failed");
			return;
		}
	}
}

//=========================== private =========================================

//=========================== helpers =========================================
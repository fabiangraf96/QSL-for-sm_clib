/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <time.h>

#include "dn_time.h"

//=========================== variables =======================================

//=========================== prototypes ======================================

//=========================== public ==========================================

uint32_t dn_time_ms(void)
{
	uint8_t rc;
	uint32_t ms = 0;
	struct timespec spec;
	
	rc = clock_gettime(CLOCK_MONOTONIC, &spec);
	if (rc < 0)
	{
		// Error
	}
	else
	{
		ms = (uint32_t)spec.tv_sec * 1000 + (uint32_t)(spec.tv_nsec / 1.0e6);
	}
	return ms;
}

//=========================== private =========================================

//=========================== helpers =========================================
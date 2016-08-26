/*
Copyright (c) 2016, Dust Networks. All rights reserved.

Port of the time module to the SAM C21 Xplained Pro.

\license See attached DN_LICENSE.txt.
*/

#include "dn_time.h"
#include "stm32l0xx_hal.h"

//=========================== variables =======================================


//=========================== prototypes ======================================


//=========================== public ==========================================

uint32_t dn_time_ms(void)
{
	uint32_t now;
	/*
	static uint32_t dummyTime = 0;
	dummyTime = dummyTime + 5;
	return dummyTime;
	*/
	now = HAL_GetTick();
	return now;
}

void dn_sleep_ms(uint32_t milliseconds)
{
	HAL_Delay(milliseconds);
}

//=========================== private =========================================


//=========================== helpers =========================================

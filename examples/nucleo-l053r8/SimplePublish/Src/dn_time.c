/*
Copyright (c) 2016, Dust Networks. All rights reserved.

Port of the time module to the NUCLEO-L053R8.

\license See attached DN_LICENSE.txt.
*/

#include "dn_time.h"
#include "stm32l0xx_hal.h"

//=========================== variables =======================================


//=========================== prototypes ======================================


//=========================== public ==========================================

uint32_t dn_time_ms(void)
{
	return HAL_GetTick();
}

void dn_sleep_ms(uint32_t milliseconds)
{
	HAL_Delay(milliseconds);
}

//=========================== private =========================================


//=========================== helpers =========================================

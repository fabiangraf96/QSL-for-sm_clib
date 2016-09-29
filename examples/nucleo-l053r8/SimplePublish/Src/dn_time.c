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
	/* A simple delay is used for simplicity in this example.
	 * To save power, we could instead have initialized a timer to fire an
	 * interrupt after the set number of milliseconds, followed by entering
	 * a low-power sleep mode. Upon wake up, we would have to check that we
	 * were indeed woken by said interrupt (and e.g. not an USART interrupt)
	 * to decide if we should go back to sleep or not. */
	HAL_Delay(milliseconds);
}

//=========================== private =========================================


//=========================== helpers =========================================

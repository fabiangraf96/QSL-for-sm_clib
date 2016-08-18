/*
Copyright (c) 2016, Dust Networks. All rights reserved.

\license See attached DN_LICENSE.txt.
*/

#include "dn_time.h"
#include "dn_debug.h"
#include "rtc_count.h"
#include "delay.h"

//=========================== variables =======================================

typedef struct {
	struct rtc_module rtc_instance;
} dn_time_vars_t;

dn_time_vars_t dn_time_vars;

//=========================== prototypes ======================================

static void configure_rtc_count(void);

//=========================== public ==========================================

uint32_t dn_time_ms(void)
{
	static bool rtc_initialized = FALSE;
	if (!rtc_initialized)
	{
		configure_rtc_count();
		rtc_initialized = TRUE;
	}
	return rtc_count_get_count(&dn_time_vars.rtc_instance);
}

void dn_sleep_ms(uint32_t milliseconds)
{
	// TODO: Implement proper sleep?
	delay_ms(milliseconds);
}

//=========================== private =========================================

static void configure_rtc_count(void)
{
	struct rtc_count_config config_rtc_count;

	rtc_count_get_config_defaults(&config_rtc_count);

	config_rtc_count.prescaler           = RTC_COUNT_PRESCALER_DIV_1;
	config_rtc_count.mode                = RTC_COUNT_MODE_32BIT;
	
	rtc_count_init(&dn_time_vars.rtc_instance, RTC, &config_rtc_count);

	rtc_count_enable(&dn_time_vars.rtc_instance);
}

//=========================== helpers =========================================
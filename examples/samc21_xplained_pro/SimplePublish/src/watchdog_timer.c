/*
Copyright (c) 2016, Dust Networks. All rights reserved.

Watchdog timer configuration for the SAM C21 Xplained Pro.

\license See attached DN_LICENSE.txt.
*/

#include "watchdog_timer.h"
#include "dn_debug.h"

void watchdog_warning_callback(void)
{
	log_warn("Watchdog warning: %u ms without food!", WATCHDOG_WARNING_MS);
}

void configure_wdt(void)
{
	struct wdt_conf config_wdt;
	wdt_get_config_defaults(&config_wdt);

	// Set timeout; system will reset if not fed within period
	config_wdt.timeout_period = WATCHDOG_TIMEOUT_CLK;
	// Enable early warning; callback will fire if not fed within period
	config_wdt.early_warning_period = WATCHDOG_WARNING_CLK;

	wdt_set_config(&config_wdt);

	wdt_register_callback(watchdog_warning_callback, WDT_CALLBACK_EARLY_WARNING);
	wdt_enable_callback(WDT_CALLBACK_EARLY_WARNING);
}
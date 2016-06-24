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

/*
 * 
 */
int main(int argc, char** argv)
{	
	bool success = FALSE;
	debug("Initializing...");
	success = dn_qsl_init();
	if (success)
	{
		debug("Connecting...");
		success = dn_qsl_connect(0, NULL, 0);
		if (success)
		{
			//printf("Connected to network\n");
			log_info("Connected to network");
			
		}
		else
		{
			//printf("Failed to connect\n");
			log_info("Failed to connect");
		}
	}
	else
	{
		//printf("Initialization failed\n");
		log_warn("Initialization failed");
	}

	return (EXIT_SUCCESS);
}


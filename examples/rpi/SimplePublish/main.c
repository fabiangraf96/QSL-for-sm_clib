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

#include <stdio.h>
#include <stdlib.h>

#include "dn_fsm.h"
#include "dn_time.h"
#include "dn_qsl_api.h"

/*
 * 
 */
int main(int argc, char** argv)
{	
	uint32_t start_ms = dn_time_ms();
	bool success = FALSE;
	success = dn_qsl_init();
	if (success)
	{
		success = dn_qsl_connect(0, NULL, 0);
		if (success)
		{
			printf("Connected to network\n");
		}
		else
		{
			printf("Failed to connect\n");
		}
	}
	else
	{
		printf("Initialization failed\n");
	}
	
	printf("Runtime: %u\n", dn_time_ms() - start_ms);
	return (EXIT_SUCCESS);
}


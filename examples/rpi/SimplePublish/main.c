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
#include <unistd.h>		//Used for UART
#include <fcntl.h>		//Used for UART
#include <termios.h>	//Used for UART

#include "dn_common.h"
#include "dn_ipmt.h"
#include "dn_uart.h"

void readByte(uint8_t rxbyte);

/*
 * 
 */
int main(int argc, char** argv)
{	
	dn_uart_init(readByte);
	
	dn_uart_txByte('H');
	dn_uart_txByte('e');
	dn_uart_txByte('l');
	dn_uart_txByte('l');
	dn_uart_txByte('o');
	
	sleep(5); // Wait a bit so that read daemon has time to read bytes sent
	
	printf("Hello, world!\n");
	printf("MAX_FRAME_LENGTH: %d\n", MAX_FRAME_LENGTH);
	return (EXIT_SUCCESS);
}

void readByte(uint8_t rxbyte)
{
	printf("Recived byte: %c\n", rxbyte);
}


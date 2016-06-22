/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <unistd.h>		//Used for UART
#include <fcntl.h>		//Used for UART
#include <termios.h>	//Used for UART
#include <stdio.h>
#include <stdlib.h>
#include "dn_uart.h"

//=========================== variables =======================================

typedef struct {
	dn_uart_rxByte_cbt		ipmt_uart_rxByte_cb;
	int32_t					uart_fd;
} dn_uart_vars_t;

dn_uart_vars_t dn_uart_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

void dn_uart_init(dn_uart_rxByte_cbt rxByte_cb)
{
	char *portname = "/dev/ttyUSB0";
	struct termios options;
	
	dn_uart_vars.ipmt_uart_rxByte_cb = rxByte_cb;
	
	dn_uart_vars.uart_fd = -1;
	dn_uart_vars.uart_fd = open(portname, O_RDWR | O_NOCTTY);
	if (dn_uart_vars.uart_fd == -1)
	{
		printf("dn_uart: Unable to open UART\n");
	}
	else
	{
		printf("dn_uart: UART opened\n");
	}

	tcgetattr(dn_uart_vars.uart_fd, &options);
	options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(dn_uart_vars.uart_fd, TCIFLUSH);
	tcsetattr(dn_uart_vars.uart_fd, TCSANOW, &options);
}

void dn_uart_txByte(uint8_t byte)
{
	if (dn_uart_vars.uart_fd == -1)
	{
		printf("dn_uart: txByte error; uart not initialized\n");
		return;
	}
	
	write(dn_uart_vars.uart_fd, &byte, 1);
}

void dn_uart_txFlush(void)
{
	// Nothing to do since POSIX driver is byte-oriented
}

//=========================== private =========================================

//=========================== helpers =========================================

//=========================== interrupt handlers ==============================
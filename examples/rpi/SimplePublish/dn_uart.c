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
#include <pthread.h>

#include "dn_uart.h"
#include "dn_ipmt.h"

//=========================== variables =======================================

typedef struct {
	dn_uart_rxByte_cbt		ipmt_uart_rxByte_cb;
	int32_t					uart_fd;
	pthread_t				read_daemon;
} dn_uart_vars_t;

dn_uart_vars_t dn_uart_vars;


//=========================== prototypes ======================================

static void* dn_uart_read_daemon(void* arg);


//=========================== public ==========================================

void dn_uart_init(dn_uart_rxByte_cbt rxByte_cb)
{
	char *portname = "/dev/ttyUSB0";
	struct termios options;
	int32_t rc;
	
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
	
	rc = pthread_create(&dn_uart_vars.read_daemon, NULL, dn_uart_read_daemon, NULL);
	if (rc != 0)
	{
		printf("dn_uart: Failed to start read daemon!\n");
	}
}

void dn_uart_txByte(uint8_t byte)
{
	if (dn_uart_vars.uart_fd == -1)
	{
		printf("dn_uart: UART not initialized (tx)!\n");
		return;
	}
	
	write(dn_uart_vars.uart_fd, &byte, 1);
}

void dn_uart_txFlush(void)
{
	// Nothing to do since POSIX driver is byte-oriented
}


//=========================== private =========================================

static void* dn_uart_read_daemon(void* arg)
{
	struct timeval timeout;
	int32_t rc;
	fd_set set;
	uint8_t rxBuff[MAX_FRAME_LENGTH];
	int8_t rxBytes = 0;
	uint8_t n = 0;
	
	if (dn_uart_vars.uart_fd == -1)
	{
		printf("dn_uart: UART not initialized (rx)!\n");
		return;
	}	
	
	FD_ZERO(&set);
	FD_SET(dn_uart_vars.uart_fd, &set);
	timeout.tv_sec = 1;
	
	while(TRUE)
	{
		rc = select(dn_uart_vars.uart_fd + 1, &set, NULL, NULL, &timeout);
		if (rc < 0)
		{
			printf("Select UART error\n");
		}
		else if (rc == 0)
		{
			printf("UART read timeout\n");
		}
		else
		{
			rxBytes = read(dn_uart_vars.uart_fd, (void*)rxBuff, MAX_FRAME_LENGTH);
			if (rxBytes > 0)
			{
				for (n = 0; n < rxBytes; n++)
				{
					dn_uart_vars.ipmt_uart_rxByte_cb(rxBuff[n]);
				}
			}
		}
	}
	
	
}


//=========================== helpers =========================================

//=========================== interrupt handlers ==============================
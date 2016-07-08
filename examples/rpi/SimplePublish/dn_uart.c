/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   dn_uart.c
 * Author: jhbr@datarespons.no
 *
 * Created on 22. juni 2016, 10:19
 */

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <pthread.h>

#include "dn_uart.h"
#include "dn_ipmt.h"
#include "dn_debug.h"

//=========================== variables =======================================

typedef struct {
	dn_uart_rxByte_cbt		ipmt_uart_rxByte_cb;
	int32_t					uart_fd;
	pthread_t				read_daemon;
} dn_uart_vars_t;

static dn_uart_vars_t dn_uart_vars;


//=========================== prototypes ======================================

static void* dn_uart_read_daemon(void* arg);


//=========================== public ==========================================

void dn_uart_init(dn_uart_rxByte_cbt rxByte_cb)
{
	char *portname = "/dev/ttyUSB0"; // External USB UART
	//char *portname = "/dev/ttyAMA0"; // On-board UART
	struct termios options;
	int32_t rc;
	
	// Store byte received callback
	dn_uart_vars.ipmt_uart_rxByte_cb = rxByte_cb;
	
	// Open and store UART file descriptor
	dn_uart_vars.uart_fd = -1;
	dn_uart_vars.uart_fd = open(portname, O_RDWR | O_NOCTTY);
	if (dn_uart_vars.uart_fd == -1)
	{
		log_err("Unable to open UART %s", portname);
	} else
	{
		debug("UART opened");
	}

	// Get current options and configure flags below
	tcgetattr(dn_uart_vars.uart_fd, &options);
	
	// Control flags: 115.2K baud | 8 bit char size mask | Ignore modem control lines | Enable receiver
	options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
	// Input flags: Ignore framing errors and parity errors
	options.c_iflag = IGNPAR;
	// Output flags: None
	options.c_oflag = 0;
	// Local flags: None
	options.c_lflag = 0;
	
	// Discard any data written and set new options
	tcflush(dn_uart_vars.uart_fd, TCIFLUSH);
	tcsetattr(dn_uart_vars.uart_fd, TCSANOW, &options);
	
	// Start read daemon to listen for UART RX
	rc = pthread_create(&dn_uart_vars.read_daemon, NULL, dn_uart_read_daemon, NULL);
	if (rc != 0)
	{
		log_err("Failed to start read daemon");
	}
}

void dn_uart_txByte(uint8_t byte)
{
	int rc;
	if (dn_uart_vars.uart_fd == -1)
	{
		log_err("UART not initialized (tx)");
		return;
	}
	
	rc = write(dn_uart_vars.uart_fd, &byte, 1);
	if (rc < 0)
	{
		log_warn("Write to UART failed");
	} else if (rc == 0)
	{
		// Nothing was sent
		log_warn("Write to UART sent nothing");
	} else
	{
		//debug("dn_uart: Sent a byte");
	}
}

void dn_uart_txFlush(void)
{
	// Nothing to do since POSIX driver is byte-oriented
}


//=========================== private =========================================

static void* dn_uart_read_daemon(void* arg)
{
	struct timeval timeout;
	int rc;
	fd_set set;
	uint8_t rxBuff[MAX_FRAME_LENGTH];
	int8_t rxBytes = 0;
	uint8_t n = 0;
	
	if (dn_uart_vars.uart_fd == -1)
	{
		log_err("UART not initialized (rx)");
		return NULL;
	}
	debug("Read daemon started");	
	
	while(TRUE)
	{
		// Add socket to set
		FD_ZERO(&set);
		FD_SET(dn_uart_vars.uart_fd, &set);
		// Set select timeout to 0.5 seconds (TODO: Any reason not to set higher?)
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;
		rc = select(dn_uart_vars.uart_fd + 1, &set, NULL, NULL, &timeout);
		if (rc < 0)
		{
			log_warn("Read daemon select error");
		} else if (rc == 0)
		{
			// Timeout
		} else
		{
			// There are Bytes ready to be read
			rxBytes = read(dn_uart_vars.uart_fd, rxBuff, MAX_FRAME_LENGTH);
			if (rxBytes < 0)
			{
				log_warn("Read from UART failed");
			} else if (rxBytes > 0)
			{
				debug("Received %d bytes", rxBytes);
				for (n = 0; n < rxBytes; n++)
				{
					// Push individual byte to HDLC layer
					dn_uart_vars.ipmt_uart_rxByte_cb(rxBuff[n]);
				}
			}
		}
	}
	
	
}


//=========================== helpers =========================================

//=========================== interrupt handlers ==============================
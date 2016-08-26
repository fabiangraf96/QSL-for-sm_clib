/*
Copyright (c) 2016, Dust Networks. All rights reserved.

Port of the uart module to the SAM C21 Xplained Pro.
Setup and initialization found in serial.c.

\license See attached DN_LICENSE.txt.
*/

#include "dn_uart.h"
#include "dn_ipmt.h"
#include "dn_debug.h"
#include "usart.h"


//=========================== defines =========================================
#define TX_TIMEOUT_MS	1

//=========================== variables =======================================

typedef struct {
	dn_uart_rxByte_cbt	ipmt_uart_rxByte_cb;
} dn_uart_vars_t;

dn_uart_vars_t dn_uart_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

void dn_uart_init(dn_uart_rxByte_cbt rxByte_cb)
{
	// Store RX callback function
	dn_uart_vars.ipmt_uart_rxByte_cb = rxByte_cb;

	// USART1 initialized in main by CubeMX auto-generated code

	// Enable Read Data Register Not Empty interrupt
	__HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);

	debug("SMIP Serial Initialized");
}

void dn_uart_txByte(uint8_t byte)
{
	HAL_UART_Transmit(&huart1, &byte, 1, TX_TIMEOUT_MS);
}

void dn_uart_txFlush()
{
	// Nothing to do since we push byte-by-byte
}

//=========================== private =========================================

//=========================== helpers =========================================

//=========================== interrupt handlers ==============================

void USART_SMIP_Interrupt(UART_HandleTypeDef *huart)
{
	uint8_t rbyte;

	// Check that the Read Data Register Not Empty interrupt is enabled and has occurred
	if((__HAL_UART_GET_IT(huart, UART_IT_RXNE) != RESET) && (__HAL_UART_GET_IT_SOURCE(huart, UART_IT_RXNE) != RESET))
	{
		// Get byte from Read Data Register
		rbyte = (uint8_t)(huart->Instance->RDR & (uint8_t)0xff);
		// Flush Read Data Register to prevent overflow
		__HAL_UART_SEND_REQ(huart, UART_RXDATA_FLUSH_REQUEST);
		// Push byte to HDLC layer
		dn_uart_vars.ipmt_uart_rxByte_cb(rbyte);
	}
}

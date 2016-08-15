/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   dn_uart.c
 * Author: jhbr@datarespons.no
 *
 * Created on 9. aug 2016, 10:30
 */

#include "dn_uart.h"
#include "dn_ipmt.h"
#include "dn_debug.h"
#include <asf.h>
#include "serial.h"


//=========================== defines =========================================

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

	// Initialize external USART (SERCOM3)
	ext_usart_clock_init();
	ext_usart_pin_init();
	ext_usart_init();

	debug("SMIP Serial Initialized");
}

void dn_uart_txByte(uint8_t byte)
{
	// Wait until Data Register Empty flag is set
	while (!SERCOM3->USART.INTFLAG.bit.DRE);
	// Write byte into DATA register, causing it to be moved to the shift register and transmitted
	SERCOM3->USART.DATA.reg = byte;
}

void dn_uart_txFlush()
{
	// Nothing to do since we push byte-by-byte
}


//=========================== private =========================================

//=========================== helpers =========================================

//=========================== interrupt handlers ==============================

void SERCOM3_Handler()
{
	// All interrupts for one peripheral are ORed together; check that it was RX completed
	if (SERCOM3->USART.INTFLAG.bit.RXC){
		//port_pin_toggle_output_level(LED_0_PIN);
		// Push received byte to HDLC layer
		dn_uart_vars.ipmt_uart_rxByte_cb(SERCOM3->USART.DATA.reg);
	}
}
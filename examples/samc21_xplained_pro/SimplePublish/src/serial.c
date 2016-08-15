/*
 * debug_usart.c
 *
 * Created: 10.08.2016 14:45:47
 *  Author: jhbr
 */ 

#include <asf.h>
#include "serial.h"

#define USART_DEBUG_BAUD_RATE	115200
#define USART_SMIP_BAUD_RATE	115200
#define USART_SAMPLE_NUM		16
#define SHIFT					32

static inline void pin_set_peripheral_function(uint32_t pinmux);
static uint64_t long_division(uint64_t n, uint64_t d);
static uint16_t calculate_baud_value(const uint32_t baudrate,const uint32_t peripheral_clock, uint8_t sample_num);

/* EDBG UART(SERCOM4) UART bus and generic clock initialization */
void edbg_usart_clock_init(void)
{
	struct system_gclk_chan_config gclk_chan_conf;
	uint32_t gclk_index = SERCOM4_GCLK_ID_CORE;
	// Turn on module in PM
	system_apb_clock_set_mask(SYSTEM_CLOCK_APB_APBC, 0x20);//PM_APBCMASK_SERCOM4);
	// Get and set default values
	system_gclk_chan_get_config_defaults(&gclk_chan_conf);
	system_gclk_chan_set_config(gclk_index, &gclk_chan_conf);
	// Enable the generic clock
	system_gclk_chan_enable(gclk_index);
}/* EDBG UART(SERCOM4) pin initialization */
void edbg_usart_pin_init(void)
{
	// PB10 and PB11 set into peripheral function D
	pin_set_peripheral_function(PINMUX_PB10D_SERCOM4_PAD2);	pin_set_peripheral_function(PINMUX_PB11D_SERCOM4_PAD3);
}/* EDBG UART(SERCOM4) UART initialization */
void edbg_usart_init(void)
{
	uint16_t baud_value;
	
	// Configure Control A register
	SERCOM4->USART.CTRLA.reg =
		SERCOM_USART_CTRLA_MODE(0x1)	// Use internal clock
		| SERCOM_USART_CTRLA_DORD		// LSB first
		| SERCOM_USART_CTRLA_RXPO(0x3)	// PAD[3] used as RX
		| SERCOM_USART_CTRLA_TXPO(0x1)	// PAD[2] used as TX
		| SERCOM_USART_CTRLA_SAMPR(0x0)	// 16x over-sampling
		| SERCOM_USART_CTRLA_RUNSTDBY;	// Run during standby
	
	// Configure baud rate
	baud_value = calculate_baud_value(USART_DEBUG_BAUD_RATE, system_gclk_chan_get_hz(SERCOM4_GCLK_ID_CORE), USART_SAMPLE_NUM);
	SERCOM4->USART.BAUD.reg = baud_value;
	
	// Configure Control B register
	SERCOM4->USART.CTRLB.reg =
		SERCOM_USART_CTRLB_CHSIZE(0x0)	// 8 bit character size
		| SERCOM_USART_CTRLB_TXEN		// Enable transmitter
		| SERCOM_USART_CTRLB_RXEN;		// Enable receiver
	
	// Make sure synchronization is complete before enabling USART
	while(SERCOM4->USART.SYNCBUSY.bit.CTRLB);
	
	// Enable the USART and wait for it to complete synchronization
	SERCOM4->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
	while(SERCOM4->USART.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_ENABLE);
}

/* External connector(SERCOM3) UART bus and generic clock initialization */
void ext_usart_clock_init(void)
{
	struct system_gclk_chan_config gclk_chan_conf;
	uint32_t gclk_index = SERCOM3_GCLK_ID_CORE;
	// Turn on module in PM
	system_apb_clock_set_mask(SYSTEM_CLOCK_APB_APBC, 0x10);//PM_APBCMASK_SERCOM3);
	// Get and set default values
	system_gclk_chan_get_config_defaults(&gclk_chan_conf);
	system_gclk_chan_set_config(gclk_index, &gclk_chan_conf);
	// Enable the generic clock
	system_gclk_chan_enable(gclk_index);
}/* External connector(SERCOM3) pin initialization */
void ext_usart_pin_init(void)
{
	// PA22 and PA23 set into peripheral function C
	pin_set_peripheral_function(PINMUX_PA22C_SERCOM3_PAD0);	pin_set_peripheral_function(PINMUX_PA23C_SERCOM3_PAD1);
}// External connector(SERCOM3) UART initialization
void ext_usart_init(void)
{
	uint16_t baud_value;
	
	// Configure Control A register
	SERCOM3->USART.CTRLA.reg =
		SERCOM_USART_CTRLA_MODE(0x1)	// Use internal clock
		| SERCOM_USART_CTRLA_DORD		// LSB first
		| SERCOM_USART_CTRLA_RXPO(0x1)	// PAD[1] used as RX
		| SERCOM_USART_CTRLA_TXPO(0x0)	// PAD[0] used as TX
		| SERCOM_USART_CTRLA_SAMPR(0x0)	// 16x over-sampling
		| SERCOM_USART_CTRLA_RUNSTDBY;	// Run during standby
	
	// Configure baud rate
	baud_value = calculate_baud_value(USART_SMIP_BAUD_RATE, system_gclk_chan_get_hz(SERCOM3_GCLK_ID_CORE), USART_SAMPLE_NUM);
	SERCOM3->USART.BAUD.reg = baud_value;
	
	// Configure Control B register
	SERCOM3->USART.CTRLB.reg =
		SERCOM_USART_CTRLB_CHSIZE(0x0)	// 8 bit character size
		| SERCOM_USART_CTRLB_TXEN		// Enable transmitter
		| SERCOM_USART_CTRLB_RXEN;		// Enable receiver
	
	// Make sure synchronization is complete before enabling USART
	while(SERCOM3->USART.SYNCBUSY.bit.CTRLB);
	
	// Enable the USART and wait for it to complete synchronization
	SERCOM3->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
	while(SERCOM3->USART.SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_ENABLE);

	// Enable NVIC line
	system_interrupt_enable(SERCOM3_IRQn);
	// Enable RX complete interrupt
	SERCOM3->USART.INTENSET.reg = SERCOM_USART_INTFLAG_RXC;
}


/* Assigning pin to the alternate peripheral function */
static inline void pin_set_peripheral_function(uint32_t pinmux)
{
	uint8_t port = (uint8_t)((pinmux >> 16)/32);
	PORT->Group[port].PINCFG[((pinmux >> 16) - (port*32))].bit.PMUXEN = 1;
	PORT->Group[port].PMUX[((pinmux >> 16) - (port*32))/2].reg &= ~(0xF << (4 * ((pinmux >>	16) & 0x01u)));
	PORT->Group[port].PMUX[((pinmux >> 16) - (port*32))/2].reg |= (uint8_t)((pinmux & 0x0000FFFF) << (4 * ((pinmux >> 16) & 0x01u)));
}

/*
* internal Calculate 64 bit division, ref can be found in
* http://en.wikipedia.org/wiki/Division_algorithm#Long_division
*/
static uint64_t long_division(uint64_t n, uint64_t d)
{
	int32_t i;
	uint64_t q = 0, r = 0, bit_shift;
	for (i = 63; i >= 0; i--) {
		bit_shift = (uint64_t)1 << i;
		r = r << 1;
		if (n & bit_shift) {
			r |= 0x01;
		}		if (r >= d) {
			r = r - d;
			q |= bit_shift;
		}
	}
	return q;
}/*
* \internal Calculate asynchronous baudrate value (UART)
*/
static uint16_t calculate_baud_value(const uint32_t baudrate, const uint32_t peripheral_clock, uint8_t sample_num)
{
	// Temporary variables
	uint64_t ratio = 0;
	uint64_t scale = 0;
	uint64_t baud_calculated = 0;
	uint64_t temp1;
	// Calculate the BAUD value
	temp1 = ((sample_num * (uint64_t)baudrate) << SHIFT);
	ratio = long_division(temp1, peripheral_clock);
	scale = ((uint64_t)1 << SHIFT) - ratio;
	baud_calculated = (65536 * scale) >> SHIFT;
	return baud_calculated;
}


/* Implementation of _write for stdio */
int __attribute__((weak))
_write (int file, char * ptr, int len);

int __attribute__((weak))
_write (int file, char * ptr, int len)
{
	int nChars = 0;

	if ((file != 1) && (file != 2) && (file!=3)) {
		return -1;
	}

	for (; len != 0; --len) {
		// Wait until Data Register Empty flag is set
		while (!SERCOM4->USART.INTFLAG.bit.DRE);
		// Write byte into DATA register, causing it to be moved to the shift register and transmitted
		SERCOM4->USART.DATA.reg = *ptr++;
		++nChars;
	}
	return nChars;
}


/* Implementation of _read for stdio */
int __attribute__((weak))
_read (int file, char * ptr, int len);

int __attribute__((weak))
_read (int file, char * ptr, int len)
{
	int nChars = 0;

	if (file != 0) {
		return -1;
	}

	for (; len > 0; --len) {
		// Wait for RX complete flag
		while (!SERCOM4->USART.INTFLAG.bit.RXC);
		// Read DATA register
		*ptr = SERCOM4->USART.DATA.reg;
		ptr++;
		nChars++;
	}
	return nChars;
}
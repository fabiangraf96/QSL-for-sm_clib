/*
 * debug_usart.h
 *
 * Created: 10.08.2016 14:46:04
 *  Author: jhbr
 */ 


#ifndef DEBUG_USART_H_
#define DEBUG_USART_H_

#include "dn_common.h"

#ifdef __cplusplus
extern "C" {
#endif

void edbg_usart_clock_init(void);
void edbg_usart_pin_init(void);
void edbg_usart_init(void);

void ext_usart_clock_init(void);
void ext_usart_pin_init(void);
void ext_usart_init(void);

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_USART_H_ */
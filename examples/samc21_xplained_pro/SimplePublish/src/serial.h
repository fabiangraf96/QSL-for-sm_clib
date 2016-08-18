/*
Copyright (c) 2016, Dust Networks. All rights reserved.

Serial (SERCOM) configuration for the SAM C21 Xplained Pro.

\license See attached DN_LICENSE.txt.
*/


#ifndef DEBUG_USART_H_
#define DEBUG_USART_H_

#include "dn_common.h"

#define USART_DEBUG_BAUD_RATE	115200
#define USART_SMIP_BAUD_RATE	115200
#define USART_SAMPLE_NUM		16
#define SHIFT					32

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
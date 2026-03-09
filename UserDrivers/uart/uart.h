#ifndef USERDRIVERS_UART_UART_H_
#define USERDRIVERS_UART_UART_H_

#include "def.h"

bool UART3_DMA_init(uint32_t baud);
uint32_t uart_Available(uint8_t ch);
uint8_t uart_Read(uint8_t ch);
void uart_Write(uint8_t ch, const uint8_t *p_data, uint32_t length);

#endif /* USERDRIVERS_UART_UART_H_ */

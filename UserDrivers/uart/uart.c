#include "uart.h"
#include "stm32u3xx_hal.h"
#include <string.h>

#ifndef UART_RX_BUF_SIZE
#define UART_RX_BUF_SIZE 2048u
#endif

extern UART_HandleTypeDef huart3;

static volatile uint8_t  rx_buf[UART_RX_BUF_SIZE];
static volatile uint32_t rx_head = 0;
static volatile uint32_t rx_tail = 0;
static uint8_t rx_byte;

static inline uint32_t rb_count(void)
{
  uint32_t head = rx_head;
  uint32_t tail = rx_tail;
  if (head >= tail) return head - tail;
  return UART_RX_BUF_SIZE - (tail - head);
}

static inline void rb_push(uint8_t b)
{
  uint32_t next = (rx_head + 1u) % UART_RX_BUF_SIZE;
  if (next != rx_tail)
  {
    rx_buf[rx_head] = b;
    rx_head = next;
  }
}

static inline uint8_t rb_pop(void)
{
  if (rx_head == rx_tail) return 0;
  uint8_t b = rx_buf[rx_tail];
  rx_tail = (rx_tail + 1u) % UART_RX_BUF_SIZE;
  return b;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART3)
  {
    rb_push(rx_byte);
    HAL_UART_Receive_IT(&huart3, &rx_byte, 1);
  }
}

bool UART3_DMA_init(uint32_t baud)
{
  // USART3 is already initialized by MX; we just adjust baudrate and start IT RX
  huart3.Init.BaudRate = baud;
  if (HAL_UART_DeInit(&huart3) != HAL_OK) return false;
  if (HAL_UART_Init(&huart3) != HAL_OK) return false;

  rx_head = rx_tail = 0;
  if (HAL_UART_Receive_IT(&huart3, &rx_byte, 1) != HAL_OK) return false;
  return true;
}

uint32_t uart_Available(uint8_t ch)
{
  (void)ch; // only USART3 supported
  return rb_count();
}

uint8_t uart_Read(uint8_t ch)
{
  (void)ch;
  return rb_pop();
}

void uart_Write(uint8_t ch, const uint8_t *p_data, uint32_t length)
{
  (void)ch;
  if (p_data == NULL || length == 0) return;
  HAL_UART_Transmit(&huart3, (uint8_t*)p_data, length, 1000);
}

#ifndef USERDRIVERS_CMD_CMD_H_
#define USERDRIVERS_CMD_CMD_H_

#include "def.h"
#include "uart.h"

#define CMD_STX                     0x02
#define CMD_ETX                     0x03

#define CMD_DIR_M_TO_S              0x00
#define CMD_DIR_S_TO_M              0x01

#define CMD_OK                      0x00

#define CMD_STATE_WAIT_STX          0
#define CMD_STATE_WAIT_CMD          1
#define CMD_STATE_WAIT_DIR          2
#define CMD_STATE_WAIT_ERROR        3
#define CMD_STATE_WAIT_LENGTH_L     4
#define CMD_STATE_WAIT_LENGTH_H     5
#define CMD_STATE_WAIT_DATA         6
#define CMD_STATE_WAIT_CHECKSUM     7
#define CMD_STATE_WAIT_ETX          8

#ifndef CMD_MAX_DATA
#define CMD_MAX_DATA                4096
#endif

typedef struct
{
  uint8_t  cmd;
  uint8_t  dir;
  uint8_t  error;
  uint16_t length;
  uint8_t  check_sum;
  uint8_t  check_sum_recv;

  uint8_t  buffer[CMD_MAX_DATA + 16];
  uint8_t *data;
} cmd_packet_t;

typedef struct
{
  uint8_t   ch;
  bool      is_init;
  uint32_t  baud;
  uint8_t   state;
  uint32_t  pre_time;
  uint32_t  index;
  uint8_t   error;

  cmd_packet_t  rx_packet;
  cmd_packet_t  tx_packet;
} cmd_t;

static inline uint32_t Millis(void) { return HAL_GetTick(); }

void cmd_Init(cmd_t *p_cmd);
bool cmd_Open(cmd_t *p_cmd, uint8_t ch, uint32_t baud);
bool cmd_Close(cmd_t *p_cmd);
bool cmd_ReceivePacket(cmd_t *p_cmd);
void cmd_SendCmd(cmd_t *p_cmd, uint8_t cmd, uint8_t *p_data, uint32_t length);
void cmd_SendResp(cmd_t *p_cmd, uint8_t cmd, uint8_t err_code, uint8_t *p_data, uint32_t length);
bool cmd_SendCmdRxResp(cmd_t *p_cmd, uint8_t cmd, uint8_t *p_data, uint32_t length, uint32_t timeout);

#endif /* USERDRIVERS_CMD_CMD_H_ */

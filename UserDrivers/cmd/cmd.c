#include "cmd.h"

void cmd_Init(cmd_t *p_cmd)
{
  p_cmd->is_init = false;
  p_cmd->state = CMD_STATE_WAIT_STX;

  p_cmd->rx_packet.data = &p_cmd->rx_packet.buffer[CMD_STATE_WAIT_DATA];
  p_cmd->tx_packet.data = &p_cmd->tx_packet.buffer[CMD_STATE_WAIT_DATA];
}

bool cmd_Open(cmd_t *p_cmd, uint8_t ch, uint32_t baud)
{
  p_cmd->ch = ch;
  p_cmd->baud = baud;
  p_cmd->is_init = true;
  p_cmd->state = CMD_STATE_WAIT_STX;
  p_cmd->pre_time = Millis();

  return UART3_DMA_init(baud);
}

bool cmd_Close(cmd_t *p_cmd)
{
  (void)p_cmd;
  return false;
}

bool cmd_ReceivePacket(cmd_t *p_cmd)
{
  bool result = false;
  uint8_t rx_data;

  if(uart_Available(p_cmd->ch))
  {
    rx_data = uart_Read(p_cmd->ch);
  }
  else
  {
    return false;
  }

  if((Millis() - p_cmd->pre_time) >= 1000)
    p_cmd->state = CMD_STATE_WAIT_STX;

  p_cmd->pre_time = Millis();
  switch(p_cmd->state)
  {
    case CMD_STATE_WAIT_STX:
      if (rx_data == CMD_STX)
      {
        p_cmd->state = CMD_STATE_WAIT_CMD;
        p_cmd->rx_packet.check_sum = 0;
      }
      break;
    case CMD_STATE_WAIT_CMD:
      p_cmd->rx_packet.cmd = rx_data;
      p_cmd->rx_packet.check_sum ^= rx_data;
      p_cmd->state = CMD_STATE_WAIT_DIR;
      break;
    case CMD_STATE_WAIT_DIR:
      p_cmd->rx_packet.dir = rx_data;
      p_cmd->rx_packet.check_sum ^= rx_data;
      p_cmd->state = CMD_STATE_WAIT_ERROR;
      break;
    case CMD_STATE_WAIT_ERROR:
      p_cmd->rx_packet.error = rx_data;
      p_cmd->rx_packet.check_sum ^= rx_data;
      p_cmd->state = CMD_STATE_WAIT_LENGTH_L;
      break;
    case CMD_STATE_WAIT_LENGTH_L:
      p_cmd->rx_packet.length = rx_data;
      p_cmd->rx_packet.check_sum ^= rx_data;
      p_cmd->state = CMD_STATE_WAIT_LENGTH_H;
      break;
    case CMD_STATE_WAIT_LENGTH_H:
      p_cmd->rx_packet.length |= (rx_data << 8);
      p_cmd->rx_packet.check_sum ^= rx_data;

      if (p_cmd->rx_packet.length > 0)
      {
        p_cmd->index = 0;
        p_cmd->state = CMD_STATE_WAIT_DATA;
      }
      else
        p_cmd->state = CMD_STATE_WAIT_CHECKSUM;
      break;
    case CMD_STATE_WAIT_DATA:
      if (p_cmd->index < CMD_MAX_DATA)
      {
        p_cmd->rx_packet.data[p_cmd->index] = rx_data;
        p_cmd->rx_packet.check_sum ^= rx_data;
        p_cmd->index++;
        if (p_cmd->index == p_cmd->rx_packet.length)
          p_cmd->state = CMD_STATE_WAIT_CHECKSUM;
      }
      else
      {
        p_cmd->state = CMD_STATE_WAIT_STX;
      }
      break;
    case CMD_STATE_WAIT_CHECKSUM:
      p_cmd->rx_packet.check_sum_recv = rx_data;
      p_cmd->state = CMD_STATE_WAIT_ETX;
      break;
    case CMD_STATE_WAIT_ETX:
      if (rx_data == CMD_ETX)
      {
        if (p_cmd->rx_packet.check_sum == p_cmd->rx_packet.check_sum_recv)
        {
          result = true;
        }
      }
      p_cmd->state = CMD_STATE_WAIT_STX;
      break;
    }

  return result;
}

void cmd_SendCmd(cmd_t *p_cmd, uint8_t cmd, uint8_t *p_data, uint32_t length)
{
  uint32_t index = 0;

  p_cmd->tx_packet.buffer[index++] = CMD_STX;
  p_cmd->tx_packet.buffer[index++] = cmd;
  p_cmd->tx_packet.buffer[index++] = CMD_DIR_M_TO_S;
  p_cmd->tx_packet.buffer[index++] = CMD_OK;
  p_cmd->tx_packet.buffer[index++] = length & 0xFF;
  p_cmd->tx_packet.buffer[index++] = (length >> 8) & 0xFF;

  for (uint16_t i = 0; i < length; i++)
  {
    p_cmd->tx_packet.buffer[index++] = p_data[i];
  }

  uint8_t check_sum = 0;

  for (uint16_t i = 0; i < (length + 5); i++)
  {
    check_sum ^= p_cmd->tx_packet.buffer[i+1];
  }
  p_cmd->tx_packet.buffer[index++] = check_sum;
  p_cmd->tx_packet.buffer[index++] = CMD_ETX;

  uart_Write(p_cmd->ch, p_cmd->tx_packet.buffer, index);
}

void cmd_SendResp(cmd_t *p_cmd, uint8_t cmd, uint8_t err_code, uint8_t *p_data, uint32_t length)
{
  uint32_t index = 0;

  p_cmd->tx_packet.buffer[index++] = CMD_STX;
  p_cmd->tx_packet.buffer[index++] = cmd;
  p_cmd->tx_packet.buffer[index++] = CMD_DIR_S_TO_M;
  p_cmd->tx_packet.buffer[index++] = err_code;
  p_cmd->tx_packet.buffer[index++] = length & 0xFF;
  p_cmd->tx_packet.buffer[index++] = (length >> 8) & 0xFF;

  for (uint16_t i = 0; i < length; i++)
  {
    p_cmd->tx_packet.buffer[index++] = p_data[i];
  }

  uint8_t check_sum = 0;

  for (uint16_t i = 0; i < length + 5; i++)
  {
    check_sum ^= p_cmd->tx_packet.buffer[i + 1];
  }
  p_cmd->tx_packet.buffer[index++] = check_sum;
  p_cmd->tx_packet.buffer[index++] = CMD_ETX;

  uart_Write(p_cmd->ch, p_cmd->tx_packet.buffer, index);
}

bool cmd_SendCmdRxResp(cmd_t *p_cmd, uint8_t cmd, uint8_t *p_data, uint32_t length, uint32_t timeout)
{
  bool  result = false;

  cmd_SendCmd(p_cmd, cmd, p_data, length);

  uint32_t pre_time = Millis();

  while(1)
  {
    if (cmd_ReceivePacket(p_cmd) == true)
    {
      result = true;
      break;
    }

    if (Millis() - pre_time >= timeout)
      break;
  }

  return result;
}

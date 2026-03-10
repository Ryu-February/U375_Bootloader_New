#include "boot_cmd.h"
#include "boot_update.h"
#include "flash_if.h"
#include "iwdg.h"
#include "led.h"
#include "boot_config.h"
#include "backup.h"
#include "boot_mode.h"
#include <string.h>
#include "flash_if.h"

static cmd_t s_cmd;

static inline uint32_t u32_le(const uint8_t *p)
{
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline uint16_t u16_le(const uint8_t *p)
{
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static inline uint32_t align_down(uint32_t v, uint32_t a){ return v & ~(a - 1U); }
static inline uint32_t align_up  (uint32_t v, uint32_t a){ return (v + a - 1U) & ~(a - 1U); }
static inline bool is_aligned(uint32_t v, uint32_t a) { return (v & (a - 1U)) == 0U; }

static uint8_t boot_IsFlashRange(uint32_t addr_start, uint32_t length)
{
  uint8_t result = false;
  uint32_t addr_end = addr_start + length - 1U;
  uint32_t flash_start = FLASH_ADDR_START;
  uint32_t flash_end = FLASH_ADDR_END;

  if ((addr_start >= flash_start) && (addr_start < flash_end)
      && (addr_end >= flash_start) && (addr_end < flash_end))
  {
    result = true;
  }
  return result;
}

static void bootCmdFlashErase(cmd_t *p_cmd)
{
  uint8_t err_code = CMD_OK;
  cmd_packet_t *p_packet = &p_cmd->rx_packet;

//  if (p_packet->length < 8U)
//  {
//    err_code = BOOT_ERR_WRONG_RANGE;
//    cmd_SendResp(p_cmd, BOOT_CMD_FLASH_ERASE, err_code, NULL, 0);
//    return;
//  }

  uint32_t addr   = u32_le(&p_packet->data[0]);
  uint32_t length = u32_le(&p_packet->data[4]);

  uint32_t start = align_down(addr, FLASH_PAGE_SIZE);
  uint32_t end   = align_up(addr + length, FLASH_PAGE_SIZE);
  uint32_t span  = end - start;

  if (boot_IsFlashRange(start, span) == true)
  {
    if (flash_Erase_New(start, span) != true)
    {
      err_code = BOOT_ERR_FLASH_ERASE;
    }
  }
  else
  {
    err_code = BOOT_ERR_WRONG_RANGE;
  }

  cmd_SendResp(p_cmd, BOOT_CMD_FLASH_ERASE, err_code, NULL, 0);
}

void boot_cmd_init(void)
{
  cmd_Init(&s_cmd);
  cmd_Open(&s_cmd, 3, 115200);
  flash_init();
  boot_update_init(FLASH_ADDR_FW, (uint32_t)(FLASH_ADDR_END - FLASH_ADDR_FW));
}

static void bootCmdFlashWrite(cmd_t *p_cmd)
{
  uint8_t err_code = CMD_OK;
  cmd_packet_t *p_packet = &p_cmd->rx_packet;

//  if (p_packet->length < 8U)
//  {
//    err_code = BOOT_ERR_WRONG_RANGE;
//    cmd_SendResp(p_cmd, BOOT_CMD_FLASH_WRITE, err_code, NULL, 0);
//    return;
//  }

  uint32_t addr   = u32_le(&p_packet->data[0]);
  uint32_t length = u32_le(&p_packet->data[4]);

//  if ((uint32_t)p_packet->length < (8U + length))
//  {
//    err_code = BOOT_ERR_BUF_OVF;
//    cmd_SendResp(p_cmd, BOOT_CMD_FLASH_WRITE, err_code, NULL, 0);
//    return;
//  }

  if (length == 0U)
  {
    err_code = BOOT_ERR_WRONG_RANGE;
  }
  else if (boot_IsFlashRange(addr, length) == true)
  {
    if (!is_aligned(addr, FLASH_PROG_ALIGN) || ((length % FLASH_PROG_ALIGN) != 0U))
    {
      err_code = BOOT_ERR_WRONG_RANGE;
    }
    else
    {
      if (flash_Write_New(addr, &p_packet->data[8], length) != true)
      {
        err_code = BOOT_ERR_FLASH_WRITE;
      }
    }
  }
  else
  {
    err_code = BOOT_ERR_WRONG_RANGE;
  }

  cmd_SendResp(p_cmd, BOOT_CMD_FLASH_WRITE, err_code, NULL, 0);
}

static void handle_packet(cmd_t *pcmd)
{
	uint8_t err = CMD_OK;

	switch (pcmd->rx_packet.cmd)
	{
		case BOOT_CMD_READ_BOOT_VERSION:
		{
		  uint8_t resp[32]; memset(resp, 0, sizeof(resp));
		  const char *s = "Bootloader - V1.0";
		  strncpy((char*)resp, s, sizeof(resp)-1);
		  cmd_SendResp(pcmd, BOOT_CMD_READ_BOOT_VERSION, CMD_OK, resp, 32);
		} break;

		case BOOT_CMD_READ_BOOT_NAME:
		{
		  uint8_t resp[32]; memset(resp, 0, sizeof(resp));
		  const char *s = "KIBO - Bootloader";
		  strncpy((char*)resp, s, sizeof(resp)-1);
		  cmd_SendResp(pcmd, BOOT_CMD_READ_BOOT_NAME, CMD_OK, resp, 32);
		} break;

		case BOOT_CMD_READ_FIRM_VERSION:
		{
		  uint8_t resp[32];
		  memcpy(resp, (const void*)FLASH_ADDR_FW_VER, 32);
		  cmd_SendResp(pcmd, BOOT_CMD_READ_FIRM_VERSION, CMD_OK, resp, 32);
		} break;

		case BOOT_CMD_READ_FIRM_NAME:
		{
		  uint8_t resp[32];
		  memcpy(resp, (const void*)(FLASH_ADDR_FW_VER + 32U), 32);
		  cmd_SendResp(pcmd, BOOT_CMD_READ_FIRM_NAME, CMD_OK, resp, 32);
		} break;

		case BOOT_CMD_FLASH_ERASE:
		{
		  bootCmdFlashErase(pcmd);
		} break;

		case BOOT_CMD_FLASH_WRITE:
		{
		  bootCmdFlashWrite(pcmd);
		} break;


		case BOOT_CMD_LED_CONTROL:
		{
		  if (pcmd->rx_packet.length < 1) { err = BOOT_ERR_LED; }
		  else
		  {
			uint8_t v = pcmd->rx_packet.data[0];
			if (v == 0)      { led_off(LED_CH_BLUE); }
			else if (v == 1) { led_on(LED_CH_BLUE); }
			else if (v == 2) { led_toggle(LED_CH_BLUE); }
			else             { err = BOOT_ERR_LED; }
		  }
		  cmd_SendResp(pcmd, BOOT_CMD_LED_CONTROL, err, NULL, 0);
		} break;

		case BOOT_CMD_WRITE_STOP:
		{
		  if (boot_update_in_progress())
		  {
			(void)boot_update_end(false);
		  }
		  cmd_SendResp(pcmd, BOOT_CMD_WRITE_STOP, CMD_OK, NULL, 0);
		} break;

		case BOOT_CMD_JUMP_TO_FW:
		{
		  if (boot_verify_fw())
		  {
			if (boot_verify_crc())
			{
			  cmd_SendResp(pcmd, BOOT_CMD_JUMP_TO_FW, CMD_OK, NULL, 0);
			  HAL_Delay(100);
			  boot_bkp_write(BOOT_BKP_DR2, 1);
			  NVIC_SystemReset();
			}
			else
			{
			  cmd_SendResp(pcmd, BOOT_CMD_JUMP_TO_FW, BOOT_ERR_FW_CRC, NULL, 0);
			}
		  }
		  else
		  {
			cmd_SendResp(pcmd, BOOT_CMD_JUMP_TO_FW, BOOT_ERR_INVALID_FW, NULL, 0);
		  }
		} break;

		default:
		  err = BOOT_ERR_WRONG_CMD;
		  cmd_SendResp(pcmd, pcmd->rx_packet.cmd, err, NULL, 0);
		  break;
		}
}

void boot_cmd_process(void)
{
	if (cmd_ReceivePacket(&s_cmd))
	{
	  handle_packet(&s_cmd);
	}
}

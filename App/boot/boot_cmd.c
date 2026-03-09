#include "boot_cmd.h"
#include "boot_update.h"
#include "flash_if.h"
#include "iwdg.h"
#include "led.h"
#include "boot_config.h"
#include "backup.h"
#include "boot_mode.h"
#include <string.h>

static cmd_t s_cmd;

static inline uint32_t u32_le(const uint8_t *p)
{
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline uint16_t u16_le(const uint8_t *p)
{
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

void boot_cmd_init(void)
{
  cmd_Init(&s_cmd);
  cmd_Open(&s_cmd, 3, 115200);
  flash_init();
  boot_update_init(FLASH_ADDR_FW, (uint32_t)(FLASH_ADDR_END - FLASH_ADDR_FW));
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
		  if (pcmd->rx_packet.length < 8) { err = BOOT_ERR_WRONG_RANGE; cmd_SendResp(pcmd, BOOT_CMD_FLASH_ERASE, err, NULL, 0); break; }
		  uint32_t addr = u32_le(&pcmd->rx_packet.data[0]);
		  uint32_t len  = u32_le(&pcmd->rx_packet.data[4]);
		  uint32_t eff_addr = addr;
		  if (!(eff_addr >= FLASH_ADDR_START && eff_addr < FLASH_ADDR_END))
		  {
		    /* Treat as relative offset from FW base if not absolute flash address */
		    if (addr < (FLASH_ADDR_END - FLASH_ADDR_FW))
		    {
		      eff_addr = FLASH_ADDR_FW + addr;
		    }
		  }
		  bool ok = false;
		  if (eff_addr >= FLASH_ADDR_START && (eff_addr + len) <= FLASH_ADDR_END)
		  {
		    ok = flash_erase(eff_addr, len);
		  }
		  err = ok ? CMD_OK : BOOT_ERR_FLASH_ERASE;
		  cmd_SendResp(pcmd, BOOT_CMD_FLASH_ERASE, err, NULL, 0);
		} break;

		case BOOT_CMD_FLASH_WRITE:
		{
		  if (pcmd->rx_packet.length < 8) { err = BOOT_ERR_WRONG_RANGE; cmd_SendResp(pcmd, BOOT_CMD_FLASH_WRITE, err, NULL, 0); break; }
		  uint32_t addr = u32_le(&pcmd->rx_packet.data[0]);
		  uint32_t len  = u32_le(&pcmd->rx_packet.data[4]);
		  if ((uint32_t)pcmd->rx_packet.length < (8U + len)) { err = BOOT_ERR_BUF_OVF; cmd_SendResp(pcmd, BOOT_CMD_FLASH_WRITE, err, NULL, 0); break; }
		  const uint8_t *pl = &pcmd->rx_packet.data[8];
		
		  uint32_t eff_addr = addr;
		  if (!(eff_addr >= FLASH_ADDR_START && eff_addr < FLASH_ADDR_END))
		  {
		    /* Treat as relative offset from FW base if not absolute flash address */
		    if (addr < (FLASH_ADDR_END - FLASH_ADDR_FW))
		    {
		      eff_addr = FLASH_ADDR_FW + addr;
		    }
		  }
		
		  if ((eff_addr % FLASH_PROG_ALIGN) != 0U)
		  {
		    err = BOOT_ERR_WRONG_RANGE;
		  }
		  else if (!(eff_addr >= FLASH_ADDR_START && (eff_addr + len) <= FLASH_ADDR_END))
		  {
		    err = BOOT_ERR_WRONG_RANGE;
		  }
		  else
		  {
		    bool ok = flash_write(eff_addr, pl, len);
		    err = ok ? CMD_OK : BOOT_ERR_FLASH_WRITE;
		  }
		  cmd_SendResp(pcmd, BOOT_CMD_FLASH_WRITE, err, NULL, 0);
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

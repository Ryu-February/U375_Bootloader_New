#include "boot_mode.h"
#include "boot_update.h"
#include "backup.h"
#include "power.h"
#include "boot_config.h"
#include "stm32u3xx_hal.h"

static bool is_vector_valid(void)
{
  uint32_t app_stack = *(uint32_t *)(FLASH_ADDR_FW + 0U);
  uint32_t app_reset = *(uint32_t *)(FLASH_ADDR_FW + 4U);

  const uint32_t RAM_START  = 0x20000000UL;
  const uint32_t RAM_END    = 0x20040000UL;

  if (app_stack < RAM_START || app_stack >= RAM_END)
    return false;
  if (app_reset < FLASH_ADDR_START || app_reset >= FLASH_ADDR_END)
    return false;
  return true;
}

bool boot_verify_fw(void)
{
  return is_vector_valid();
}

bool boot_verify_crc(void)
{
  // TODO: implement actual CRC verification against a stored tag when spec is available
  // For now, rely on vector validity only
  return true;
}

void boot_check_mode(void)
{
  if (boot_bkp_read(BOOT_BKP_DR1))
  {
    boot_bkp_write(BOOT_BKP_DR1, 0);
    return;
  }
  else
  {
    if (boot_verify_fw() && boot_verify_crc())
    {
      if (boot_bkp_read(BOOT_BKP_DR2) == 0)
      {
        power_check_btn();
      }
      else
      {
        boot_bkp_write(BOOT_BKP_DR2, 0);
      }
//      boot_jump_to_fw();
      boot_JumpToFw_New();
    }
  }
}

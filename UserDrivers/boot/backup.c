#include "backup.h"
#include "utils.h"
#include "stm32u3xx_hal_rcc.h"
#include "stm32u3xx_hal_pwr.h"

static void boot_bkp_enable_access(void)
{
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_RCC_RTCAPB_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();
}

void boot_bkp_write(uint32_t index, uint32_t value)
{
  boot_bkp_enable_access();
  switch (index)
  {
    case 0U: TAMP->BKP0R = value; break;
    case 1U: TAMP->BKP1R = value; break;
    case 2U: TAMP->BKP2R = value; break;
    case 3U: TAMP->BKP3R = value; break;
    case 4U: TAMP->BKP4R = value; break;
    case 5U: TAMP->BKP5R = value; break;
    default: break;
  }
}

uint32_t boot_bkp_read(uint32_t index)
{
  boot_bkp_enable_access();
  switch (index)
  {
    case 0U: return TAMP->BKP0R;
    case 1U: return TAMP->BKP1R;
    case 2U: return TAMP->BKP2R;
    case 3U: return TAMP->BKP3R;
    case 4U: return TAMP->BKP4R;
    case 5U: return TAMP->BKP5R;
    default: return 0U;
  }
}

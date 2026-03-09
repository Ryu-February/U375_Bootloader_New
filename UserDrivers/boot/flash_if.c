#include "flash_if.h"
#include "stm32u3xx_hal_flash.h"
#include "stm32u3xx_hal_flash_ex.h"
#include "iwdg.h"
#include <string.h>

/* On STM32U3 with TrustZone, use non-secure program type when available */
#ifndef FLASH_TYPEPROGRAM_DOUBLEWORD_NS
#define FLASH_TYPEPROGRAM_DOUBLEWORD_NS FLASH_TYPEPROGRAM_DOUBLEWORD
#endif

/* Use non-secure page erase when available */
#ifndef FLASH_TYPEERASE_PAGES_NS
#define FLASH_TYPEERASE_PAGES_NS FLASH_TYPEERASE_PAGES
#endif

flash_sector_t flash_table[FLASH_SECTOR_MAX];

static inline bool is_aligned(uint32_t v, uint32_t a) { return (v & (a - 1U)) == 0U; }
static inline uint32_t align_down(uint32_t v, uint32_t a){ return v & ~(a - 1U); }
static inline uint32_t align_up  (uint32_t v, uint32_t a){ return (v + a - 1U) & ~(a - 1U); }

static bool flash_in_sector(uint16_t sector, uint32_t addr, uint32_t length)
{
  uint32_t s_addr = flash_table[sector].addr;
  uint32_t e_addr = s_addr + flash_table[sector].length;
  uint32_t r_beg = addr;
  uint32_t r_end = addr + length;
  return (r_beg < e_addr) && (r_end > s_addr);
}

void flash_init(void)
{
  for (uint16_t i = 0; i < FLASH_SECTOR_MAX; i++)
  {
    flash_table[i].addr = FLASH_START_ADDR + FLASH_PAGE_SIZE * i;
    flash_table[i].length = FLASH_PAGE_SIZE;
  }
}

bool flash_is_range_valid(uint32_t addr_start, uint32_t length)
{
  if (length == 0U) return false;
  uint32_t addr_end = addr_start + length - 1U;
  uint32_t flash_start = FLASH_ADDR_START;
  uint32_t flash_end = FLASH_ADDR_END;
  return ((addr_start >= flash_start) && (addr_start < flash_end) && (addr_end >= flash_start) && (addr_end < flash_end));
}

bool flash_erase(uint32_t addr, uint32_t length)
{
  if (!flash_is_range_valid(addr, length)) return false;

  int16_t start_sector = -1;
  uint32_t sector_count = 0;

  for (uint16_t i = 0; i < FLASH_SECTOR_MAX; i++)
  {
    if (flash_in_sector(i, addr, length))
    {
      if (start_sector < 0) start_sector = (int16_t)i;
      sector_count++;
    }
  }

  if (sector_count == 0U) return false;

  uint32_t page_addr = flash_table[(uint16_t)start_sector].addr;

  if (HAL_FLASH_Unlock() != HAL_OK) return false;

  bool ok = true;
  uint32_t first_page = (page_addr - FLASH_START_ADDR) / FLASH_PAGE_SIZE;
  for (uint32_t n = 0; n < sector_count; n++)
  {
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0;
    erase.TypeErase = FLASH_TYPEERASE_PAGES_NS;
    erase.Banks = FLASH_BANK_1;
    erase.Page = first_page + n;
    erase.NbPages = 1;

    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK)
    {
      ok = false;
      break;
    }
    iwdg_refresh();
  }

  HAL_FLASH_Lock();
  return ok;
}

bool flash_write(uint32_t addr, const uint8_t *p_data, uint32_t length)
{
  if (!flash_is_range_valid(addr, length)) return false;
  if (!is_aligned(addr, FLASH_PROG_ALIGN)) return false;
  if (addr < FLASH_START_ADDR) return false;

  if (HAL_FLASH_Unlock() != HAL_OK) return false;

  bool ok = true;

  uint32_t i = 0;
  for (; i + 8U <= length; i += 8U)
  {
    uint64_t dw =  ((uint64_t)p_data[i + 0] << 0)
                 | ((uint64_t)p_data[i + 1] << 8)
                 | ((uint64_t)p_data[i + 2] << 16)
                 | ((uint64_t)p_data[i + 3] << 24)
                 | ((uint64_t)p_data[i + 4] << 32)
                 | ((uint64_t)p_data[i + 5] << 40)
                 | ((uint64_t)p_data[i + 6] << 48)
                 | ((uint64_t)p_data[i + 7] << 56);

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD_NS, addr + i, dw) != HAL_OK)
    {
      ok = false;
      break;
    }
    iwdg_refresh();
  }

  if (ok)
  {
    uint32_t rem = length - i;
    if (rem > 0U)
    {
      uint8_t tmp[8];
      for (uint32_t k = 0; k < 8U; k++) tmp[k] = 0xFFU;
      for (uint32_t k = 0; k < rem; k++) tmp[k] = p_data[i + k];
      uint64_t dw =  ((uint64_t)tmp[0] << 0)
                   | ((uint64_t)tmp[1] << 8)
                   | ((uint64_t)tmp[2] << 16)
                   | ((uint64_t)tmp[3] << 24)
                   | ((uint64_t)tmp[4] << 32)
                   | ((uint64_t)tmp[5] << 40)
                   | ((uint64_t)tmp[6] << 48)
                   | ((uint64_t)tmp[7] << 56);
      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD_NS, addr + i, dw) != HAL_OK)
      {
        ok = false;
      }
      else
      {
        iwdg_refresh();
      }
    }
  }

  HAL_FLASH_Lock();
  return ok;
}

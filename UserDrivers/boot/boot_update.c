#include "boot_update.h"
#include "boot_config.h"
#include "flash_if.h"
#include "crc16.h"
#include "iwdg.h"
#include "stm32u3xx_hal.h"
#include <string.h>

typedef struct
{
  bool     in_progress;
  uint32_t base_addr;
  uint32_t max_size;
  uint32_t total_size;
  uint32_t written;
  uint16_t expected_crc;
  uint16_t rolling_crc;
} boot_update_ctx_t;

static boot_update_ctx_t ctx;

static inline bool is_aligned(uint32_t v, uint32_t a) { return (v & (a - 1U)) == 0U; }

void boot_update_init(uint32_t base_addr, uint32_t max_size)
{
  memset(&ctx, 0, sizeof(ctx));
  ctx.base_addr = base_addr;
  ctx.max_size = max_size;
  flash_init();
}

bool boot_update_begin(uint32_t total_size, uint16_t expected_crc)
{
  if (total_size == 0U) return false;
  if (total_size > ctx.max_size) return false;
  if (!flash_is_range_valid(ctx.base_addr, total_size)) return false;

  ctx.total_size = total_size;
  ctx.expected_crc = expected_crc;
  ctx.written = 0U;
  ctx.in_progress = true;
  crc16_reset(&ctx.rolling_crc);

  uint32_t erase_len = ((total_size + (FLASH_PAGE_SIZE - 1U)) / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE;
  if (!flash_erase(ctx.base_addr, erase_len))
  {
    ctx.in_progress = false;
    return false;
  }
  return true;
}

bool boot_update_write(uint32_t offset, const uint8_t *data, uint32_t length)
{
  if (!ctx.in_progress) return false;
  if (data == NULL) return false;
  if (length == 0U) return false;

  uint32_t addr = ctx.base_addr + offset;

  if ((offset + length) > ctx.total_size) return false;
  if (!flash_is_range_valid(addr, length)) return false;

  if (!is_aligned(addr, FLASH_PROG_ALIGN)) return false;
  if ((length % FLASH_PROG_ALIGN) != 0U) return false;

  if (!flash_write(addr, data, length)) return false;

  for (uint32_t i = 0; i < length; i++)
  {
    crc16_feed(&ctx.rolling_crc, data[i]);
  }
  if (offset + length > ctx.written)
  {
    ctx.written = offset + length;
  }

  iwdg_refresh();
  return true;
}

bool boot_update_end(bool do_reset)
{
  if (!ctx.in_progress) return false;

  bool ok = (ctx.written == ctx.total_size) && (ctx.rolling_crc == ctx.expected_crc);

  ctx.in_progress = false;

  if (ok && do_reset)
  {
    __disable_irq();
    NVIC_SystemReset();
  }
  return ok;
}

bool boot_update_in_progress(void)
{
  return ctx.in_progress;
}

bool boot_jump_to_fw(void)
{
  uint32_t app_stack = *(uint32_t *)(FLASH_ADDR_FW + 0U);
  uint32_t app_reset = *(uint32_t *)(FLASH_ADDR_FW + 4U);

  // RAM ranges from linker: RAM (192K) 0x20000000..0x20030000, RAM2 (64K) 0x20030000..0x20040000
  const uint32_t RAM_START  = 0x20000000UL;
  const uint32_t RAM_END    = 0x20040000UL; // end of RAM2

  // Basic vector table sanity
  if (app_stack < RAM_START || app_stack >= RAM_END)
  {
    return false;
  }

  // Reset handler must be in flash FW region and Thumb bit set
  if (app_reset < FLASH_ADDR_START || app_reset >= FLASH_ADDR_END || ((app_reset & 1U) == 0U))
  {
    return false;
  }

  HAL_RCC_DeInit();

  for (uint16_t i = 0; i < 16; i++)
  {
    NVIC->ICER[i] = 0xFFFFFFFFu;
    __DSB();
    __ISB();
  }
  SysTick->CTRL = 0u;

  __set_MSP(app_stack);
  void (*app_entry)(void) = (void (*)(void))(app_reset & ~1U);
  app_entry();
  return true;
}

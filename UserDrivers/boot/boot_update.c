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
#define HSI_TIMEOUT_VALUE               (2U)
#define CLOCKSWITCH_TIMEOUT_VALUE       5000U     /* 5 s    */

HW_StatusTypeDef RCC_DeInit(void)
{
	uint32_t tickstart;

	/* Get start tick */
	tickstart = ms_now();

	/* Set MSISON and MSIKON bit */
	SET_BIT(RCC->CR, (RCC_CR_MSISON | RCC_CR_MSIKON));

	/* Wait till MSIS is ready */
	while(READ_BIT(RCC->CR, RCC_CR_MSISRDY) == 0U)
	{
		if((ms_now() - tickstart) > HSI_TIMEOUT_VALUE)
		{
			if (READ_BIT(RCC->CR, RCC_CR_MSISRDY) == 0U)
			{
				return HW_TIMEOUT;
			}
		}
	}

	/* Select MSIS and MSIK source and divider */
	MODIFY_REG(RCC->ICSCR1, (RCC_ICSCR1_MSISDIV | RCC_ICSCR1_MSIKDIV),
	(RCC_ICSCR1_MSISDIV_0 | RCC_ICSCR1_MSISSEL | RCC_ICSCR1_MSIKDIV_0 | RCC_ICSCR1_MSIKSEL));

	/* Set MSRCx trimming default value */
	WRITE_REG(RCC->ICSCR2, (RCC_ICSCR2_MSITRIM1_5 | RCC_ICSCR2_MSITRIM0_5));

	/* Set HSITRIM default value */
	WRITE_REG(RCC->ICSCR3, RCC_ICSCR3_HSITRIM_4);

	/* Get start tick*/
	tickstart = ms_now();

	/* Reset CFGR1 register (MSIS is selected as system clock source) */
	CLEAR_REG(RCC->CFGR1);

	/* Wait till clock switch is ready */
	while(READ_BIT(RCC->CFGR1, RCC_CFGR1_SWS) != 0U)
	{
		if((ms_now() - tickstart) > CLOCKSWITCH_TIMEOUT_VALUE)
		{
			if(READ_BIT(RCC->CFGR1, RCC_CFGR1_SWS) != 0U)
			{
				return HW_TIMEOUT;
			}
		}
	}

	/* Set AHBx and APBx prescaler to their default values */
	CLEAR_REG(RCC->CFGR2);
	CLEAR_REG(RCC->CFGR3);
	CLEAR_REG(RCC->CFGR4);

	/* Clear CR register in 2 steps: first to clear HSEON in case bypass was enabled */
	RCC->CR = (RCC_CR_MSISON | RCC_CR_MSIKON);

	/* Then again to HSEBYP in case bypass was enabled */
	RCC->CR = (RCC_CR_MSISON | RCC_CR_MSIKON);

	/* Disable all interrupts */
	CLEAR_REG(RCC->CIER);

	/* Clear all interrupts flags */
	WRITE_REG(RCC->CICR, 0xFFFFFFFFU);

	/* Update the SystemCoreClock global variable */
	SystemCoreClock = (MSIRC1_VALUE >> 1u);

	/* Adapt Systick interrupt period */
//	if(System_InitTick(uwTickPrio) != HW_OK)
//	{
//		return HW_ERROR;
//	}
//	else
//	{
//		return HW_OK;
//	}
	return HW_OK;
}




void boot_JumpToFw_New(void)
{
	void (**jump_func)(void) = (void (**)(void))(FLASH_ADDR_FW + 4); // 펌웨어 시작점 주소 + 4byte


	if(RCC_DeInit() != HW_OK)
	{
		//UART3_tx_string("RCC_DeInit Error\r\n");
	}


	for(uint16_t i = 0; i < 16; i++)
	{
		NVIC->ICER[i] = 0xFFFFFFFF;	//인터럽트 다 끄는 과정
		__DSB();
		__ISB();
	}
	SysTick->CTRL = 0;

	__set_MSP(*(volatile uint32_t*)FLASH_ADDR_FW);
	(*jump_func)();
}




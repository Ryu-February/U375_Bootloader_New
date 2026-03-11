#include "flash_if.h"
#include "utils.h"
//#include "stm32u3xx_hal_flash.h"
//#include "stm32u3xx_hal_flash_ex.h"
//#include "iwdg.h"
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
    flash_table[i].addr = FLASH_START_ADDR + 4096 * i;
    flash_table[i].length = 4096;
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

HW_StatusTypeDef FLASH_Unlock(void)
{
    HW_StatusTypeDef status = HW_OK;

    if(READ_BIT(FLASH->CR, FLASH_CR_LOCK) != RESET)
    {
        /* Authorize the FLASH Registers access */
        WRITE_REG(FLASH->KEYR, FLASH_KEY1);
        WRITE_REG(FLASH->KEYR, FLASH_KEY2);

        /* Verify Flash is unlocked */
        if(READ_BIT(FLASH->CR, FLASH_CR_LOCK) != RESET)
        {
            status = HW_ERROR;
        }
    }

    return status;
}

HW_StatusTypeDef FLASH_Lock(void)
{
    HW_StatusTypeDef status = HW_ERROR;

    /* Set the LOCK Bit to lock the FLASH Registers access */
    SET_BIT(FLASH->CR, FLASH_CR_LOCK);

    /* Verify Flash is locked */
    if(READ_BIT(FLASH->CR, FLASH_CR_LOCK) != RESET)
    {
        status = HW_OK;
    }

    return status;
}


uint8_t flash_program_range_dw(uint32_t addr, uint8_t *p_data, uint32_t length)
{
    uint8_t result = true;
    uint8_t error_status = 0;

    /* U385 requires 8B-aligned address & length, and full DW programming */
    if(((addr % 8U) != 0U) || ((length % 8U) != 0U) || (addr < FLASH_START_ADDR))
    {
        return false;
    }

    error_status = FLASH_Unlock();

    if(error_status)
    {
        return false;
    }

    /* write in 8-byte units */
    for(uint32_t i = 0; i < length; i += 8U)
    {
        //uint16_t data = (uint16_t)(p_data[i + 1] << 8) + p_data[i];
        uint64_t data =
            ((uint64_t)p_data[i + 0])       |
            ((uint64_t)p_data[i + 1] << 8)  |
            ((uint64_t)p_data[i + 2] << 16) |
            ((uint64_t)p_data[i + 3] << 24) |
            ((uint64_t)p_data[i + 4] << 32) |
            ((uint64_t)p_data[i + 5] << 40) |
            ((uint64_t)p_data[i + 6] << 48) |
            ((uint64_t)p_data[i + 7] << 56);

        if(FLASH_Write_Ex(addr + i, data) != HW_OK)
        {
            result = false;
            break;
        }
    }

    FLASH_Lock();

    return result;
}

HW_StatusTypeDef FLASH_Wait_LastOperation(uint16_t timeout)
{
    HW_StatusTypeDef status = HW_OK;

    uint32_t tickStart = ms_now();

    while(READ_BIT(FLASH->SR, FLASH_SR_BSY))
    {
        if(ms_now() - tickStart >= timeout)
        {
            status = HW_TIMEOUT;
            break;
        }
    }

    /* Check FLASH End of Operation flag  */
    if(READ_BIT(FLASH->SR, FLASH_SR_EOP))
    {
        /* Clear FLASH End of Operation pending bit */
        WRITE_REG(FLASH->SR, FLASH_SR_EOP);
    }

    return status;
}


HW_StatusTypeDef FLASH_Write_Ex(uint32_t PageAddr, uint64_t data)
{
    HW_StatusTypeDef status = HW_OK;

    if(FLASH_Wait_LastOperation(FLASH_TIMEOUT_VALUE) == HW_OK)
    {
        /* Address must be DW-aligned and we always write a full DW */
        uint32_t primask_bit;
        if(PageAddr & 0x7U)
        {
            return HW_ERROR;
        }
        // 에러/완료 플래그는 1로 써서 미리 클리어해 두는 습관 권장
        WRITE_REG(FLASH->SR, FLASH_SR_EOP | FLASH_SR_WRPERR | FLASH_SR_PGSERR | FLASH_SR_OPERR);

        /* Enter critical section: Disable interrupts to avoid any interruption during the loop */
        primask_bit = __get_PRIMASK();
        __disable_irq();

        /* Set FLASH_CR_PG bit --------------------------------------------- */
        SET_BIT(FLASH->CR, FLASH_CR_PG);
        __DSB(); __ISB();

        /* Write(Program) the double-word ---------------------------------- */
        *(__IO uint32_t *)(PageAddr    ) = (uint32_t)(data & 0xFFFFFFFFULL); // 하위 32비트 먼저
        *(__IO uint32_t *)(PageAddr + 4) = (uint32_t)(data >> 32);           // 상위 32비트

        __DSB(); __ISB();

        /* Exit critical section: restore previous priority mask */
        __set_PRIMASK(primask_bit);

        if(FLASH_Wait_LastOperation(FLASH_TIMEOUT_VALUE) != HW_OK)
        {
            status = HW_ERROR;
        }

        CLEAR_BIT(FLASH->CR, FLASH_CR_PG);

        // EOP/에러 플래그 정리
        WRITE_REG(FLASH->SR, FLASH_SR_EOP | FLASH_SR_WRPERR | FLASH_SR_PGSERR | FLASH_SR_OPERR);
    }
    else
    {
        status = HW_ERROR;
    }

    return status;
}


HW_StatusTypeDef FLASH_InSector_Page(uint32_t addr, Flash_Page_t *pageData)
{
    HW_StatusTypeDef status = HW_OK;

    if(addr < FLASH_MAIN_BASE || addr >= (FLASH_MAIN_BASE + 2U*FLASH_BANK_SIZE))
    {
        status = HW_ERROR;
        return status;
    }

    const uint32_t boundary = FLASH_MAIN_BASE + FLASH_BANK_SIZE;
    //const uint32_t swapped  = FLASH_IsBankSwapped();

    uint32_t bank;
    uint32_t bank_base;

    if(addr < boundary)
    {
        bank = FLASH_BANK_1;
        bank_base = FLASH_MAIN_BASE;
    }
    else
    {
        bank = FLASH_BANK_2;
        bank_base = boundary;
    }

    uint32_t offset = addr - bank_base;
    uint8_t page   = (uint8_t)(offset / FLASH_PAGE_SIZE);
    if(page >= FLASH_PAGES_PER_BANK)
    {
        status = HW_ERROR;
        return status;
    }

    pageData->bank = bank;
    pageData->page = page;
    pageData->page_base_addr = bank_base + ((uint32_t)page * FLASH_PAGE_SIZE);

    return status;
}


HW_StatusTypeDef FLASH_Erase_Ex(uint32_t PageAddr, uint8_t PageNum)
{
    HW_StatusTypeDef status = HW_OK;

    if(FLASH_Wait_LastOperation(FLASH_TIMEOUT_VALUE) == HW_OK)
    {
        Flash_Page_t pageData;
        if(FLASH_InSector_Page(PageAddr, &pageData) == HW_OK)
        {
            /* Loop over N pages */
            for(uint32_t i = 0; i < (uint32_t)PageNum; i++)
            {
                /* Select bank */
                if((pageData.bank & FLASH_BANK_1) != 0U) { CLEAR_BIT(FLASH->CR, FLASH_CR_BKER); }
                else                                     { SET_BIT(FLASH->CR, FLASH_CR_BKER);   }

                /* Program page number (current page = base page + i) and start erase */
                MODIFY_REG(FLASH->CR,
                           (FLASH_CR_PNB | FLASH_CR_PER | FLASH_CR_STRT),
                           (((uint32_t)(pageData.page + i) << FLASH_CR_PNB_Pos) | FLASH_CR_PER | FLASH_CR_STRT));

                /* Wait complete */
                if(FLASH_Wait_LastOperation(FLASH_TIMEOUT_VALUE) != HW_OK)
                {
                    status = HW_ERROR;
                    break;
                }

                /* Clear control bits for next iteration */
                CLEAR_BIT(FLASH->CR, FLASH_CR_PER | FLASH_CR_PNB);
            }
            /* Clear bank select bit after loop */
            CLEAR_BIT(FLASH->CR, FLASH_CR_BKER);
        }
        else
        {
            status = HW_ERROR;
        }
    }
    else
    {
        status = HW_ERROR;
    }

    return status;
}


uint8_t flash_InSector(uint32_t num, uint32_t addr, uint32_t length)
{
    uint8_t result = false;

    uint32_t sector_start, sector_end;
    uint32_t flash_start, flash_end;

    sector_start = flash_table[num].addr;
    sector_end = flash_table[num].addr + flash_table[num].length - 1;
    flash_start = addr;
    flash_end = addr + length - 1;

    if(sector_start >= flash_start && sector_start <= flash_end)
        result = true;
    if(sector_end >= flash_start && sector_end <= flash_end)
        result = true;
    if(flash_start >= sector_start && flash_start <= sector_end)
        result = true;
    if(flash_end >= sector_start && flash_end <= sector_end)
        result = true;

    return result;
}


uint8_t flash_erase_range(uint32_t addr, uint32_t length)
{
    uint8_t result = false;
    uint8_t error_status = 0;

    int16_t start_sectorNum = -1;
    uint8_t sectorNum = 0;

    uint32_t PageAddress = 0;

    for(uint16_t i = 0; i < FLASH_SECTOR_MAX; i++)
    {
        if(flash_InSector(i, addr, length) == true)
        {
            if(start_sectorNum < 0)
                start_sectorNum = i;

            sectorNum++;
        }
    }

    if(sectorNum)
    {
        PageAddress = flash_table[start_sectorNum].addr;

        FLASH_Unlock();

        error_status = FLASH_Erase_Ex(PageAddress, sectorNum);
        //error_status = FLASH_Erase(PageAddress, sectorNum);
        //error_status = FLASH_ErasePage(PageAddress, sectorNum);

        if(error_status == 0)
        {
            result = true;
        }

        FLASH_Lock();

    }

    return result;
}


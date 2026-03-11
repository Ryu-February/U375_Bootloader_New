#ifndef BOOT_FLASH_IF_H_
#define BOOT_FLASH_IF_H_

#include "def.h"
#include "boot_config.h"


#define FLASH_PAGE_SIZE                              0x1000U                /*!< 4 Kbyte */
#define FLASH_PAGES_PER_BANK                          (128UL)             /*!< page per bank */
//#define FLASH_BANK_SIZE                               (FLASH_PAGES_PER_BANK * FLASH_PAGE_SIZE) // 0x80000
#define FLASH_MAIN_BASE                                0x08000000U

typedef struct
{
  uint32_t addr;
  uint32_t length;
} flash_sector_t;

typedef enum
{
    HW_OK       = 0x00,
    HW_ERROR    = 0x01,
    HW_BUSY     = 0x02,
    HW_TIMEOUT  = 0x03

} HW_StatusTypeDef;



typedef struct
{
    uint32_t bank;
    uint8_t page;                /*!< page number (0..127) */

    uint32_t page_base_addr;    /*!< page start(base) address */

} Flash_Page_t;

extern flash_sector_t flash_table[FLASH_SECTOR_MAX];

void flash_init(void);
bool flash_is_range_valid(uint32_t addr_start, uint32_t length);
bool flash_erase(uint32_t addr, uint32_t length);
bool flash_write(uint32_t addr, const uint8_t *p_data, uint32_t length);

#define flash_Write flash_write
#define flash_Erase flash_erase

uint8_t flash_program_range_dw(uint32_t addr, uint8_t *p_data, uint32_t length);
HW_StatusTypeDef FLASH_Write_Ex(uint32_t PageAddr, uint64_t data);
uint8_t flash_erase_range(uint32_t addr, uint32_t length);

#endif /* BOOT_FLASH_IF_H_ */


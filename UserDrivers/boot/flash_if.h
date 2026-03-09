#ifndef BOOT_FLASH_IF_H_
#define BOOT_FLASH_IF_H_

#include "def.h"
#include "boot_config.h"

typedef struct
{
  uint32_t addr;
  uint32_t length;
} flash_sector_t;

extern flash_sector_t flash_table[FLASH_SECTOR_MAX];

void flash_init(void);
bool flash_is_range_valid(uint32_t addr_start, uint32_t length);
bool flash_erase(uint32_t addr, uint32_t length);
bool flash_write(uint32_t addr, const uint8_t *p_data, uint32_t length);

#define flash_Write flash_write
#define flash_Erase flash_erase

#endif /* BOOT_FLASH_IF_H_ */

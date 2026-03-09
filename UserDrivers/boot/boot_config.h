#ifndef BOOT_BOOT_CONFIG_H_
#define BOOT_BOOT_CONFIG_H_

#include "def.h"

#define FLASH_SECTOR_MAX            128U
#define FLASH_START_ADDR            0x08000000UL

//#define FLASH_PAGE_SIZE             4096UL

#define FLASH_ADDR_TAG              0x08010000UL
#define FLASH_ADDR_FW               0x08012000UL
#define FLASH_ADDR_FW_VER           0x08012400UL

#define FLASH_ADDR_START            0x08010000UL
#define FLASH_ADDR_END              (FLASH_ADDR_START + (112UL) * FLASH_PAGE_SIZE)
#define FLASH_PROG_ALIGN            8UL

#endif /* BOOT_BOOT_CONFIG_H_ */

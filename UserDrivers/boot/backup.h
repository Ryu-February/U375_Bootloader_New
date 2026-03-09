#ifndef USERDRIVERS_BOOT_BACKUP_H_
#define USERDRIVERS_BOOT_BACKUP_H_

#include "def.h"

#define BOOT_BKP_DR1   1U
#define BOOT_BKP_DR2   2U

void boot_bkp_write(uint32_t index, uint32_t value);
uint32_t boot_bkp_read(uint32_t index);

#endif /* USERDRIVERS_BOOT_BACKUP_H_ */

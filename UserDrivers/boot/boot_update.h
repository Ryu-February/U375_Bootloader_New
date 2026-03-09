#ifndef BOOT_BOOT_UPDATE_H_
#define BOOT_BOOT_UPDATE_H_

#include "def.h"

void boot_update_init(uint32_t base_addr, uint32_t max_size);
bool boot_update_begin(uint32_t total_size, uint16_t expected_crc);
bool boot_update_write(uint32_t offset, const uint8_t *data, uint32_t length);
bool boot_update_end(bool do_reset);
bool boot_update_in_progress(void);

bool boot_jump_to_fw(void);

#endif /* BOOT_BOOT_UPDATE_H_ */

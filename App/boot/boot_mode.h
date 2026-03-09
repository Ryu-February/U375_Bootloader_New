#ifndef APP_BOOT_BOOT_MODE_H_
#define APP_BOOT_BOOT_MODE_H_

#include "def.h"

bool boot_verify_fw(void);
bool boot_verify_crc(void);
void boot_check_mode(void);

#endif /* APP_BOOT_BOOT_MODE_H_ */

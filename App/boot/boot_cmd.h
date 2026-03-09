#ifndef APP_BOOT_BOOT_CMD_H_
#define APP_BOOT_BOOT_CMD_H_

#include "def.h"
#include "cmd.h"

// Command IDs (company tool)
#define BOOT_CMD_READ_BOOT_VERSION      0x00
#define BOOT_CMD_READ_BOOT_NAME         0x01
#define BOOT_CMD_READ_FIRM_VERSION      0x02
#define BOOT_CMD_READ_FIRM_NAME         0x03
#define BOOT_CMD_FLASH_ERASE            0x04
#define BOOT_CMD_FLASH_WRITE            0x05

#define BOOT_CMD_JUMP_TO_FW             0x08
#define BOOT_CMD_LED_CONTROL            0x10

#define BOOT_CMD_WRITE_STOP             0x20

// Error codes (company tool)
#define BOOT_ERR_WRONG_CMD      0x01
#define BOOT_ERR_LED            0x02
#define BOOT_ERR_FLASH_ERASE    0x03
#define BOOT_ERR_WRONG_RANGE    0x04
#define BOOT_ERR_FLASH_WRITE    0x05
#define BOOT_ERR_BUF_OVF        0x06
#define BOOT_ERR_INVALID_FW     0x07
#define BOOT_ERR_FW_CRC         0x08

void boot_cmd_init(void);
void boot_cmd_process(void);

#endif /* APP_BOOT_BOOT_CMD_H_ */

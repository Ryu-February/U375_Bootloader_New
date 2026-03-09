#ifndef COMMON_CRC16_H_
#define COMMON_CRC16_H_

#include "def.h"

void crc16_reset(uint16_t *crc);
void crc16_feed(uint16_t *crc, uint8_t data);
uint16_t crc16_compute(const uint8_t *data, uint32_t length);

#endif /* COMMON_CRC16_H_ */

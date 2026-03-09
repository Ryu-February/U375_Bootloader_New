#include "crc16.h"

// CRC-16/CCITT-FALSE: poly=0x1021, init=0xFFFF, refin=false, refout=false, xorout=0x0000

void crc16_reset(uint16_t *crc)
{
  if (crc) *crc = 0xFFFFu;
}

void crc16_feed(uint16_t *crc, uint8_t data)
{
  uint16_t c = *crc;
  c ^= (uint16_t)data << 8;
  for (int i = 0; i < 8; i++)
  {
    if (c & 0x8000)
      c = (c << 1) ^ 0x1021;
    else
      c = (c << 1);
  }
  *crc = c;
}

uint16_t crc16_compute(const uint8_t *data, uint32_t length)
{
  uint16_t c; crc16_reset(&c);
  for (uint32_t i = 0; i < length; i++)
  {
    crc16_feed(&c, data[i]);
  }
  return c;
}

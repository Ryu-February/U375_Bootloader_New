/*
 * swv.h
 *
 *  Created on: 2026. 4. 17.
 *      Author: RCY
 */

#ifndef SWV_SWV_H_
#define SWV_SWV_H_

#include "def.h"

void swv_init(uint32_t core_clock_hz, uint32_t swo_clock_hz);
int swv_printf(const char *fmt, ...);

#endif /* SWV_SWV_H_ */

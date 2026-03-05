/*
 * utils.c
 *
 *  Created on: 2026. 3. 5.
 *      Author: RCY
 */

#include "utils.h"






void delay_ms(uint32_t ms)
{
	HAL_Delay(ms);
}

uint32_t ms_now(void)
{
	return HAL_GetTick();
}

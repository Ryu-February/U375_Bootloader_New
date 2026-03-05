/*
 * boot_indicator.c
 *
 *  Created on: 2026. 3. 5.
 *      Author: RCY
 */

#include "boot_indicator.h"
#include "led.h"




void boot_indicator_tick(void)
{
	uint32_t now = ms_now();
	static uint32_t prev = 0;

	if (now - prev >= 50)
	{
		prev = now;
		led_toggle(LED_CH_BLUE);
	}
}

/*
 * ap.c
 *
 *  Created on: 2026. 3. 5.
 *      Author: RCY
 */

#include "ap.h"




void ap_init(void)
{
	led_init();
}

void ap_main(void)
{
	while (1)
	{
		uint32_t now = ms_now();
		static uint32_t prev = 0;

		if (now - prev >= 50)
		{
			prev = now;
			led_toggle(LED_CH_BLUE);
		}
	}
}

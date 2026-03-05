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
		boot_indicator_tick();

		iwdg_refresh();
	}
}

/*
 * ap.c
 *
 *  Created on: 2026. 3. 5.
 *      Author: RCY
 */

#include "ap.h"

#include "power.h"



void ap_init(void)
{
	power_check_btn();

	led_init();
	power_init();
}

void ap_main(void)
{
	while (1)
	{
		boot_indicator_tick();

		iwdg_refresh();

		power_poll();
	}
}


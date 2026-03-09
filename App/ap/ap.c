/*
 * ap.c
 *
 *  Created on: 2026. 3. 5.
 *      Author: RCY
 */

#include "ap.h"

#include "power.h"
#include "boot_cmd.h"
#include "boot_mode.h"



void ap_init(void)
{
//    power_check_btn();
	led_init();
	power_init();
    boot_check_mode();
	boot_cmd_init();
}

void ap_main(void)
{
	while (1)
	{
		boot_indicator_tick();
		boot_cmd_process();

		iwdg_refresh();

		power_poll();
	}
}


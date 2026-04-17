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
#include "swv.h"

volatile uint32_t live_watch = 0;
volatile uint32_t millis = 0;
volatile uint32_t prev_ms = 0;

void ap_init(void)
{
//    power_check_btn();
	led_init();

	// SWV 테스트: 5초간 반복 출력 (타이밍 문제 배제용)
	for (int i = 0; i < 10; i++)
	{
		swv_printf("SWV Test: %d\r\n", i);
		HAL_Delay(500);
	}

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
		millis = ms_now();

		if (millis - prev_ms >= 500)
		{
			live_watch++;
			prev_ms = millis;
			swv_printf("live_watch: %lu\r\n", live_watch);
		}

	}
}


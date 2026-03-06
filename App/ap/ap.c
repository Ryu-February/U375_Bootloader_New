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
	led_init();

	for(uint16_t i = 0; i < 500; i++)
	{
		if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) != 0)
		{
			power_enter_shutdown_safe();
		}
		delay_ms(1);
	}
}

void ap_main(void)
{
	uint32_t pa0_press_start = 0;
	uint8_t pa0_pressed_last = 0;
	uint8_t shutdown_pending = 0;

	while (1)
	{
		boot_indicator_tick();

		iwdg_refresh();

		uint8_t pa0 = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == 0);//전원 버튼 (웨이크업 핀)
		if (pa0)
		{
			if (!pa0_pressed_last)
			{
				pa0_pressed_last = 1;
				pa0_press_start = ms_now();
			}
			else
			{
				if (!shutdown_pending && (ms_now() - pa0_press_start >= 500))//0.5초 이상 눌림 → 꺼짐 예약
				{
					shutdown_pending = 1;
				}
			}
		}
		else
		{
			if (pa0_pressed_last)
			{
				if (shutdown_pending)
				{
					shutdown_pending = 0;
					power_enter_shutdown_safe();
				}
			}
			pa0_pressed_last = 0;
		}
	}
}

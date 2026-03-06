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

	for(uint16_t i = 0; i < 500; i++)
	{
		if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) != 0)
		{
			enter_shutdown_safe();
		}
		delay_ms(1);
	}
}

void ap_main(void)
{
	uint32_t pa0_press_start = 0;
	uint8_t pa0_pressed_last = 0;
	uint8_t standby_armed = 1;

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
				if (standby_armed && (ms_now() - pa0_press_start >= 500))//0.5초 이상 누르면 꺼지게
				{
					standby_armed = 0;
					enter_shutdown_safe();
				}
			}
		}
		else
		{
			pa0_pressed_last = 0;
			standby_armed = 1;
		}
	}
}

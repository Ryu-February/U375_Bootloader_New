/*
 * led.c
 *
 *  Created on: 2026. 3. 5.
 *      Author: RCY
 */



#include "led.h"


typedef struct
{
	GPIO_TypeDef	*port;
	uint16_t		pin;
	bool			active_low;
	bool 			boot_on;
}led_ch_cfg_t;


static led_ch_cfg_t led_cfg[LED_CH_COUNT] =
{
		[LED_CH_BLUE]	 = {GPIOC, GPIO_PIN_0, true, false},
		[LED_CH_RED]	 = {GPIOC, GPIO_PIN_1, true, false},
		[LED_CH_GREEN]	 = {GPIOC, GPIO_PIN_2, true, false},
};

static bool led_state[LED_CH_COUNT] = {0};


static GPIO_PinState level_for(const led_ch_cfg_t *c, bool on)
{
	if (c->active_low)
		return on ? GPIO_PIN_RESET : GPIO_PIN_SET;
	else
		return on ? GPIO_PIN_RESET : GPIO_PIN_SET;
}


void led_init(void)
{
	for (led_ch_t i = 0; i < LED_CH_COUNT; i++)
	{
		led_state[i] = led_cfg[i].boot_on;
		HAL_GPIO_WritePin(led_cfg[i].port, led_cfg[i].pin, level_for(&led_cfg[i], led_state[i]));
	}
}

void led_write(led_ch_t ch, bool on)
{
	if (ch >= LED_CH_COUNT)
		return;

	led_state[ch] = on;
	HAL_GPIO_WritePin(led_cfg[ch].port, led_cfg[ch].pin, level_for(&led_cfg[ch], led_state[ch]));
}

void led_on(led_ch_t ch)
{
	led_write(ch, true);
}

void led_off(led_ch_t ch)
{
	led_write(ch, false);
}

void led_toggle(led_ch_t ch)
{
	bool next = !led_state[ch];
	led_state[ch] = next;

	led_write(ch, next);
}

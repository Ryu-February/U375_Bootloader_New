/*
 * led.h
 *
 *  Created on: 2026. 3. 5.
 *      Author: RCY
 */

#ifndef LED_LED_H_
#define LED_LED_H_


#include "utils.h"

typedef enum
{
	LED_CH_BLUE,
	LED_CH_RED,
	LED_CH_GREEN,
	LED_CH_COUNT
}led_ch_t;

void led_init(void);
void led_write(led_ch_t ch, bool on);
void led_on(led_ch_t ch);
void led_off(led_ch_t ch);
void led_toggle(led_ch_t ch);



#endif /* LED_LED_H_ */

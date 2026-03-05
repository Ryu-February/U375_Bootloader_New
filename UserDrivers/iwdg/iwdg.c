/*
 * iwdg.c
 *
 *  Created on: 2026. 3. 5.
 *      Author: RCY
 */

#include "iwdg.h"
#include "utils.h"



static uint32_t s_prev_millis = 0;
static uint32_t s_isr_alive_ms = 0;
static uint32_t s_last_wdg_kick_ms = 0;


extern IWDG_HandleTypeDef hiwdg;

void iwdg_refresh(void)
{
	uint32_t now = ms_now();
	if (now != s_prev_millis) { s_prev_millis = now; s_isr_alive_ms = now; }
	if ((uint32_t)(now - s_isr_alive_ms) < 100U)
	{
		if ((uint32_t)(now - s_last_wdg_kick_ms) > 50U)
		{
			HAL_IWDG_Refresh(&hiwdg);
//			IWDG->KR = 0xAAAA;	//HAL_IWDG_Refresh와 동일한 코드임(일반 펌웨어에도 있음)
			s_last_wdg_kick_ms = now;
		}
	}
}

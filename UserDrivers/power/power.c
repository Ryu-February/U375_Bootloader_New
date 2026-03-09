/*
 * power.c
 *  Created on: 2026. 3. 6.
 *  Author: RCY
 */

#include "def.h"
#include "utils.h"
#include "power.h"

static uint8_t pa0_pressed_last = 0;
static uint8_t shutdown_pending = 0;
static uint32_t pa0_press_start = 0;
static uint32_t longpress_ms = 500;

static bool wkup1_arm_for_pa0(void)
{
    CLEAR_BIT(PWR->WUCR1, PWR_WUCR1_WUPEN1);

    WRITE_REG(PWR->WUSR, PWR_WUSR_WUF1);

    SET_BIT(PWR->WUCR1, PWR_WUCR1_WUPEN1);

    if (READ_BIT(PWR->WUSR, PWR_WUSR_WUF1)) {
        CLEAR_BIT(PWR->WUCR1, PWR_WUCR1_WUPEN1);
        WRITE_REG(PWR->WUSR, PWR_WUSR_WUF1);
        return false;
    }
    return true;
}

void power_enter_shutdown_safe(void)
{
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) != GPIO_PIN_SET)
        return;

    if (!wkup1_arm_for_pa0())
        return;

    __disable_irq();
    HAL_SuspendTick();
    HAL_PWREx_EnterSHUTDOWNMode();
    while (1) { }
}

void power_enter_standby_safe(void)
{
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) != GPIO_PIN_SET)
        return;

    __disable_irq();
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

    CLEAR_BIT(PWR->WUCR1, PWR_WUCR1_WUPEN1);
    WRITE_REG(PWR->WUSCR, PWR_WUSCR_CWUF1);
    PWR->WUCR3 = (PWR->WUCR3 & ~PWR_WUCR3_WUSEL1_Msk)
               | (0x0U << PWR_WUCR3_WUSEL1_Pos);
    SET_BIT(PWR->WUCR2, PWR_WUCR2_WUPP1);
    WRITE_REG(PWR->WUSCR, PWR_WUSCR_CWUF1);
    SET_BIT(PWR->WUCR1, PWR_WUCR1_WUPEN1);
    if (READ_BIT(PWR->WUSR, PWR_WUSR_WUF1))
    {
        CLEAR_BIT(PWR->WUCR1, PWR_WUCR1_WUPEN1);
        WRITE_REG(PWR->WUSCR, PWR_WUSCR_CWUF1);
        __enable_irq();
        return;
    }

    MODIFY_REG(PWR->CR1, PWR_CR1_LPMS, (0x4U << PWR_CR1_LPMS_Pos));
    SET_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPDEEP_Msk));
    __WFI();
}

void power_init(void)
{
    pa0_pressed_last = 0;
    shutdown_pending = 0;
    pa0_press_start = 0;
    longpress_ms = 500;
}

void power_check_btn(void)
{
	for (uint16_t i = 0; i < 500; i++)	//스위치 0.5초간 안 눌렀으면
	{
		if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) != 0)
		{
			power_enter_shutdown_safe();
		}
		delay_ms(1);
	}
}

void power_poll(void)
{
    uint8_t pa0 = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == 0);
    if (pa0)
    {
        if (!pa0_pressed_last)
        {
            pa0_pressed_last = 1;
            pa0_press_start = ms_now();
        }
        else
        {
            if (!shutdown_pending && (ms_now() - pa0_press_start >= longpress_ms))
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
                power_enter_shutdown_safe();	//얘 이름만 shutdown이고 사실은 standby임
            }
        }
        pa0_pressed_last = 0;
    }
}

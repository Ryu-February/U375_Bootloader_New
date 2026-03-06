/*
 * btn.c
 *
 *  Created on: 2026. 3. 5.
 *      Author: RCY
 */


#include "btn.h"
#include "utils.h"




// PB1에 Delete 스위치 (프로젝트 기준)
#define LP_STBY_BTN_PORT     GPIOA
#define LP_STBY_BTN_PIN      GPIO_PIN_0


static inline bool btn_pressed_raw(void)
{
    // Active-Low: 눌림 → LOW
    return (HAL_GPIO_ReadPin(LP_STBY_BTN_PORT, LP_STBY_BTN_PIN) == GPIO_PIN_RESET);
}



void lp_stby_prepare_before(void)
{
    // 사용자가 오버라이드:
    // - 모터/버저/LED OFF
    // - 상태 저장(Flash/EEPROM)
    // - 주변장치 안전 종료
}

static bool wkup1_arm_for_pa0(void)
{
    // 0) Disable
    CLEAR_BIT(PWR->WUCR1, PWR_WUCR1_WUPEN1);

    // 1) 소스 선택: WUCR3.WUSEL1을 PA0에 맞는 값으로 설정
//    MODIFY_REG(PWR->WUCR3, PWR_WUCR3_WUSEL1_Msk,
//               (WKUP1_SEL_FOR_PA0 << PWR_WUCR3_WUSEL1_Pos));

    // 2) 플래그 클리어
    WRITE_REG(PWR->WUSR, PWR_WUSR_WUF1);

    // 3) Enable
    SET_BIT(PWR->WUCR1, PWR_WUCR1_WUPEN1);

    // 4) Enable 직후 플래그 체크 → 활성로 보이면 취소
    if (READ_BIT(PWR->WUSR, PWR_WUSR_WUF1)) {
        CLEAR_BIT(PWR->WUCR1, PWR_WUCR1_WUPEN1);
        WRITE_REG(PWR->WUSR, PWR_WUSR_WUF1);
        return false;
    }
    return true;
}

void enter_standby_safe(void)
{
    // 릴리즈(High) 최종 소프트 체크
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) != GPIO_PIN_SET)
    	return;

    lp_stby_prepare_before();

    if (!wkup1_arm_for_pa0())
    	return;     // 활성로 보이면 진입 취소

    __disable_irq();
    HAL_SuspendTick();
//    HAL_PWR_EnterSTANDBYMode();
    HAL_PWREx_EnterSHUTDOWNMode();
    while (1) { }
}

void enter_shutdown_safe(void)
{
	__disable_irq();
	SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

	/* Arm WKUP1 (PA0) safely */
	CLEAR_BIT(PWR->WUCR1, PWR_WUCR1_WUPEN1);
	WRITE_REG(PWR->WUSCR, PWR_WUSCR_CWUF1);
	PWR->WUCR3 = (PWR->WUCR3 & ~PWR_WUCR3_WUSEL1_Msk)
	           | (0x0U << PWR_WUCR3_WUSEL1_Pos);
	/* Example: Falling-edge active (for active-low button) */
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

	/* Enter Standby mode */
	MODIFY_REG(PWR->CR1, PWR_CR1_LPMS, (0x4U << PWR_CR1_LPMS_Pos));
	SET_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPDEEP_Msk));
	__WFI();
}



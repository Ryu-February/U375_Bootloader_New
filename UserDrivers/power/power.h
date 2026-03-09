/*
 * power.h
 *
 *  Created on: 2026. 3. 6.
 *      Author: RCY
 */

#ifndef POWER_POWER_H_
#define POWER_POWER_H_

void power_enter_standby_safe(void);
void power_enter_shutdown_safe(void);
void power_init(void);
void power_poll(void);
void power_check_btn(void);

#endif /* POWER_POWER_H_ */

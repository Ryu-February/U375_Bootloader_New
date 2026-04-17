/*
 * swv.c
 *
 *  Created on: 2026. 4. 17.
 *      Author: RCY
 */

#include "swv.h"
#include <stdarg.h>
#include <stdio.h>

int _write(int file, char *ptr, int len)
{
	(void)file;
	for (int i = 0; i < len; i++)
	{
		ITM_SendChar(*ptr++);
	}
	return len;
}

int swv_printf(const char *fmt, ...)
{
	char buf[256];
	va_list args;

	va_start(args, fmt);
	int len = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	for (int i = 0; i < len; i++)
	{
		ITM_SendChar(buf[i]);
	}
	return len;
}

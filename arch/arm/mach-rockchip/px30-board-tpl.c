// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2019 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <debug_uart.h>
#include <dm.h>
#include <init.h>
#include <ram.h>
#include <spl.h>
#include <asm/io.h>
#include <asm/arch-rockchip/bootrom.h>
#include <asm/arch-rockchip/sdram_px30.h>

void board_init_f(ulong dummy)
{
	int ret;

#if defined(CONFIG_DEBUG_UART) && defined(CONFIG_TPL_SERIAL)
	debug_uart_init();
	/*
	 * Debug UART can be used from here if required:
	 *
	 * debug_uart_init();
	 * printch('a');
	 * printhex8(0x1234);
	 * printascii("string");
	 */
#if CONFIG_TPL_BANNER_PRINT
	printascii("U-Boot TPL board init\n");
#endif
#endif

	secure_timer_init();
	ret = sdram_init();
	if (ret)
		printascii("sdram_init failed\n");

	/* return to maskrom */
	back_to_bootrom(BROM_BOOT_NEXTSTAGE);
}

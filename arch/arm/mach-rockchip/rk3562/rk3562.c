// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2024 Benetti Engineering sas
 * Author: Giulio Benetti <giulio.benetti@benettiengineering.com>
 */

#include <common.h>
#include <dm.h>
#include <asm/armv8/mmu.h>
#include <asm/io.h>
#include <asm/arch-rockchip/bootrom.h>
#include <asm/arch-rockchip/grf_rk3562.h>
#include <asm/arch-rockchip/hardware.h>
#include <dt-bindings/clock/rk3562-cru.h>

#define PMUGRF_BASE			0xFF010000
#define GPIO0_IOC_BASE			0xFF080000
#define GPIO1_IOC_BASE			0xFF060000

#define FIREWALL_DDR_BASE	0xfef00000
#define FW_DDR_MST4_REG		0x30	/* emmc */
#define FW_DDR_MST6_REG		0x38	/* sdmmc mcu */

#define PMU_BASE_ADDR		0xFF250000
#define PMU2_BIU_AUTO_CON0	(0x0130)
#define SYS_GRF_BASE		0xFF030000
#define GRF_SYS_CPU_PVTPLL_CON0	(0x0620)

/* GPIO0_IOC_GPIO0D_IOMUX_SEL_L */
#define GPIO0D1_SEL_SHIFT	4
#define GPIO0D1_SEL_MASK	GENMASK(6, GPIO0D1_SEL_SHIFT)
#define GPIO0D1_SEL_UART0_TXM0	1

#define GPIO0D0_SEL_SHIFT	0
#define GPIO0D0_SEL_MASK	GENMASK(2, GPIO0D0_SEL_SHIFT)
#define GPIO0D0_SEL_UART0_RXM0	1

#define GPIO_DS_DISABLE		(0x00)
#define GPIO_DS_LEVEL_0		(0x01)
#define GPIO_DS_LEVEL_1		(0x03)
#define GPIO_DS_LEVEL_2		(0x07)
#define GPIO_DS_LEVEL_3		(0x0f)
#define GPIO_DS_LEVEL_4		(0x1f)
#define GPIO_DS_LEVEL_5		(0x3f)

/* GPIOx_IOC_GPIOxx_DSx */
#define GPIO_SEL_SHIFT_0	0
#define GPIO_SEL_MASK_0		GENMASK(5, GPIO_SEL_SHIFT_0)

#define GPIO_SEL_SHIFT_1	8
#define GPIO_SEL_MASK_1		GENMASK(13, GPIO_SEL_SHIFT_1)

#define GPIO1A0_SEL_SHIFT	GPIO_SEL_SHIFT_0
#define GPIO1A0_SEL_MASK	GPIO_SEL_MASK_0
#define GPIO1A1_SEL_SHIFT	GPIO_SEL_SHIFT_1
#define GPIO1A1_SEL_MASK	GPIO_SEL_MASK_1
#define GPIO1A2_SEL_SHIFT	GPIO_SEL_SHIFT_0
#define GPIO1A2_SEL_MASK	GPIO_SEL_MASK_0
#define GPIO1A3_SEL_SHIFT	GPIO_SEL_SHIFT_1
#define GPIO1A3_SEL_MASK	GPIO_SEL_MASK_1
#define GPIO1A4_SEL_SHIFT	GPIO_SEL_SHIFT_0
#define GPIO1A4_SEL_MASK	GPIO_SEL_MASK_0
#define GPIO1A5_SEL_SHIFT	GPIO_SEL_SHIFT_1
#define GPIO1A5_SEL_MASK	GPIO_SEL_MASK_1
#define GPIO1A6_SEL_SHIFT	GPIO_SEL_SHIFT_0
#define GPIO1A6_SEL_MASK	GPIO_SEL_MASK_0
#define GPIO1A7_SEL_SHIFT	GPIO_SEL_SHIFT_1
#define GPIO1A7_SEL_MASK	GPIO_SEL_MASK_1

#define GPIO1B0_SEL_SHIFT	GPIO_SEL_SHIFT_0
#define GPIO1B0_SEL_MASK	GPIO_SEL_MASK_0
#define GPIO1B1_SEL_SHIFT	GPIO_SEL_SHIFT_1
#define GPIO1B1_SEL_MASK	GPIO_SEL_MASK_1
#define GPIO1B2_SEL_SHIFT	GPIO_SEL_SHIFT_0
#define GPIO1B2_SEL_MASK	GPIO_SEL_MASK_0
#define GPIO1B3_SEL_SHIFT	GPIO_SEL_SHIFT_1
#define GPIO1B3_SEL_MASK	GPIO_SEL_MASK_1
#define GPIO1B4_SEL_SHIFT	GPIO_SEL_SHIFT_0
#define GPIO1B4_SEL_MASK	GPIO_SEL_MASK_0
#define GPIO1B5_SEL_SHIFT	GPIO_SEL_SHIFT_1
#define GPIO1B5_SEL_MASK	GPIO_SEL_MASK_1
#define GPIO1B6_SEL_SHIFT	GPIO_SEL_SHIFT_0
#define GPIO1B6_SEL_MASK	GPIO_SEL_MASK_0
#define GPIO1B7_SEL_SHIFT	GPIO_SEL_SHIFT_1
#define GPIO1B7_SEL_MASK	GPIO_SEL_MASK_1

#define GPIO1C0_SEL_SHIFT	GPIO_SEL_SHIFT_0
#define GPIO1C0_SEL_MASK	GPIO_SEL_MASK_0

static struct mm_region rk3562_mem_map[] = {
	{
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0xf0000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		.virt = 0xf0000000UL,
		.phys = 0xf0000000UL,
		.size = 0x10000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		.virt = 0x300000000,
		.phys = 0x300000000,
		.size = 0x0c0c00000,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* List terminator */
		0,
	}
};

const char * const boot_devices[BROM_LAST_BOOTSOURCE + 1] = {
	[BROM_BOOTSOURCE_EMMC] = "/mmc@ff870000",
	[BROM_BOOTSOURCE_SPINOR] = "/spi@ff860000/flash@0",
	[BROM_BOOTSOURCE_SD] = "/mmc@ff880000",
};

struct mm_region *mem_map = rk3562_mem_map;

void board_debug_uart_init(void)
{
	static struct rk3562_gpio0_ioc * const gpio0_ioc = (void *)GPIO0_IOC_BASE;

	/* Switch iomux to UART0 M0 */
	rk_clrsetreg(&gpio0_ioc->gpio0d_iomux_l,
		     GPIO0D1_SEL_MASK | GPIO0D0_SEL_MASK,
		     GPIO0D1_SEL_UART0_TXM0 << GPIO0D1_SEL_SHIFT |
		     GPIO0D0_SEL_UART0_RXM0 << GPIO0D0_SEL_SHIFT);
}

int arch_cpu_init(void)
{
#ifdef CONFIG_SPL_BUILD
	static struct rk3562_gpio1_ioc * const gpio1_ioc = (void *)GPIO1_IOC_BASE;
	u32 val;

	/*
	 * When perform idle operation, corresponding clock can
	 * be opened or gated automatically.
	 */
	writel(0x5fff5fff, PMU_BASE_ADDR + PMU2_BIU_AUTO_CON0);

	/* Set core pvtpll ring length to 60 and enable it */
	writel(0x00050003, SYS_GRF_BASE + GRF_SYS_CPU_PVTPLL_CON0);

	/* Set the emmc to access ddr memory */
	val = readl(FIREWALL_DDR_BASE + FW_DDR_MST4_REG);
	writel(val & 0x0000ffff, FIREWALL_DDR_BASE + FW_DDR_MST4_REG);

	/* Set the sdmmc to access ddr memory */
	val = readl(FIREWALL_DDR_BASE + FW_DDR_MST6_REG);
	writel(val & 0xff0000ff, FIREWALL_DDR_BASE + FW_DDR_MST6_REG);

	/* set the emmc and sdmmc0 driver strength to level 2 */
	rk_clrsetreg(&gpio1_ioc->gpio1a_ds_0,
		     GPIO1A0_SEL_MASK | GPIO1A1_SEL_MASK,
		     (GPIO_DS_LEVEL_2) << GPIO1A0_SEL_SHIFT |
		     (GPIO_DS_LEVEL_2) << GPIO1A1_SEL_SHIFT);
	rk_clrsetreg(&gpio1_ioc->gpio1a_ds_1,
		     GPIO1A2_SEL_MASK | GPIO1A3_SEL_MASK,
		     (GPIO_DS_LEVEL_2) << GPIO1A2_SEL_SHIFT |
		     (GPIO_DS_LEVEL_2) << GPIO1A3_SEL_SHIFT);
	rk_clrsetreg(&gpio1_ioc->gpio1a_ds_2,
		     GPIO1A4_SEL_MASK | GPIO1A5_SEL_MASK,
		     (GPIO_DS_LEVEL_2) << GPIO1A4_SEL_SHIFT |
		     (GPIO_DS_LEVEL_2) << GPIO1A5_SEL_SHIFT);
	rk_clrsetreg(&gpio1_ioc->gpio1a_ds_3,
		     GPIO1A6_SEL_MASK | GPIO1A7_SEL_MASK,
		     (GPIO_DS_LEVEL_2) << GPIO1A6_SEL_SHIFT |
		     (GPIO_DS_LEVEL_2) << GPIO1A7_SEL_SHIFT);
	rk_clrsetreg(&gpio1_ioc->gpio1b_ds_0,
		     GPIO1B0_SEL_MASK | GPIO1B1_SEL_MASK,
		     (GPIO_DS_LEVEL_2) << GPIO1B0_SEL_SHIFT |
		     (GPIO_DS_LEVEL_2) << GPIO1B1_SEL_SHIFT);
	rk_clrsetreg(&gpio1_ioc->gpio1b_ds_1,
		     GPIO1B2_SEL_MASK | GPIO1B3_SEL_MASK,
		     (GPIO_DS_LEVEL_2) << GPIO1B2_SEL_SHIFT |
		     (GPIO_DS_LEVEL_2) << GPIO1B3_SEL_SHIFT);
	rk_clrsetreg(&gpio1_ioc->gpio1b_ds_2,
		     GPIO1B4_SEL_MASK | GPIO1B5_SEL_MASK,
		     (GPIO_DS_LEVEL_2) << GPIO1B4_SEL_SHIFT |
		     (GPIO_DS_LEVEL_2) << GPIO1B5_SEL_SHIFT);
	rk_clrsetreg(&gpio1_ioc->gpio1b_ds_3,
		     GPIO1B6_SEL_MASK | GPIO1B7_SEL_MASK,
		     (GPIO_DS_LEVEL_2) << GPIO1B6_SEL_SHIFT |
		     (GPIO_DS_LEVEL_2) << GPIO1B7_SEL_SHIFT);
	rk_clrsetreg(&gpio1_ioc->gpio1c_ds_0,
		     GPIO1C0_SEL_MASK,
		     (GPIO_DS_LEVEL_2) << GPIO1C0_SEL_SHIFT);
#endif
	return 0;
}

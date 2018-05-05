/*
 * Copyright 2008 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <config.h>
#include <command.h>
#include <asm/arch/timer.h>
#include <sprite.h>

DECLARE_GLOBAL_DATA_PTR;

static int do_interrupts(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{

	if (argc != 2)
		return CMD_RET_USAGE;

	/* on */
	if (strncmp(argv[1], "on", 2) == 0)
		enable_interrupts();
	else
		disable_interrupts();

	return 0;
}

U_BOOT_CMD(
	interrupts, 5, 0, do_interrupts,
	"enable or disable interrupts",
	"[on, off]"
);

/* Implemented in $(CPU)/interrupts.c */
int do_irqinfo (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

U_BOOT_CMD(
	irqinfo,    1,    1,     do_irqinfo,
	"print information about IRQs",
	""
);

struct timer_list timer0_t;
struct timer_list timer1_t;
static int timer_test_flag[2];
extern int sprite_led_init(void);
extern int sprite_led_exit(int status);

static  void  timer0_test_func(void *p)
{
	struct timer_list *timer_t;

	timer_t = (struct timer_list *)p;
	debug("timer number = %d\n", timer_t->timer_num);
	printf("this is timer test\n");

	del_timer(timer_t);
	timer_test_flag[0] = 0;

	return;
}

int do_timer_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int  base_count = 1000;

	if(timer_test_flag[0])
	{
		printf("can not test timer 0 now\n");

		return -1;
	}

	if(argc == 2)
	{
		base_count = simple_strtol(argv[1], NULL, 10);
	}
	timer0_t.data = (unsigned long)&timer0_t;
	timer0_t.expires = base_count;
	timer0_t.function = timer0_test_func;

	init_timer(&timer0_t);
	add_timer(&timer0_t);
	timer_test_flag[0] = 1;

	return 0;
}


U_BOOT_CMD(
	timer_test, 2, 0, do_timer_test,
	"do a timer and int test",
	"[delay time]"
);

static  void  timer1_test_func(void *p)
{
	struct timer_list *timer_t;

	timer_t = (struct timer_list *)p;
	debug("timer number = %d\n", timer_t->timer_num);
	printf("this is timer test\n");

	del_timer(timer_t);
	timer_test_flag[1] = 0;

	return;
}


int do_timer_test1(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int  base_count = 1000;

	if(timer_test_flag[1])
	{
		printf("can not test timer 1 now\n");

		return -1;
	}

	if(argc == 2)
	{
		base_count = simple_strtol(argv[1], NULL, 10);
	}
	timer1_t.data = (unsigned long)&timer1_t;
	timer1_t.expires = base_count;
	timer1_t.function = timer1_test_func;

	init_timer(&timer1_t);
	add_timer(&timer1_t);

	timer_test_flag[1] = 1;

	return 0;
}


U_BOOT_CMD(
	timer_test1, 2, 0, do_timer_test1,
	"do a timer and int test",
	"[delay time]"
);




#ifndef CONFIG_SUNXI_SPINOR_PLATFORM
int sunxi_usb_dev_register(uint dev_name);
void sunxi_usb_main_loop(int mode);
int sunxi_card_sprite_main(int workmode, char *name);





int do_sprite_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret;
	printf("work mode=0x%x\n", uboot_spare_head.boot_data.work_mode);
	if(uboot_spare_head.boot_data.work_mode == WORK_MODE_USB_PRODUCT)
	{
		printf("run usb efex\n");
		if(sunxi_usb_dev_register(2))
		{
			printf("sunxi usb test: invalid usb device\n");
		}
		sunxi_usb_main_loop(2500);
	}
	else if(uboot_spare_head.boot_data.work_mode == WORK_MODE_CARD_PRODUCT)
	{
		printf("run card sprite\n");
		sprite_led_init();
		ret = sunxi_card_sprite_main(0, NULL);
		sprite_led_exit(ret);
		return ret;
	}
	else if(uboot_spare_head.boot_data.work_mode == WORK_MODE_USB_DEBUG)
	{
		unsigned int val;

		printf("run usb debug\n");
		if(sunxi_usb_dev_register(2))
		{
			printf("sunxi usb test: invalid usb device\n");
		}

		asm("mrc p15, 0, %0, c1, c0, 0	@ get CR" : "=r" (val) : : "cc");
		val &= ~(1<<2);
		asm volatile("mcr p15, 0, %0, c1, c0, 0	@ set CR" : : "r" (val) : "cc");

		sunxi_usb_main_loop(0);
	}
	else if(uboot_spare_head.boot_data.work_mode == WORK_MODE_SPRITE_RECOVERY)
	{
		printf("run sprite recovery\n");
		sprite_led_init();
		ret = sprite_form_sysrecovery();
		sprite_led_exit(ret);
		return ret;
	}
	else
	{
		printf("others\n");
	}

	return 0;
}

U_BOOT_CMD(
	sprite_test, 2, 0, do_sprite_test,
	"do a sprite test",
	"NULL"
);



int do_fastboot_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("run usb fastboot\n");
	if(sunxi_usb_dev_register(3))
	{
		printf("sunxi usb test: invalid usb device\n");
	}
	sunxi_usb_main_loop(0);

	return 0;
}


U_BOOT_CMD(
	fastboot_test, 2, 0, do_fastboot_test,
	"do a sprite test",
	"NULL"
);

int do_mass_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("run usb mass\n");
	if(sunxi_usb_dev_register(1))
	{
		printf("sunxi usb test: invalid usb device\n");
	}
	sunxi_usb_main_loop(0);

	return 0;
}


U_BOOT_CMD(
	mass_test, 2, 0, do_mass_test,
	"do a usb mass test",
	"NULL"
);

int do_efex_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("run usb efex_test\n");
	if(sunxi_usb_dev_register(5))
	{
		printf("sunxi usb test: invalid usb device\n");
	}
	sunxi_usb_main_loop(2500);

	return 0;
}
U_BOOT_CMD(
	efex_test, 2, 0, do_efex_test,
	"do a usb efex test",
	"NULL"
);
#endif

int do_memcpy_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint  size = 64 * 1024 * 1024;

	tick_printf("memcpy test start\n");
	memcpy((void *)MEMCPY_TEST_DST, (void *)MEMCPY_TEST_SRC, size);
	tick_printf("memcpy test end\n");

	return 0;
}


U_BOOT_CMD(
	memcpy_test, 2, 0, do_memcpy_test,
	"do a memcpy test",
	"NULL"
);

int do_delay_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	tick_printf("delay 1000ms\n");
	__msdelay(1000);
	tick_printf("delay test end\n");

	tick_printf("delay 1000ms\n");
	__usdelay(1000 * 1000);
	tick_printf("delay test end\n");

	return 0;
}


U_BOOT_CMD(
	delay_test, 2, 0, do_delay_test,
	"do a delay test",
	"NULL"
);


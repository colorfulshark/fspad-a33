#include <common.h>
#include <spare_head.h>
#include <sunxi_mbr.h>
#include <boot_type.h>
#include <sys_partition.h>
#include <sys_config.h>
#include <sunxi_board.h>
#include <fastboot.h>

DECLARE_GLOBAL_DATA_PTR;

#define PARTITION_SETS_MAX_SIZE	 1024


int sunxi_board_restart(int next_mode)
{
#if 0
	if(!next_mode)
	{
		next_mode = PMU_PRE_SYS_MODE;
	}
	printf("set next mode %d\n", next_mode);
	axp_set_next_poweron_status(next_mode);
	board_display_set_exit_mode(0);
#ifdef CONFIG_SUNXI_DISPLAY
	drv_disp_exit();
#endif
	sunxi_board_close_source();
	reset_cpu(0);
#endif
	return 0;
}

int sunxi_board_shutdown(void)
{
	#if 0
#if defined(CONFIG_SUNXI_RTC)
	printf("rtc disable\n");
    rtc_disable();
#endif
	printf("set next system normal\n");
	axp_set_next_poweron_status(0x0);

	board_display_set_exit_mode(0);
#ifdef CONFIG_SUNXI_DISPLAY
    drv_disp_exit();
#endif
	sunxi_flash_exit(1);	//强制关闭FLASH
	sunxi_sprite_exit(1);
	disable_interrupts();
	interrupt_exit();

	tick_printf("power off\n");
	axp_set_hardware_poweroff_vol();
	axp_set_power_off();
#if defined(CONFIG_ARCH_SUN8IW7P1)
	power_off();
#endif
#endif

	return 0;

}

int sunxi_board_run_fel(void)
{
	#if 0
#if defined(CONFIG_SUN6I) || defined(CONFIG_ARCH_SUN8IW3P1)|| defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN7I)|| defined(CONFIG_ARCH_SUN8IW8P1)
	*((volatile unsigned int *)(SUNXI_RUN_EFEX_ADDR)) = SUNXI_RUN_EFEX_FLAG;
#elif defined(CONFIG_ARCH_SUN9IW1P1) || defined(CONFIG_ARCH_SUN8IW7P1) || defined(CONFIG_ARCH_SUN8IW6P1)
	sunxi_set_fel_flag();
#endif
	printf("set next system status\n");

	axp_set_next_poweron_status(PMU_PRE_SYS_MODE);

	board_display_set_exit_mode(0);
#ifdef CONFIG_SUNXI_DISPLAY
	drv_disp_exit();
#endif
	printf("sunxi_board_close_source\n");
	sunxi_board_close_source();

	sunxi_flush_allcaches();
#if defined(CONFIG_ARCH_SUN5I)
	printf("jump to fel_base\n");
	jump_to(FEL_BASE);
#else
	printf("reset cpu\n");

	reset_cpu(0);
#endif
#endif
	return 0;
}



void sunxi_update_subsequent_processing(int next_work)
{
	printf("next work %d\n", next_work);
	switch(next_work)
	{
		case SUNXI_UPDATE_NEXT_ACTION_REBOOT:	//重启
		case SUNXI_UPDATA_NEXT_ACTION_SPRITE_TEST:
		printf("SUNXI_UPDATE_NEXT_ACTION_REBOOT\n");
			//sunxi_board_restart(0);
			break;
			
		case SUNXI_UPDATE_NEXT_ACTION_SHUTDOWN:	//关机
			printf("SUNXI_UPDATE_NEXT_ACTION_SHUTDOWN\n");
			//sunxi_board_shutdown();
			break;
			
		case SUNXI_UPDATE_NEXT_ACTION_REUPDATE:
			printf("SUNXI_UPDATE_NEXT_ACTION_REUPDATE\n");
			//sunxi_board_run_fel();			//进行量产
			break;
			
		case SUNXI_UPDATE_NEXT_ACTION_BOOT:
		case SUNXI_UPDATE_NEXT_ACTION_NORMAL:
		default:
			printf("SUNXI_UPDATE_NEXT_ACTION_NULL\n");
			break;
	}

	return ;
}


void sunxi_fastboot_init(void)
{
	struct fastboot_ptentry fb_part;
	int index, part_total;
	char partition_sets[PARTITION_SETS_MAX_SIZE];
	char part_name[16];
	char *partition_index = partition_sets;
	int offset = 0;
	int temp_offset = 0;
	int storage_type = uboot_spare_head.boot_data.storage_type;

	printf("--------fastboot partitions--------\n");
	part_total = sunxi_partition_get_total_num();
	if((part_total <= 0) || (part_total > SUNXI_MBR_MAX_PART_COUNT))
	{
		printf("mbr not exist\n");

		return ;
	}
	printf("-total partitions:%d-\n", part_total);
	printf("%-12s  %-12s  %-12s\n", "-name-", "-start-", "-size-");

	memset(partition_sets, 0, PARTITION_SETS_MAX_SIZE);

	for(index = 0; index < part_total && index < SUNXI_MBR_MAX_PART_COUNT; index++)
	{
		sunxi_partition_get_name(index, &fb_part.name[0]);
		fb_part.start = sunxi_partition_get_offset(index) * 512;
		fb_part.length = sunxi_partition_get_size(index) * 512;
		fb_part.flags = 0;
		printf("%-12s: %-12x  %-12x\n", fb_part.name, fb_part.start, fb_part.length);

		memset(part_name, 0, 16);
		if(!storage_type)
		{
			sprintf(part_name, "nand%c", 'a' + index);
		}
		else
		{
			if(index == 0)
			{
				strcpy(part_name, "mmcblk0p2");
			}
			else if( (index+1)==part_total)
			{
				strcpy(part_name, "mmcblk0p1");
			}
			else
			{
				sprintf(part_name, "mmcblk0p%d", index + 4);
			}
		}

		temp_offset = strlen(fb_part.name) + strlen(part_name) + 2;
		if(temp_offset >= PARTITION_SETS_MAX_SIZE)
		{
			printf("partition_sets is too long, please reduces partition name\n");
			break;
		}
		//fastboot_flash_add_ptn(&fb_part);
		sprintf(partition_index, "%s@%s:", fb_part.name, part_name);
		offset += temp_offset;
		partition_index = partition_sets + offset;
	}

	partition_sets[offset-1] = '\0';
	partition_sets[PARTITION_SETS_MAX_SIZE - 1] = '\0';
	printf("-----------------------------------\n");

	setenv("partitions", partition_sets);
}

int check_android_misc(void)
{
	return 0;
}


int board_late_init(void)
{
	sunxi_fastboot_init();

#ifdef  CONFIG_ARCH_HOMELET
	respond_physical_key_action();
#endif
#ifndef CONFIG_SUNXI_SPINOR_PLATFORM
	check_android_misc();
#endif
#ifdef  CONFIG_ARCH_HOMELET
	update_user_data();
#endif
	return 0;
}


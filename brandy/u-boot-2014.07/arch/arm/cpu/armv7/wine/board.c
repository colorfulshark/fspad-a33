/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
//#include <asm/armv7.h>
#include <asm/io.h>
#include <asm/arch/timer.h>
#include <asm/arch/ccmu.h>
#include <asm/arch/clock.h>
#include <asm/arch/sid.h>
#include <asm/arch/platform.h>
#include <asm/arch/sys_proto.h>
#include <boot_type.h>
#include <sys_partition.h>
#include <sys_config.h>
/* The sunxi internal brom will try to loader external bootloader
 * from mmc0, nannd flash, mmc2.
 * We check where we boot from by checking the config
 * of the gpio pin.
 */
DECLARE_GLOBAL_DATA_PTR;


/* do some early init */
void s_init(void)
{
	watchdog_disable();
}

void reset_cpu(ulong addr)
{
	watchdog_enable();
	while(1);
}

void v7_outer_cache_enable(void)
{
	return ;
}

void v7_outer_cache_inval_all(void)
{
	return ;
}

void v7_outer_cache_flush_range(u32 start, u32 stop)
{
	return ;
}

void enable_caches(void)
{
	icache_enable();
	dcache_enable();
}

void disable_caches(void)
{
	icache_disable();
	dcache_disable();
}

int display_inner(void)
{
	printf("version: %s\n", uboot_spare_head.boot_head.version);

	return 0;
}

int script_init(void)
{
	uint offset, length;
	char *addr;

	offset = uboot_spare_head.boot_head.uboot_length;
	length = uboot_spare_head.boot_head.length - uboot_spare_head.boot_head.uboot_length;

	addr   = (char *)CONFIG_SYS_TEXT_BASE + offset;

	if(length == 0)
	{
		script_parser_init(NULL);
		return 0;
	}
	
	if (!(gd->flags & GD_FLG_RELOC))
	{
		script_parser_init((char *)addr);
	}
	else
	{
		script_parser_init((char *)(gd->script_reloc_buf));
	}

	#if 0  //just for test
	{
	int dcdc_vol = 0;
	script_parser_fetch("power_sply", "dcdc2_vol", &dcdc_vol, 1);
	printf("----script test ---- dcdc2_vol = %d\n",dcdc_vol);
	}
	#endif

	

	return 0;
}


int power_source_init(void)
{
    int pll_cpux;
//	int dcdc3_vol;
//	int dcdc3_vol_d;
//
//	if(axp_probe() > 0)
//	{
//		if(!axp_probe_power_supply_condition())
//		{
//			if(!(uboot_spare_head.boot_data.reserved[0] & 0xff00))	//a7启动
//			{
//				if(script_parser_fetch("power_sply", "dcdc3_vol", &dcdc3_vol, 1))
//				{
//					dcdc3_vol_d = 1200;
//				}
//				else
//				{
//					dcdc3_vol_d = dcdc3_vol%10000;
//				}
//				printf("try to set dcdc3 to %d mV\n", dcdc3_vol_d);
//				if(!axp_set_supply_status(0, PMU_SUPPLY_DCDC3, dcdc3_vol_d, -1))
//				{
//					tick_printf("PMU: dcdc3 %d\n", dcdc3_vol_d);
//				}
//				else
//				{
//					printf("axp_set_dcdc3 fail\n");
//				}
//			}
//			else												//a15启动
//			{
//				int table_num, core_vol;
//
//				table_num = sunxi_get_soc_bin();				//找到对应cpu版本的vf表
//				/*scan cpu VF table*/                           //根据给定频率，找到对应电压
//				core_vol = sunxi_scan_VF_table(table_num,uboot_spare_head.boot_data.run_clock);
//				if(core_vol <= 0)
//				{
//					printf("can not find the voltage which match the dedicated a15 freq\n");
//				}
//				else            /*set core frequency and vol*/
//			    {
//			    	//set cpuB vol
//				    if(!axp_set_supply_status_byname("axp806","dcdca_vol",core_vol,1))		//调整对应c1的设定电压
//				    {
//				    	tick_printf("PMU: axp806 dcdca %d\n", core_vol);
//				    	core_vol += 1000000;
//				    	script_parser_patch("slave_power_sply","dcdca_vol",&core_vol,1);
//
//			    		sunxi_clock_set_C1corepll(uboot_spare_head.boot_data.run_clock, core_vol);
//				    }
//				    else
//				    {
//				    	printf("axp_set_dcdca fail\n");
//				    }
//			    }
//			}
//		}
//		else
//		{
//			printf("axp_probe_power_supply_condition error\n");
//		}
//	}
//	else
//	{
//		printf("axp_probe error\n");
//	}
//
//	config_cpu_map();
//
    pll_cpux = sunxi_clock_get_corepll();
    tick_printf("PMU: cpux %d Mhz,AXI=%d Mhz\n", pll_cpux,sunxi_clock_get_axi());
    printf("PLL6=%d Mhz,AHB=%d Mhz, APB1=%d Mhz \n", 
        sunxi_clock_get_pll6(),
        sunxi_clock_get_ahb(),
        sunxi_clock_get_apb());

//
//    axp_set_charge_vol_limit();
//    axp_set_all_limit();
//    axp_set_hardware_poweron_vol();
//
//	__disable_unused_mode();
//
//	axp_set_power_supply_output();
//	axp_slave_set_power_supply_output();
//
//	power_config_gpio_bias();
//    power_limit_init();

    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void sunxi_set_fel_flag(void)
{
	volatile uint reg_val;

    do
    {
	    writel((1<<16) | (SUNXI_RUN_EFEX_FLAG<<8), SUNXI_RPRCM_BASE + 0x1f0);
	    writel((1<<16) | (SUNXI_RUN_EFEX_FLAG<<8) | (1U<<31), SUNXI_RPRCM_BASE + 0x1f0);
	    __usdelay(10);
	    asm volatile("DMB SY");
	    writel((1<<16) | (SUNXI_RUN_EFEX_FLAG<<8), SUNXI_RPRCM_BASE + 0x1f0);
	    reg_val = readl(SUNXI_RPRCM_BASE + 0x1f0);
    }
    while((reg_val & 0xff) != SUNXI_RUN_EFEX_FLAG);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void sunxi_clear_fel_flag(void)
{
	volatile uint reg_val;

    do
    {
	    writel((1<<16),            SUNXI_RPRCM_BASE + 0x1f0);
	    writel((1<<16) | (1U<<31), SUNXI_RPRCM_BASE + 0x1f0);
	    __usdelay(10);
	    asm volatile("DMB SY");
	    writel((1<<16), SUNXI_RPRCM_BASE + 0x1f0);
	    reg_val = readl(SUNXI_RPRCM_BASE + 0x1f0);
    }
    while((reg_val & 0xff) != 0);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void sunxi_set_rtc_flag(u8 flag)
{
	volatile uint reg_val;

    do
    {
	    writel((1<<16) | (flag<<8), SUNXI_RPRCM_BASE + 0x1f0);
	    writel((1<<16) | (flag<<8) | (1U<<31), SUNXI_RPRCM_BASE + 0x1f0);
	    __usdelay(10);
	    asm volatile("DMB SY");
	    writel((1<<16) | (flag<<8), SUNXI_RPRCM_BASE + 0x1f0);
	    reg_val = readl(SUNXI_RPRCM_BASE + 0x1f0);
    }
    while((reg_val & 0xff) != flag);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int sunxi_probe_securemode(void)
{
//	uint reg_val;
//
//	reg_val = readl(CCM_PLL1_C0_CTRL);
//	if(!reg_val)  //读到数据全是0，那么只能是使能secure的normal模式
//	{
//		if(uboot_spare_head.boot_data.secureos_exist==1)	//如果是1，由sbromsw传递，表示存在安全系统，否则没有
//		{
//			gd->securemode = SUNXI_SECURE_MODE_WITH_SECUREOS;
//			printf("secure mode: with secureos\n");
//		}
//		else
//		{
//			gd->securemode = SUNXI_SECURE_MODE_NO_SECUREOS;		//不存在安全系统
//			printf("secure mode: no secureos\n");
//		}
//	}
//	else		 //读到数据非0，那么只能是未使能secure
//	{
//		gd->securemode = SUNXI_NORMAL_MODE;
//		printf("normal mode\n");
//	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int sunxi_set_secure_mode(void)
{
//	int mode;
//
//	if(gd->securemode == SUNXI_NORMAL_MODE)
//	{
//		mode = sid_probe_security_mode();
//		if(!mode)
//		{
//			sid_set_security_mode();
//		}
//	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int sunxi_get_securemode(void)
{
	return gd->securemode;
}


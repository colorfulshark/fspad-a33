/*
************************************************************************************************************************
*                                          Boot rom
*                                         Seucre Boot
*
*                             Copyright(C), 2006-2013, AllWinners Microelectronic Co., Ltd.
*											       All Rights Reserved
*
* File Name   : Base.h
*
* Author      : glhuang
*
* Version     : 0.0.1
*
* Date        : 2013.09.05
*
* Description :
*
* Others      : None at present.
*
*
* History     :
*
*  <Author>        <time>       <version>      <description>
*
* glhuang       2013.09.05       0.0.1        build the file
*
************************************************************************************************************************
*/
#include "common.h"
#include "sbrom_toc.h"
#include "boot_type.h"
#include "openssl_ext.h"
#include "asm/arch/clock.h"
#include "asm/arch/ss.h"
#include "asm/arch/timer.h"
#include "asm/arch/uart.h"
#include "asm/arch/rtc_region.h"
#include "asm/arch/gic.h"
#include "private_toc.h"
#include "sbrom_toc.h"
#include "../libs/sbrom_libs.h"
#include <asm/arch/dram.h>
#ifdef CONFIG_BOOT_A15
#include <asm/arch/cpu_switch.h>
#endif

static int sbromsw_toc1_traverse(void);
static int sbromsw_probe_fel_flag(void);
static int sbromsw_clear_env(void);

sbrom_toc0_config_t *toc0_config = (sbrom_toc0_config_t *)CONFIG_TOC0_CONFIG_ADDR;

void sbromsw_entry(void)
{
	toc0_private_head_t *toc0 = (toc0_private_head_t *)CONFIG_SBROMSW_BASE;
	uint dram_size;
	int  ret, flag;

	timer_init();
	set_pll();
	sunxi_serial_init(toc0_config->uart_port, toc0_config->uart_ctrl, 2);
	printf("try to probe rtc region\n");
	flag = rtc_region_probe_fel_flag();
	printf("flag=0x%x\n", flag);
	if(flag == SUNXI_RUN_EFEX_FLAG)
	{
		printf("sbromsw_entry sbromsw_probe_fel_flag\n");
		rtc_region_clear_fel_flag();

		goto __sbromsw_entry_err0;
	}
#ifdef CONFIG_BOOT_A15
//	printf("toc0_config->boot_cpu=0x%x\n", toc0_config->boot_cpu);
//	if(toc0_config->boot_cpu)
//	{
//		flag = BOOT_A7_FLAG;
//	}
//	else
//	{
//		flag = BOOT_A15_FLAG;
//	}
/*
    boot_cpu  ����

	bit0~7                       bit8~15

	0:����Ҫ�����־λ           1:��ǰӦ���л�a15����
	1:֪ͨu-boot����             0:��ǰӦ���л�a7����

	ÿ�δ�brom��ȡ��boot_cpuֻ����0x100����0
*/

	if(flag == BOOT_A15_FLAG)
	{
		rtc_region_clear_fel_flag();
		if(toc0_config->boot_cpu == 0x00)
			toc0_config->boot_cpu = 0x101;   //a15��������Ҫ�����־λ

		switch_to_a15(toc0_config->a15_power_gpio);
	}
	else if(flag == BOOT_A7_FLAG)
	{
		rtc_region_clear_fel_flag();
		if(toc0_config->boot_cpu == 0x100)      //���ԭ����a15����
			toc0_config->boot_cpu = 0x01;       //a7��������Ҫ�����־λ
	}
	else
	{
		if(toc0_config->boot_cpu == 0x100)
		{
			switch_to_a15(toc0_config->a15_power_gpio);                //a15����������Ҫ�����־λ
		}
		else
		{
			toc0_config->boot_cpu = 0x0;    //a7����������Ҫ�����־λ
		}
	}
    //printf("toc0_config->boot_cpu=0x%x\n", toc0_config->boot_cpu);
#endif
	printf("try to setup mmu\n");
	//mmu init
	mmu_setup();
	printf("mmu setup ok\n");
	//dram init
	printf("try to init dram\n");
	dram_size = init_DRAM(0, (void *)toc0_config->dram_para);
	if (dram_size)
	{
		printf("init dram ok, size=%dM\n", dram_size);
	}
	else
	{
		printf("init dram fail\n");

		goto __sbromsw_entry_err;
	}
	printf("init heap\n");
	create_heap(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);
	printf("init gic\n");
	gic_init();
	printf("init flash\n");
	ret = sunxi_flash_init(toc0->platform[0] & 0x0f);		//��ʼ���ⲿ���ʣ�׼����ȡTOC1����
	if(ret)
	{
		printf("sbromsw_entry sunxi_flash_init failed\n");

		goto __sbromsw_entry_err;
	}
	ret = toc1_init();      //TOC1��ʼ�����ж�TOC1��ͷ���Ƿ�ϸ�
	if(ret)
	{
		printf("sbromsw_entry toc1_init failed\n");

		goto __sbromsw_entry_err;
	}
	ret = sbromsw_toc1_traverse();
	if(ret)
	{
		printf("sbromsw_entry sbromsw_toc1_traverse failed\n");

		goto __sbromsw_entry_err;
	}

__sbromsw_entry_err:
#ifdef CONFIG_BOOT_A15
	if(!(toc0_config->boot_cpu & 0xff00))
	{
		sbromsw_clear_env();

		boot0_jump(SUNXI_FEL_ADDR_IN_SECURE);
	}
	else
	{
		rtc_region_set_flag(SUNXI_RUN_EFEX_FLAG);
		sbromsw_clear_env();

		watchdog_enable();
	}
#endif
__sbromsw_entry_err0:
	sbromsw_clear_env();

	boot0_jump(SUNXI_FEL_ADDR_IN_SECURE);
}

#define  SUNXI_X509_CERTIFF_MAX_LEN   (4096)

static int sbromsw_toc1_traverse(void)
{
	sbrom_toc1_item_group item_group;
	int ret;
	uint len, i;
	u8 buffer[SUNXI_X509_CERTIFF_MAX_LEN];

	sunxi_certif_info_t  root_certif;
	sunxi_certif_info_t  sub_certif;
	u8  hash_of_file[256];
	//u8  hash_in_certif[256];

	//u8  key_certif_extension[260];
	//u8  content_certif_key[520];
	int out_to_ns;

	toc1_item_traverse();

	printf("probe root certif\n");
	sunxi_ss_open();

	memset(buffer, 0, SUNXI_X509_CERTIFF_MAX_LEN);
	len = toc1_item_read_rootcertif(buffer, SUNXI_X509_CERTIFF_MAX_LEN);
	if(!len)
	{
		printf("%s error: cant read rootkey certif\n", __func__);

		return -1;
	}
	if(sunxi_certif_verify_itself(&root_certif, buffer, len))
	{
		printf("certif invalid: root certif verify itself failed\n");

		return -1;
	}
	do
	{
		memset(&item_group, 0, sizeof(sbrom_toc1_item_group));
		ret = toc1_item_probe_next(&item_group);
		if(ret < 0)
		{
			printf("sbromsw_toc1_traverse err in toc1_item_probe_next\n");

			return -1;
		}
		else if(ret == 0)
		{
			printf("sbromsw_toc1_traverse find out all items\n");

			return 0;
		}
		if(item_group.bin_certif)
		{
			memset(buffer, 0, SUNXI_X509_CERTIFF_MAX_LEN);
			len = toc1_item_read(item_group.bin_certif, buffer, SUNXI_X509_CERTIFF_MAX_LEN);
			if(!len)
			{
				printf("%s error: cant read content key certif\n", __func__);

				return -1;
			}
			//֤�����ݽ�����У�飬ȷ��û�б��滻
			if(sunxi_certif_verify_itself(&sub_certif, buffer, len))
			{
				printf("%s error: cant verify the content certif\n", __func__);

				return -1;
			}
//			printf("key n:\n");
//			ndump(sub_certif.pubkey.n, sub_certif.pubkey.n_len);
//			printf("key e:\n");
//			ndump(sub_certif.pubkey.e, sub_certif.pubkey.e_len);
			//ÿ������һ����Կ֤�飬���ڸ�֤����Ѱ��ƥ����Ŀ���Ҳ�������Ϊ�д���
			for(i=0;i<root_certif.extension.extension_num;i++)
			{
				if(!strcmp((const char *)root_certif.extension.name[i], item_group.bin_certif->name))
				{
					printf("find %s key stored in root certif\n", item_group.bin_certif->name);

					if(memcmp(root_certif.extension.value[i], sub_certif.pubkey.n+1, sub_certif.pubkey.n_len-1))
					{
						printf("%s key n is incompatible\n", item_group.bin_certif->name);
						printf(">>>>>>>key in rootcertif<<<<<<<<<<\n");
						ndump(root_certif.extension.value[i], sub_certif.pubkey.n_len-1);
						printf(">>>>>>>key in certif<<<<<<<<<<\n");
						ndump(sub_certif.pubkey.n+1, sub_certif.pubkey.n_len-1);

						return -1;
					}
					if(memcmp(root_certif.extension.value[i] + sub_certif.pubkey.n_len-1, sub_certif.pubkey.e, sub_certif.pubkey.e_len))
					{
						printf("%s key e is incompatible\n", item_group.bin_certif->name);
						printf(">>>>>>>key in rootcertif<<<<<<<<<<\n");
						ndump(root_certif.extension.value[i] + sub_certif.pubkey.n_len-1, sub_certif.pubkey.e_len);
						printf(">>>>>>>key in certif<<<<<<<<<<\n");
						ndump(sub_certif.pubkey.e, sub_certif.pubkey.e_len);

						return -1;
					}
					break;
				}
			}
			if(i==root_certif.extension.extension_num)
			{
				printf("cant find %s key stored in root certif", item_group.bin_certif->name);

				return -1;
			}
		}

		if(item_group.binfile)
		{
			//����bin�ļ����ݵ��ڴ�
			len = sunxi_flash_read(item_group.binfile->data_offset/512, (item_group.binfile->data_len+511)/512, (void *)item_group.binfile->run_addr);
			//len = sunxi_flash_read(item_group.binfile->data_offset/512, (item_group.binfile->data_len+511)/512, (void *)0x2a000000);
			if(!len)
			{
				printf("%s error: cant read bin file\n", __func__);

				return -1;
			}
			//�����ļ�hash
			memset(hash_of_file, 0, sizeof(hash_of_file));
			ret = sunxi_sha_calc(hash_of_file, sizeof(hash_of_file), (u8 *)item_group.binfile->run_addr, item_group.binfile->data_len);
			//ret = sunxi_sha_calc(hash_of_file, sizeof(hash_of_file), (u8 *)0x2a000000, item_group.binfile->data_len);
			if(ret)
			{
				printf("sunxi_sha_calc: calc sha256 with hardware err\n");

				return -1;
			}
			//ʹ������֤�����չ����ļ�hash���бȽ�
			//��ʼ�Ƚ��ļ�hash(С���˽׶μ���õ�)��֤��hash(PC�˼���õ�)
			if(memcmp(hash_of_file, sub_certif.extension.value[0], 32))
			{
				printf("hash compare is not correct\n");
				printf(">>>>>>>hash of file<<<<<<<<<<\n");
				ndump(hash_of_file, 32);
				printf(">>>>>>>hash in certif<<<<<<<<<<\n");
				ndump(sub_certif.extension.value[0], 32);

				return -1;
			}

			printf("ready to run %s\n", item_group.binfile->name);
			if(strcmp(item_group.binfile->name, "u-boot"))
			{
				out_to_ns = 0;
			}
			else
			{
				out_to_ns = 1;
			}
			go_exec(item_group.binfile->run_addr, CONFIG_TOC0_CONFIG_ADDR, out_to_ns);
		}
	}
	while(1);

	return 0;
}

static int sbromsw_probe_fel_flag(void)
{
	uint flag;

	flag = rtc_region_probe_fel_flag();
	rtc_region_clear_fel_flag();

	return flag;
}


static int sbromsw_clear_env(void)
{
#if defined(CONFIG_ARCH_SUN9IW1P1)
	mctl_rst_securout();
#endif
	gic_exit();
	reset_pll();
	mmu_turn_off();

	return 0;
}

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
#include <malloc.h>
#include <asm/io.h>
#include <asm/arch/platform.h>
#include <asm/arch/gpio.h>

#include <sys_config.h>

DECLARE_GLOBAL_DATA_PTR;

static script_sub_key_t *sw_cfg_get_subkey(const char *script_buf, const char *main_key, const char *sub_key)
{
    script_head_t *hd = (script_head_t *)script_buf;
    script_main_key_t *mk = (script_main_key_t *)(hd + 1);
    script_sub_key_t *sk = NULL;
    int i, j;

    for (i = 0; i < hd->main_key_count; i++) {

        if (strcmp(main_key, mk->main_name)) {
            mk++;
            continue;
        }

        for (j = 0; j < mk->lenth; j++) {
            sk = (script_sub_key_t *)(script_buf + (mk->offset<<2) + j * sizeof(script_sub_key_t));
            if (!strcmp(sub_key, sk->sub_name)) return sk;
        }
    }
    return NULL;
}

int sw_cfg_get_int(const char *script_buf, const char *main_key, const char *sub_key)
{
    script_sub_key_t *sk = NULL;
    char *pdata;
    int value;

    sk = sw_cfg_get_subkey(script_buf, main_key, sub_key);
    if (sk == NULL) {
        return -1;
    }

    if (((sk->pattern >> 16) & 0xffff) == SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD) {
        pdata = (char *)(script_buf + (sk->offset<<2));
        value = *((int *)pdata);
        return value;
    }

    return -1;
}

char *sw_cfg_get_str(const char *script_buf, const char *main_key, const char *sub_key, char *buf)
{
    script_sub_key_t *sk = NULL;
    char *pdata;

    sk = sw_cfg_get_subkey(script_buf, main_key, sub_key);
    if (sk == NULL) {
        return NULL;
    }

    if (((sk->pattern >> 16) & 0xffff) == SCIRPT_PARSER_VALUE_TYPE_STRING) {
        pdata = (char *)(script_buf + (sk->offset<<2));
        memcpy(buf, pdata, ((sk->pattern >> 0) & 0xffff));
        return (char *)buf;
    }

    return NULL;
}


static  int   _test_str_length(char *str)
{
    int length = 0;

    while(str[length++])
    {
        if(length > 32)
        {
            length = 32;
            break;
        }
    }

    return length;
}

int script_parser_init(char *script_buf)
{
    script_head_t   *script_head;

	if(script_buf)
    {
        gd->script_mod_buf = script_buf;
        script_head = (script_head_t *)script_buf;

        gd->script_main_key_count = script_head->main_key_count;

        return SCRIPT_PARSER_OK;
    }
    else
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }
}

unsigned script_get_length(void)
{
	script_head_t *orign_head = (script_head_t *)gd->script_mod_buf;

	return orign_head->length;
}

int script_parser_exit(void)
{
    gd->script_mod_buf = NULL;
    gd->script_main_key_count = 0;

    return SCRIPT_PARSER_OK;
}

ulong script_parser_subkey( script_main_key_t* main_name,char *subkey_name , uint *pattern)
{
    script_main_key_t *main_key = NULL;
    script_sub_key_t  *sub_key  = NULL;
    int i = 0;

    if((!gd->script_mod_buf)||(gd->script_main_key_count<=0))
    {
        return 0;
    }
    if((main_name == NULL)||(subkey_name == NULL))
    {
        printf("main_name is invalid \n");
        return 0;
    }

    main_key = main_name;
    for(i = 0;i<main_key->lenth;i++)
    {
        sub_key = (script_sub_key_t *)(gd->script_mod_buf + (main_key->offset<<2) + (i * sizeof(script_sub_key_t)));
        if(strcmp(sub_key->sub_name,subkey_name))
            continue;
        *pattern = (sub_key->pattern>>16)&0xffff;
        return (ulong)sub_key;
    }
    return 0;
}

ulong script_parser_fetch_subkey_start(char *main_name)
{
	char   main_bkname[32];
    char   *main_char;
    script_main_key_t  *main_key = NULL;
    int    i;
    /* check params */
    if((!gd->script_mod_buf) || (gd->script_main_key_count <= 0))
    {
        return 0;
    }

    if(main_name == NULL)
    {
		return 0;
    }

    /* truncate string if size >31 bytes */
    main_char = main_name;
    if(_test_str_length(main_name) > 31)
    {
        memset(main_bkname, 0, 32);
        strncpy(main_bkname, main_name, 31);
        main_char = main_bkname;
    }

    for(i=0;i<gd->script_main_key_count;i++)
    {
        main_key = (script_main_key_t *)(gd->script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
        if(strcmp(main_key->main_name, main_char))
        {
            continue;
        }

		return (ulong)main_key;
	}

	return 0;
}


int script_parser_fetch_subkey_next(ulong hd, char *sub_name, int value[], int *index)
{
    script_main_key_t  *main_key;
    script_sub_key_t   *sub_key = NULL;
    int    j;
    int    pattern;

	if(!hd)
	{
		return -1;
	}

	main_key = (script_main_key_t *)hd;
    /* now find sub key */
    for(j = *index; j < main_key->lenth; j++)
    {
    	sub_key = (script_sub_key_t *)(gd->script_mod_buf + (main_key->offset<<2) + (j * sizeof(script_sub_key_t)));

        pattern    = (sub_key->pattern>>16) & 0xffff; /* get datatype */
		if(pattern == SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD)
		{
			value[0] = *(int *)(gd->script_mod_buf + (sub_key->offset<<2));
			strcpy(sub_name, sub_key->sub_name);

			*index = j + 1;

			return SCRIPT_PARSER_OK;
		}
		else if(pattern == SCIRPT_PARSER_VALUE_TYPE_STRING)
		{
			strcpy((void *)value, gd->script_mod_buf + (sub_key->offset<<2));
			strcpy(sub_name, sub_key->sub_name);

			*index = j + 1;

			return SCRIPT_PARSER_OK;
		}
    }

    return SCRIPT_PARSER_KEY_NOT_FIND;
}

int script_parser_fetch(char *main_name, char *sub_name, int value[], int count)
{
    char   main_bkname[32], sub_bkname[32];
    char   *main_char, *sub_char;
    script_main_key_t  *main_key = NULL;
    script_sub_key_t   *sub_key = NULL;
    int    i, j;
    int    pattern, word_count;
    /* check params */
    if((!gd->script_mod_buf) || (gd->script_main_key_count <= 0))
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }

    if((main_name == NULL) || (sub_name == NULL))
    {
		return SCRIPT_PARSER_KEYNAME_NULL;
    }

    if(value == NULL)
    {
		return SCRIPT_PARSER_DATA_VALUE_NULL;
    }

    /* truncate string if size >31 bytes */
    main_char = main_name;
    if(_test_str_length(main_name) > 31)
    {
        memset(main_bkname, 0, 32);
        strncpy(main_bkname, main_name, 31);
        main_char = main_bkname;
    }
    sub_char = sub_name;
    if(_test_str_length(sub_name) > 31)
    {
        memset(sub_bkname, 0, 32);
        strncpy(sub_bkname, sub_name, 31);
        sub_char = sub_bkname;
    }
    for(i=0;i<gd->script_main_key_count;i++)
    {
        main_key = (script_main_key_t *)(gd->script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
        if(strcmp(main_key->main_name, main_char))
        {
            continue;
        }

        /* now find sub key */
        for(j=0;j<main_key->lenth;j++)
        {
            sub_key = (script_sub_key_t *)(gd->script_mod_buf + (main_key->offset<<2) + (j * sizeof(script_sub_key_t)));
            if(strcmp(sub_key->sub_name, sub_char))
            {
                continue;
            }
            pattern    = (sub_key->pattern>>16) & 0xffff; /* get datatype */
            word_count = (sub_key->pattern>> 0) & 0xffff; /*get count of word */

            switch(pattern)
            {
                case SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD:
                    value[0] = *(int *)(gd->script_mod_buf + (sub_key->offset<<2));
                    break;

                case SCIRPT_PARSER_VALUE_TYPE_STRING:
                    if(count < word_count)
                    {
                        word_count = count;
                    }
                    memcpy((char *)value, gd->script_mod_buf + (sub_key->offset<<2), word_count << 2);
                    break;

                case SCIRPT_PARSER_VALUE_TYPE_MULTI_WORD:
                    break;
                case SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD:
                {
                    script_gpio_set_t  *user_gpio_cfg = (script_gpio_set_t *)value;
                    /* buffer space enough? */
                    if(sizeof(script_gpio_set_t) > (count<<2))
                    {
                        return SCRIPT_PARSER_BUFFER_NOT_ENOUGH;
                    }
                    strcpy( user_gpio_cfg->gpio_name, sub_char);
                    memcpy(&user_gpio_cfg->port, gd->script_mod_buf + (sub_key->offset<<2),  sizeof(script_gpio_set_t) - 32);
                    break;
                }
            }

            return SCRIPT_PARSER_OK;
        }
    }

    return SCRIPT_PARSER_KEY_NOT_FIND;
}

int script_parser_fetch_ex(char *main_name, char *sub_name, int value[], script_parser_value_type_t *type, int count)
{
    char   main_bkname[32], sub_bkname[32];
    char   *main_char, *sub_char;
    script_main_key_t  *main_key = NULL;
    script_sub_key_t   *sub_key = NULL;
    int    i, j;
    int    pattern, word_count;
    /* check params */
    if((!gd->script_mod_buf) || (gd->script_main_key_count <= 0))
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }

    if((main_name == NULL) || (sub_name == NULL))
    {
		return SCRIPT_PARSER_KEYNAME_NULL;
    }

    if(value == NULL)
    {
		return SCRIPT_PARSER_DATA_VALUE_NULL;
    }

    /* truncate string if size >31 bytes */
    main_char = main_name;
    if(_test_str_length(main_name) > 31)
    {
        memset(main_bkname, 0, 32);
        strncpy(main_bkname, main_name, 31);
        main_char = main_bkname;
    }
    sub_char = sub_name;
    if(_test_str_length(sub_name) > 31)
    {
        memset(sub_bkname, 0, 32);
        strncpy(sub_bkname, sub_name, 31);
        sub_char = sub_bkname;
    }
    for(i=0;i<gd->script_main_key_count;i++)
    {
        main_key = (script_main_key_t *)(gd->script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
        if(strcmp(main_key->main_name, main_char))
        {
            continue;
        }

        /* now find sub key */
        for(j=0;j<main_key->lenth;j++)
        {
            sub_key = (script_sub_key_t *)(gd->script_mod_buf + (main_key->offset<<2) + (j * sizeof(script_sub_key_t)));
            if(strcmp(sub_key->sub_name, sub_char))
            {
                continue;
            }
            pattern    = (sub_key->pattern>>16) & 0xffff; /* get datatype */
            word_count = (sub_key->pattern>> 0) & 0xffff; /*get count of word */

            switch(pattern)
            {
                case SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD:
                    value[0] = *(int *)(gd->script_mod_buf + (sub_key->offset<<2));
                    *type = SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD;
                    break;

                case SCIRPT_PARSER_VALUE_TYPE_STRING:
                    if(count < word_count)
                    {
                        word_count = count;
                    }
                    memcpy((char *)value, gd->script_mod_buf + (sub_key->offset<<2), word_count << 2);
                    *type = SCIRPT_PARSER_VALUE_TYPE_STRING;
                    break;

                case SCIRPT_PARSER_VALUE_TYPE_MULTI_WORD:
					*type = SCIRPT_PARSER_VALUE_TYPE_MULTI_WORD;
                    break;
                case SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD:
                {
                    script_gpio_set_t  *user_gpio_cfg = (script_gpio_set_t *)value;
                    /* buffer space enough? */
                    if(sizeof(script_gpio_set_t) > (count<<2))
                    {
                        return SCRIPT_PARSER_BUFFER_NOT_ENOUGH;
                    }
                    strcpy( user_gpio_cfg->gpio_name, sub_char);
                    memcpy(&user_gpio_cfg->port, gd->script_mod_buf + (sub_key->offset<<2),  sizeof(script_gpio_set_t) - 32);
                    *type = SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD;
                    break;
                }
            }

            return SCRIPT_PARSER_OK;
        }
    }

    return SCRIPT_PARSER_KEY_NOT_FIND;
}

int script_parser_patch_all(char *main_name, void *str, uint data_count)
{
	script_main_key_t  *main_key = NULL;
	script_sub_key_t   *sub_key = NULL;
	int    i, j;
	int    data_max;
	int    pattern;
	uint   *data = (uint *)str;

	//检查脚本buffer是否存在
	if(!gd->script_mod_buf)
	{
		return SCRIPT_PARSER_EMPTY_BUFFER;
	}
	//检查主键名称和子键名称是否为空
	if(main_name == NULL)
	{
		return SCRIPT_PARSER_KEYNAME_NULL;
	}
	//检查数据buffer是否为空
	if(str == NULL)
	{
		return SCRIPT_PARSER_DATA_VALUE_NULL;
	}
	//保存主键名称和子键名称，如果超过31字节则截取31字节
	for(i=0;i<gd->script_main_key_count;i++)
	{
		main_key = (script_main_key_t *)(gd->script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
		if(strcmp(main_key->main_name, main_name))    //如果主键不匹配，寻找下一个主键
		{
			continue;
		}
		if(data_count > main_key->lenth)
		{
			data_max = main_key->lenth;
		}
		else
		{
			data_max = data_count;
		}
		//主键匹配，寻找子键名称匹配
		for(j=0;j<data_max;j++)
		{
			sub_key = (script_sub_key_t *)(gd->script_mod_buf + (main_key->offset<<2) + (j * sizeof(script_sub_key_t)));

			pattern    = (sub_key->pattern>>16) & 0xffff;             //获取数据的类型
			//取出数据
			if(pattern == SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD)                      //单word数据类型
			{
				*(int *)(gd->script_mod_buf + (sub_key->offset<<2)) = *(int *)data;
				data ++;
    		}
		}

		return 0;
	}

	return -1;
}

int script_parser_patch(char *main_name, char *sub_name, void *str, int str_size)
{
	char   main_bkname[32], sub_bkname[32];
	char   *main_char, *sub_char;
	script_main_key_t  *main_key = NULL;
	script_sub_key_t   *sub_key = NULL;
	int    i, j;
	int    pattern, word_count;

	//检查脚本buffer是否存在
	if(!gd->script_mod_buf)
	{
		return SCRIPT_PARSER_EMPTY_BUFFER;
	}
	//检查主键名称和子键名称是否为空
	if((main_name == NULL) || (sub_name == NULL))
	{
		return SCRIPT_PARSER_KEYNAME_NULL;
	}
	//检查数据buffer是否为空
	if(str == NULL)
	{
		return SCRIPT_PARSER_DATA_VALUE_NULL;
	}
	//保存主键名称和子键名称，如果超过31字节则截取31字节
	main_char = main_name;
	if(_test_str_length(main_name) > 31)
	{
	    memset(main_bkname, 0, 32);
		strncpy(main_bkname, main_name, 31);
		main_char = main_bkname;
	}
    sub_char = sub_name;
	if(_test_str_length(sub_name) > 31)
	{
		memset(sub_bkname, 0, 32);
		strncpy(sub_bkname, sub_name, 31);
		sub_char = sub_bkname;
	}
	for(i=0;i<gd->script_main_key_count;i++)
	{
		main_key = (script_main_key_t *)(gd->script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
		if(strcmp(main_key->main_name, main_char))    //如果主键不匹配，寻找下一个主键
		{
			continue;
		}
		//主键匹配，寻找子键名称匹配
		for(j=0;j<main_key->lenth;j++)
		{
			sub_key = (script_sub_key_t *)(gd->script_mod_buf + (main_key->offset<<2) + (j * sizeof(script_sub_key_t)));
			if(strcmp(sub_key->sub_name, sub_char))    //如果主键不匹配，寻找下一个主键
			{
				continue;
			}
			pattern    = (sub_key->pattern>>16) & 0xffff;             //获取数据的类型
			word_count = (sub_key->pattern>> 0) & 0xffff;             //获取所占用的word个数
			//取出数据
			if(pattern == SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD)                      //单word数据类型
			{
				*(int *)(gd->script_mod_buf + (sub_key->offset<<2)) = *(int *)str;

    			return SCRIPT_PARSER_OK;
    		}
    		else if(pattern == SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD)
    		{
    			script_gpio_set_t  *user_gpio_cfg = (script_gpio_set_t *)str;

    			memset(gd->script_mod_buf + (sub_key->offset<<2), 0, 24);
    			memcpy(gd->script_mod_buf + (sub_key->offset<<2), &user_gpio_cfg->port, 24);

    			return SCRIPT_PARSER_OK;
    		}
    		else if(pattern == SCIRPT_PARSER_VALUE_TYPE_STRING)
    		{
    			if(str_size > word_count)
				{
					str_size = word_count;
				}
				memset(gd->script_mod_buf + (sub_key->offset<<2), 0, word_count << 2);
				memcpy(gd->script_mod_buf + (sub_key->offset<<2), str, str_size << 2);

				return SCRIPT_PARSER_OK;
    		}
		}
	}

	return SCRIPT_PARSER_KEY_NOT_FIND;
}

int script_parser_subkey_count(char *main_name)
{
    char   main_bkname[32];
    char   *main_char;
    script_main_key_t  *main_key = NULL;
    int    i;

    if(!gd->script_mod_buf)
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }

    if(main_name == NULL)
    {
        return SCRIPT_PARSER_KEYNAME_NULL;
    }

    main_char = main_name;
    if(_test_str_length(main_name) > 31)
    {
        memset(main_bkname, 0, 32);
        strncpy(main_bkname, main_name, 31);
        main_char = main_bkname;
    }

    for(i=0;i<gd->script_main_key_count;i++)
    {
        main_key = (script_main_key_t *)(gd->script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
        if(strcmp(main_key->main_name, main_char))
        {
            continue;
        }

        return main_key->lenth;
    }

    return -1;
}

int script_parser_mainkey_count(void)
{
    if(!gd->script_mod_buf)
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }

    return  gd->script_main_key_count;
}

int script_parser_mainkey_get_gpio_count(char *main_name)
{
    char   main_bkname[32];
    char   *main_char;
    script_main_key_t  *main_key = NULL;
    script_sub_key_t   *sub_key = NULL;
    int    i, j;
    int    pattern, gpio_count = 0;

    if(!gd->script_mod_buf)
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }

    if(main_name == NULL)
    {
        return SCRIPT_PARSER_KEYNAME_NULL;
    }

    main_char = main_name;
    if(_test_str_length(main_name) > 31)
    {
        memset(main_bkname, 0, 32);
        strncpy(main_bkname, main_name, 31);
        main_char = main_bkname;
    }

    for(i=0;i<gd->script_main_key_count;i++)
    {
        main_key = (script_main_key_t *)(gd->script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
        if(strcmp(main_key->main_name, main_char))
        {
            continue;
        }

        for(j=0;j<main_key->lenth;j++)
        {
            sub_key = (script_sub_key_t *)(gd->script_mod_buf + (main_key->offset<<2) + (j * sizeof(script_sub_key_t)));

            pattern    = (sub_key->pattern>>16) & 0xffff;

            if(SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD == pattern)
            {
                gpio_count ++;
            }
        }
    }

    return gpio_count;
}

int script_parser_mainkey_get_gpio_cfg(char *main_name, void *gpio_cfg, int gpio_count)
{
    char   main_bkname[32];
    char   *main_char;
    script_main_key_t  *main_key = NULL;
    script_sub_key_t   *sub_key = NULL;
    script_gpio_set_t  *user_gpio_cfg = (script_gpio_set_t *)gpio_cfg;
    int    i, j;
    int    pattern, user_index;

    if(!gd->script_mod_buf)
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }

    if(main_name == NULL)
    {
        return SCRIPT_PARSER_KEYNAME_NULL;
    }

    memset(user_gpio_cfg, 0, sizeof(script_gpio_set_t) * gpio_count);

    main_char = main_name;
    if(_test_str_length(main_name) > 31)
    {
        memset(main_bkname, 0, 32);
        strncpy(main_bkname, main_name, 31);
        main_char = main_bkname;
    }

    for(i=0;i<gd->script_main_key_count;i++)
    {
        main_key = (script_main_key_t *)(gd->script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
        if(strcmp(main_key->main_name, main_char))
        {
            continue;
        }

       /* printf("mainkey name = %s\n", main_key->main_name);*/
        user_index = 0;
        for(j=0;j<main_key->lenth;j++)
        {
            sub_key = (script_sub_key_t *)(gd->script_mod_buf + (main_key->offset<<2) + (j * sizeof(script_sub_key_t)));
          /*  printf("subkey name = %s\n", sub_key->sub_name);*/
            pattern    = (sub_key->pattern>>16) & 0xffff;
           /* printf("subkey pattern = %d\n", pattern);*/

            if(SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD == pattern)
            {
                strcpy( user_gpio_cfg[user_index].gpio_name, sub_key->sub_name);
                memcpy(&user_gpio_cfg[user_index].port, gd->script_mod_buf + (sub_key->offset<<2), sizeof(script_gpio_set_t) - 32);
                user_index++;
                if(user_index >= gpio_count)
                {
                    break;
                }
            }
        }
        return SCRIPT_PARSER_OK;
    }

    return SCRIPT_PARSER_KEY_NOT_FIND;
}

__s32  gpio_write_one_pin_value(u32 p_handler, __u32 value_to_gpio, const char *gpio_name)
{
    char               *tmp_buf;                                        //转换成char类型
    __u32               group_count_max;                                //最大GPIO个数
    system_gpio_set_t  *user_gpio_set = NULL, *tmp_sys_gpio_data;
    volatile __u32     *tmp_group_data_addr = NULL;
    __u32               port, port_num, port_num_func, func_val;
    __u32                i, reg_val;
    //检查传进的句柄的有效性
    if(!p_handler)
    {
        return EGPIO_FAIL;
    }
    if(value_to_gpio >= 2)
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    tmp_sys_gpio_data = (system_gpio_set_t *)(tmp_buf + 16);

    if(group_count_max == 0)
    {
        return EGPIO_FAIL;
    }
    else if(group_count_max == 1)
    {
        user_gpio_set = tmp_sys_gpio_data;
    }
    else if(gpio_name)
    {
        for(i=0; i<group_count_max; i++)
        {
            if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name))
            {
                tmp_sys_gpio_data ++;
                continue;
            }
            user_gpio_set = tmp_sys_gpio_data;
            break;
        }
    }
    if(!user_gpio_set)
    {
        return EGPIO_FAIL;
    }

    port     = user_gpio_set->port;
    port_num = user_gpio_set->port_num;
    port_num_func = port_num >> 3;

    reg_val  = PIO_REG_CFG_VALUE(port, port_num_func);
    func_val = (reg_val >> ((port_num - (port_num_func<<3))<<2)) & 0x07;
    if(func_val == 1)
    {
        tmp_group_data_addr = PIO_REG_DATA(port);
        reg_val = GPIO_REG_READ(tmp_group_data_addr);
        reg_val &= ~(1 << port_num);
        reg_val |=  (value_to_gpio << port_num);
        GPIO_REG_WRITE(tmp_group_data_addr, reg_val);

        return EGPIO_SUCCESS;
    }

    return EGPIO_FAIL;
}


static int gpio_request_early(void  *user_gpio_list, __u32 group_count_max, __s32 set_gpio)
{
	user_gpio_set_t    *tmp_user_gpio_data, *gpio_list;
	__u32				first_port;                      //保存真正有效的GPIO的个数
	__u32               tmp_group_func_data;
	__u32               tmp_group_pull_data;
	__u32               tmp_group_dlevel_data;
	__u32               tmp_group_data_data;
	__u32               data_change = 0;
//	__u32			   *tmp_group_port_addr;
	volatile __u32     *tmp_group_func_addr,   *tmp_group_pull_addr;
	volatile __u32     *tmp_group_dlevel_addr, *tmp_group_data_addr;
	__u32  				port, port_num, port_num_func, port_num_pull;
	__u32  				pre_port, pre_port_num_func;
	__u32  				pre_port_num_pull;
	__s32               i, tmp_val;


   	gpio_list = (user_gpio_set_t *)user_gpio_list;

    for(first_port = 0; first_port < group_count_max; first_port++)
    {
        tmp_user_gpio_data = gpio_list + first_port;
        port     = tmp_user_gpio_data->port;                         //读出端口数值
	    port_num = tmp_user_gpio_data->port_num;                     //读出端口中的某一个GPIO
	    if(!port)
	    {
	    	continue;
	    }
	    port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址
        tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);  //更新pull寄存器
        tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull);//更新level寄存器
        tmp_group_data_addr    = PIO_REG_DATA(port);                 //更新data寄存器

        tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
        tmp_group_pull_data    = GPIO_REG_READ(tmp_group_pull_addr);
        tmp_group_dlevel_data  = GPIO_REG_READ(tmp_group_dlevel_addr);
        tmp_group_data_data    = GPIO_REG_READ(tmp_group_data_addr);

        pre_port          = port;
        pre_port_num_func = port_num_func;
        pre_port_num_pull = port_num_pull;
        //更新功能寄存器
        tmp_val = (port_num - (port_num_func << 3)) << 2;
        tmp_group_func_data &= ~(0x07 << tmp_val);
        if(set_gpio)
        {
        	tmp_group_func_data |= (tmp_user_gpio_data->mul_sel & 0x07) << tmp_val;
        }
        //根据pull的值决定是否更新pull寄存器
        tmp_val = (port_num - (port_num_pull << 4)) << 1;
        if(tmp_user_gpio_data->pull >= 0)
        {
        	tmp_group_pull_data &= ~(                           0x03  << tmp_val);
        	tmp_group_pull_data |=  (tmp_user_gpio_data->pull & 0x03) << tmp_val;
        }
        //根据driver level的值决定是否更新driver level寄存器
        if(tmp_user_gpio_data->drv_level >= 0)
        {
        	tmp_group_dlevel_data &= ~(                                0x03  << tmp_val);
        	tmp_group_dlevel_data |=  (tmp_user_gpio_data->drv_level & 0x03) << tmp_val;
        }
        //根据用户输入，以及功能分配决定是否更新data寄存器
        if(tmp_user_gpio_data->mul_sel == 1)
        {
            if(tmp_user_gpio_data->data >= 0)
            {
            	tmp_val = tmp_user_gpio_data->data & 1;
                tmp_group_data_data &= ~(1 << port_num);
                tmp_group_data_data |= tmp_val << port_num;
                data_change = 1;
            }
        }

        break;
	}
	//检查是否有数据存在
	if(first_port >= group_count_max)
	{
	    return -1;
	}
	//保存用户数据
	for(i = first_port + 1; i < group_count_max; i++)
	{
		tmp_user_gpio_data = gpio_list + i;                 //gpio_set依次指向用户的每个GPIO数组成员
	    port     = tmp_user_gpio_data->port;                //读出端口数值
	    port_num = tmp_user_gpio_data->port_num;            //读出端口中的某一个GPIO
	    if(!port)
	    {
	    	break;
	    }
        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        if((port_num_pull != pre_port_num_pull) || (port != pre_port))    //如果发现当前引脚的端口不一致，或者所在的pull寄存器不一致
        {
            GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);     //回写功能寄存器
            GPIO_REG_WRITE(tmp_group_pull_addr, tmp_group_pull_data);     //回写pull寄存器
            GPIO_REG_WRITE(tmp_group_dlevel_addr, tmp_group_dlevel_data); //回写driver level寄存器
            if(data_change)
            {
                data_change = 0;
                GPIO_REG_WRITE(tmp_group_data_addr, tmp_group_data_data); //回写data寄存器
            }

        	tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址
        	tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);  //更新pull寄存器
        	tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull);//更新level寄存器
        	tmp_group_data_addr    = PIO_REG_DATA(port);                 //更新data寄存器

            tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
            tmp_group_pull_data    = GPIO_REG_READ(tmp_group_pull_addr);
            tmp_group_dlevel_data  = GPIO_REG_READ(tmp_group_dlevel_addr);
            tmp_group_data_data    = GPIO_REG_READ(tmp_group_data_addr);
        }
        else if(pre_port_num_func != port_num_func)                       //如果发现当前引脚的功能寄存器不一致
        {
            GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);    //则只回写功能寄存器
            tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址

            tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
        }
		//保存当前硬件寄存器数据
        pre_port_num_pull = port_num_pull;                      //设置当前GPIO成为前一个GPIO
        pre_port_num_func = port_num_func;
        pre_port          = port;

        //更新功能寄存器
        tmp_val = (port_num - (port_num_func << 3)) << 2;
        if(tmp_user_gpio_data->mul_sel >= 0)
        {
        	tmp_group_func_data &= ~(                              0x07  << tmp_val);
        	if(set_gpio)
        	{
        		tmp_group_func_data |=  (tmp_user_gpio_data->mul_sel & 0x07) << tmp_val;
        	}
        }
        //根据pull的值决定是否更新pull寄存器
        tmp_val = (port_num - (port_num_pull << 4)) << 1;
        if(tmp_user_gpio_data->pull >= 0)
        {
        	tmp_group_pull_data &= ~(                           0x03  << tmp_val);
        	tmp_group_pull_data |=  (tmp_user_gpio_data->pull & 0x03) << tmp_val;
        }
        //根据driver level的值决定是否更新driver level寄存器
        if(tmp_user_gpio_data->drv_level >= 0)
        {
        	tmp_group_dlevel_data &= ~(                                0x03  << tmp_val);
        	tmp_group_dlevel_data |=  (tmp_user_gpio_data->drv_level & 0x03) << tmp_val;
        }
        //根据用户输入，以及功能分配决定是否更新data寄存器
        if(tmp_user_gpio_data->mul_sel == 1)
        {
            if(tmp_user_gpio_data->data >= 0)
            {
            	tmp_val = tmp_user_gpio_data->data & 1;
                tmp_group_data_data &= ~(1 << port_num);
                tmp_group_data_data |= tmp_val << port_num;
                data_change = 1;
            }
        }
    }
    //for循环结束，如果存在还没有回写的寄存器，这里写回到硬件当中
    if(tmp_group_func_addr)                         //只要更新过寄存器地址，就可以对硬件赋值
    {                                               //那么把所有的值全部回写到硬件寄存器
        GPIO_REG_WRITE(tmp_group_func_addr,   tmp_group_func_data);   //回写功能寄存器
        GPIO_REG_WRITE(tmp_group_pull_addr,   tmp_group_pull_data);   //回写pull寄存器
        GPIO_REG_WRITE(tmp_group_dlevel_addr, tmp_group_dlevel_data); //回写driver level寄存器
        if(data_change)
        {
            GPIO_REG_WRITE(tmp_group_data_addr, tmp_group_data_data); //回写data寄存器
        }
    }

    return 0;
}


u32 gpio_request(user_gpio_set_t *gpio_list, __u32 group_count_max)
{
    char               *user_gpio_buf;                                        //按照char类型申请
    system_gpio_set_t  *user_gpio_set, *tmp_sys_gpio_data;                      //user_gpio_set将是申请内存的句柄
    user_gpio_set_t  *tmp_user_gpio_data;
    __u32                real_gpio_count = 0, first_port;                      //保存真正有效的GPIO的个数
    __u32               tmp_group_func_data = 0;
    __u32               tmp_group_pull_data = 0;
    __u32               tmp_group_dlevel_data = 0;
    __u32               tmp_group_data_data = 0;
    __u32               func_change = 0, pull_change = 0;
    __u32               dlevel_change = 0, data_change = 0;
    volatile __u32  *tmp_group_func_addr = NULL, *tmp_group_pull_addr = NULL;
    volatile __u32  *tmp_group_dlevel_addr = NULL, *tmp_group_data_addr = NULL;
    __u32  port, port_num, port_num_func, port_num_pull;
    __u32  pre_port = 0x7fffffff, pre_port_num_func = 0x7fffffff;
    __u32  pre_port_num_pull = 0x7fffffff;
    __s32  i, tmp_val;
    if((!gpio_list) || (!group_count_max))
    {
        return (u32)0;
    }
    for(i = 0; i < group_count_max; i++)
    {
        tmp_user_gpio_data = gpio_list + i;                 //gpio_set依次指向每个GPIO数组成员
        if(!tmp_user_gpio_data->port)
        {
            continue;
        }
        real_gpio_count ++;
    }

    //SYSCONFIG_DEBUG("to malloc space for pin \n");
    user_gpio_buf = (char *)malloc(16 + sizeof(system_gpio_set_t) * real_gpio_count);   //申请内存，多申请16个字节，用于存放GPIO个数等信息
    if(!user_gpio_buf)
    {
        return (u32)0;
    }
    memset(user_gpio_buf, 0, 16 + sizeof(system_gpio_set_t) * real_gpio_count);         //首先全部清零
    *(int *)user_gpio_buf = real_gpio_count;                                           //保存有效的GPIO个数
    user_gpio_set = (system_gpio_set_t *)(user_gpio_buf + 16);                         //指向第一个结构体
    //准备第一个GPIO数据
    for(first_port = 0; first_port < group_count_max; first_port++)
    {
        tmp_user_gpio_data = gpio_list + first_port;
        port     = tmp_user_gpio_data->port;                         //读出端口数值
        port_num = tmp_user_gpio_data->port_num;                     //读出端口中的某一个GPIO
        if(!port)
        {
            continue;
        }
        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址
        tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);  //更新pull寄存器
        tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull);//更新level寄存器
        tmp_group_data_addr    = PIO_REG_DATA(port);                 //更新data寄存器

        tmp_group_func_data    = *tmp_group_func_addr;
        tmp_group_pull_data    = *tmp_group_pull_addr;
        tmp_group_dlevel_data  = *tmp_group_dlevel_addr;
        tmp_group_data_data    = *tmp_group_data_addr;
        break;
    }
    if(first_port >= group_count_max)
    {
        return 0;
    }
    //保存用户数据
    for(i = first_port; i < group_count_max; i++)
    {
        tmp_sys_gpio_data  = user_gpio_set + i;             //tmp_sys_gpio_data指向申请的GPIO空间
        tmp_user_gpio_data = gpio_list + i;                 //gpio_set依次指向用户的每个GPIO数组成员
        port     = tmp_user_gpio_data->port;                //读出端口数值
        port_num = tmp_user_gpio_data->port_num;            //读出端口中的某一个GPIO
        if(!port)
        {
            continue;
        }
        //开始保存用户数据
        strcpy(tmp_sys_gpio_data->gpio_name, tmp_user_gpio_data->gpio_name);
        tmp_sys_gpio_data->port                       = port;
        tmp_sys_gpio_data->port_num                   = port_num;
        tmp_sys_gpio_data->user_gpio_status.mul_sel   = tmp_user_gpio_data->mul_sel;
        tmp_sys_gpio_data->user_gpio_status.pull      = tmp_user_gpio_data->pull;
        tmp_sys_gpio_data->user_gpio_status.drv_level = tmp_user_gpio_data->drv_level;
        tmp_sys_gpio_data->user_gpio_status.data      = tmp_user_gpio_data->data;

        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        if((port_num_pull != pre_port_num_pull) || (port != pre_port))    //如果发现当前引脚的端口不一致，或者所在的pull寄存器不一致
        {
            if(func_change)
            {
                *tmp_group_func_addr   = tmp_group_func_data;    //回写功能寄存器
                func_change = 0;
            }
            if(pull_change)
            {
                pull_change = 0;
                *tmp_group_pull_addr   = tmp_group_pull_data;    //回写pull寄存器
            }
            if(dlevel_change)
            {
                dlevel_change = 0;
                *tmp_group_dlevel_addr = tmp_group_dlevel_data;  //回写driver level寄存器
            }
            if(data_change)
            {
                data_change = 0;
                *tmp_group_data_addr   = tmp_group_data_data;    //回写
            }

            tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址
            tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);   //更新pull寄存器
            tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull); //更新level寄存器
            tmp_group_data_addr    = PIO_REG_DATA(port);                  //更新data寄存器

			tmp_group_func_data    = *tmp_group_func_addr;
            tmp_group_pull_data    = *tmp_group_pull_addr;
            tmp_group_dlevel_data  = *tmp_group_dlevel_addr;
            tmp_group_data_data    = *tmp_group_data_addr;

        }
        else if(pre_port_num_func != port_num_func)                       //如果发现当前引脚的功能寄存器不一致
        {
            *tmp_group_func_addr   = tmp_group_func_data;    //则只回写功能寄存器

           tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址

            tmp_group_func_data    = *tmp_group_func_addr;
        }
        //保存当前硬件寄存器数据
        pre_port_num_pull = port_num_pull;                      //设置当前GPIO成为前一个GPIO
        pre_port_num_func = port_num_func;
        pre_port          = port;

        //更新功能寄存器
        if(tmp_user_gpio_data->mul_sel >= 0)
        {
            tmp_val = (port_num - (port_num_func<<3)) << 2;
            tmp_sys_gpio_data->hardware_gpio_status.mul_sel = (tmp_group_func_data >> tmp_val) & 0x07;
            tmp_group_func_data &= ~(                              0x07  << tmp_val);
            tmp_group_func_data |=  (tmp_user_gpio_data->mul_sel & 0x07) << tmp_val;
            func_change = 1;
        }
        //根据pull的值决定是否更新pull寄存器

        tmp_val = (port_num - (port_num_pull<<4)) << 1;

        if(tmp_user_gpio_data->pull >= 0)
        {
            tmp_sys_gpio_data->hardware_gpio_status.pull = (tmp_group_pull_data >> tmp_val) & 0x03;
            if(tmp_user_gpio_data->pull >= 0)
            {
                tmp_group_pull_data &= ~(                           0x03  << tmp_val);
                tmp_group_pull_data |=  (tmp_user_gpio_data->pull & 0x03) << tmp_val;
                pull_change = 1;
            }
        }
        //根据driver level的值决定是否更新driver level寄存器
        if(tmp_user_gpio_data->drv_level >= 0)
        {
            tmp_sys_gpio_data->hardware_gpio_status.drv_level = (tmp_group_dlevel_data >> tmp_val) & 0x03;
            if(tmp_user_gpio_data->drv_level >= 0)
            {
                tmp_group_dlevel_data &= ~(                                0x03  << tmp_val);
                tmp_group_dlevel_data |=  (tmp_user_gpio_data->drv_level & 0x03) << tmp_val;
                dlevel_change = 1;
            }
        }
        //根据用户输入，以及功能分配决定是否更新data寄存器
        if(tmp_user_gpio_data->mul_sel == 1)
        {
            if(tmp_user_gpio_data->data >= 0)
            {
                tmp_val = tmp_user_gpio_data->data;
                tmp_val &= 1;
                tmp_group_data_data &= ~(1 << port_num);
                tmp_group_data_data |= tmp_val << port_num;
                data_change = 1;
            }
        }
    }

    //for循环结束，如果存在还没有回写的寄存器，这里写回到硬件当中
    if(tmp_group_func_addr)                         //只要更新过寄存器地址，就可以对硬件赋值
    {                                               //那么把所有的值全部回写到硬件寄存器
        *tmp_group_func_addr   = tmp_group_func_data;       //回写功能寄存器
        if(pull_change)
        {
            *tmp_group_pull_addr   = tmp_group_pull_data;    //回写pull寄存器
        }
        if(dlevel_change)
        {
            *tmp_group_dlevel_addr = tmp_group_dlevel_data;  //回写driver level寄存器
        }
        if(data_change)
        {
            *tmp_group_data_addr   = tmp_group_data_data;    //回写data寄存器
        }
    }
    return (u32)user_gpio_buf;
}
//#endif
/*
************************************************************************************************************
*
*                                             gpio_request_ex
*
*    函数名称：
*
*    参数说明: main_name   传进的主键名称，匹配模块(驱动名称)
*
*               sub_name    传进的子键名称，如果是空，表示全部，否则寻找到匹配的单独GPIO
*
*    返回值  ：0 :    err
*              other: success
*
*    说明    ：暂时没有做冲突检查
*
*
************************************************************************************************************
*/
u32 gpio_request_ex(char *main_name, const char *sub_name)  //设备申请GPIO函数扩展接口
{
    user_gpio_set_t    *gpio_list=NULL;
    user_gpio_set_t     one_gpio;
       __u32               gpio_handle;
    __s32               gpio_count;

    if(!sub_name){
            gpio_count = script_parser_mainkey_get_gpio_count(main_name);
            if(gpio_count <= 0)
            {
                /*printf("err: gpio count < =0 ,gpio_count is: %d \n", gpio_count);*/
                return 0;
            }
            gpio_list = (user_gpio_set_t *)malloc(sizeof(system_gpio_set_t) * gpio_count); //申请一片临时内存，用于保存用户数据
            if(!gpio_list){
         /*   printf("malloc gpio_list error \n");*/
                return 0;
            }
        if(!script_parser_mainkey_get_gpio_cfg(main_name,gpio_list,gpio_count)){
            gpio_handle = gpio_request(gpio_list, gpio_count);
            free(gpio_list);

        }else{
            return 0;
        }
        }else{
            if(script_parser_fetch((char *)main_name, (char *)sub_name, (int *)&one_gpio, (sizeof(user_gpio_set_t) >> 2)) < 0){
           /* printf("script parser fetch err. \n");*/
            return 0;
            }

            gpio_handle = gpio_request(&one_gpio, 1);
        }

        return gpio_handle;

}
/*
************************************************************************************************************
*
*                                             gpio_request_simple
*
*    函数名称：
*
*    参数说明: main_name   传进的主键名称，匹配模块(驱动名称)
*
*               sub_name    传进的子键名称，如果是空，表示全部，否则寻找到匹配的单独GPIO
*
*    返回值  ：0 :    err
*              other: success
*
*    说明    ：暂时没有做冲突检查
*
*
************************************************************************************************************
*/
int gpio_request_simple(char *main_name, const char *sub_name)  //设备申请GPIO函数扩展接口
{
    user_gpio_set_t     gpio_list[16];
    __s32               gpio_count;
    int ret = -1;

    if(!sub_name)
    {
        gpio_count = script_parser_mainkey_get_gpio_count(main_name);
        if(gpio_count <= 0)
        {
            printf("err: gpio count < =0 ,gpio_count is: %d \n", gpio_count);
            return -1;
        }
        memset(gpio_list, 0, 16 * sizeof(user_gpio_set_t));
        if(!script_parser_mainkey_get_gpio_cfg(main_name, gpio_list, gpio_count))
        {
            ret = gpio_request_early(gpio_list, gpio_count, 1);
        }
    }
    else
    {
        if(script_parser_fetch((char *)main_name, (char *)sub_name, (int *)gpio_list, (sizeof(user_gpio_set_t) >> 2)) < 0)
        {
       		printf("script parser fetch err. \n");
        	return 0;
        }

        ret = gpio_request_early(gpio_list, 1, 1);
    }

    return ret;
}


__s32 gpio_release(u32 p_handler, __s32 if_release_to_default_status)
{
    char               *tmp_buf;                                        //转换成char类型
    __u32               group_count_max, first_port;                    //最大GPIO个数
    system_gpio_set_t  *user_gpio_set, *tmp_sys_gpio_data;
    __u32               tmp_group_func_data = 0;
    __u32               tmp_group_pull_data = 0;
    __u32               tmp_group_dlevel_data = 0;
    volatile __u32     *tmp_group_func_addr = NULL,   *tmp_group_pull_addr = NULL;
    volatile __u32     *tmp_group_dlevel_addr = NULL;
    __u32               port, port_num, port_num_pull, port_num_func;
    __u32               pre_port = 0x7fffffff, pre_port_num_func = 0x7fffffff, pre_port_num_pull = 0x7fffffff;
    __u32               i, tmp_val;
    //检查传进的句柄的有效性
    if(!p_handler)
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    if(!group_count_max)
    {
        return EGPIO_FAIL;
    }
    if(if_release_to_default_status == 2)
    {
        //SYSCONFIG_DEBUG("gpio module :  release p_handler = %x\n",p_handler);
        free((char *)p_handler);

        return EGPIO_SUCCESS;
    }
    user_gpio_set = (system_gpio_set_t *)(tmp_buf + 16);
    //读取用户数据
    for(first_port = 0; first_port < group_count_max; first_port++)
    {
        tmp_sys_gpio_data  = user_gpio_set + first_port;
        port     = tmp_sys_gpio_data->port;                 //读出端口数值
        port_num = tmp_sys_gpio_data->port_num;             //读出端口中的某一个GPIO
        if(!port)
        {
            continue;
        }
        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址
        tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);  //更新pull寄存器
        tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull);//更新level寄存器

        tmp_group_func_data    = *tmp_group_func_addr;
        tmp_group_pull_data    = *tmp_group_pull_addr;
        tmp_group_dlevel_data  = *tmp_group_dlevel_addr;
        break;
    }
    if(first_port >= group_count_max)
    {
        return 0;
    }
    for(i = first_port; i < group_count_max; i++)
    {
        tmp_sys_gpio_data  = user_gpio_set + i;             //tmp_sys_gpio_data指向申请的GPIO空间
        port     = tmp_sys_gpio_data->port;                 //读出端口数值
        port_num = tmp_sys_gpio_data->port_num;             //读出端口中的某一个GPIO

        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        if((port_num_pull != pre_port_num_pull) || (port != pre_port))    //如果发现当前引脚的端口不一致，或者所在的pull寄存器不一致
        {
            *tmp_group_func_addr   = tmp_group_func_data;    //回写功能寄存器
            *tmp_group_pull_addr   = tmp_group_pull_data;    //回写pull寄存器
            *tmp_group_dlevel_addr = tmp_group_dlevel_data;  //回写driver level寄存器

            tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址
            tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);   //更新pull寄存器
            tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull); //更新level寄存器

            tmp_group_func_data    = *tmp_group_func_addr;
            tmp_group_pull_data    = *tmp_group_pull_addr;
            tmp_group_dlevel_data  = *tmp_group_dlevel_addr;
        }
        else if(pre_port_num_func != port_num_func)                       //如果发现当前引脚的功能寄存器不一致
        {
            *tmp_group_func_addr   = tmp_group_func_data;                 //则只回写功能寄存器
            tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址
            tmp_group_func_data    = *tmp_group_func_addr;
        }

        pre_port_num_pull = port_num_pull;
        pre_port_num_func = port_num_func;
        pre_port          = port;
        //更新功能寄存器
        tmp_group_func_data &= ~(0x07 << ((port_num - (port_num_func<<3)) << 2));
        //更新pull状态寄存器
        tmp_val              =  (port_num - (port_num_pull<<4)) << 1;
        tmp_group_pull_data &= ~(0x03  << tmp_val);
        tmp_group_pull_data |= (tmp_sys_gpio_data->hardware_gpio_status.pull & 0x03) << tmp_val;
        //更新driver状态寄存器
        tmp_val              =  (port_num - (port_num_pull<<4)) << 1;
        tmp_group_dlevel_data &= ~(0x03  << tmp_val);
        tmp_group_dlevel_data |= (tmp_sys_gpio_data->hardware_gpio_status.drv_level & 0x03) << tmp_val;
    }
    if(tmp_group_func_addr)                              //只要更新过寄存器地址，就可以对硬件赋值
    {                                                    //那么把所有的值全部回写到硬件寄存器
        *tmp_group_func_addr   = tmp_group_func_data;    //回写功能寄存器
    }
    if(tmp_group_pull_addr)
    {
        *tmp_group_pull_addr   = tmp_group_pull_data;
    }
    if(tmp_group_dlevel_addr)
    {
        *tmp_group_dlevel_addr = tmp_group_dlevel_data;
    }

    free((char *)p_handler);

    return EGPIO_SUCCESS;
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
void upper(char *str)
{
	int i=0;
	char c;

	do
	{
		c=str[i];
		if(c=='\0')
		{
			return;
		}
		if((c>='a') && (c<='z'))
		{
			str[i]-=('a'-'A');
		}
		i++;
	}
	while(1);
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
void lower(char *str)
{
	int i=0;
	char c;

	do
	{
		c=str[i];
		if(c=='\0')
		{
			return;
		}
		if((c>='A') && (c<='Z'))
		{
			str[i]+=('a'-'A');
		}
		i++;
	}
	while(1);
}

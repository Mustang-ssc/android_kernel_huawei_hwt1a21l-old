/* < DTS2014042402686 sunlibin 20140424 begin */
/*Add for huawei TP*/
/*
 * Copyright (c) 2014 Huawei Device Company
 *
 * This file provide common requeirment for different touch IC.
 * 
 * 2014-01-04:Add "tp_get_touch_screen_obj" by sunlibin
 *
 */
#include <linux/hw_tp_common.h>
#include <linux/module.h>
#include <linux/synaptics_dsx_i2c.h>
/* < DTS2014052402264 shenjinming 20140526 begin */
/* delete a line */
/* DTS2014052402264 shenjinming 20140526 end > */
/* < DTS2015012000564 zhangyonggang 20150120 begin */
static int g_tp_type = UNKNOW_PRODUCT_MODULE;
/* DTS2015012000564 zhangyonggang 20150120 end > */
static struct kobject *touch_screen_kobject_ts = NULL;
static struct kobject *touch_glove_func_ts = NULL;
/* BEGIN PN: DTS2014101601174,Added by yuanbo, 2014/10/16*/
/* < DTS2014082603541 yanghaizhou 20140826 begin */
static unsigned char  is_tp_driver_running_flag = 0;
/* DTS2014082603541 yanghaizhou 20140826 end > */
/* END   PN: DTS2014101601174,Added by yuanbo, 2014/10/16*/
/* <DTS2014090905785 wwx203500 20141009 begin */
struct kobject *virtual_key_kobject_ts = NULL;
/* DTS2014090905785 wwx203500 20141009 end> */
/* BEGIN PN: DTS2014101601174,Added by yuanbo, 2014/10/16*/
/* < DTS2014082603541 yanghaizhou 20140826 begin */
unsigned char already_has_tp_driver_running(void)
{
    return is_tp_driver_running_flag;
}

void set_tp_driver_running(void)
{
    is_tp_driver_running_flag = 1;
}
/* DTS2014082603541 yanghaizhou 20140826 end > */
/* END   PN: DTS2014101601174,Added by yuanbo, 2014/10/16*/
struct kobject* tp_get_touch_screen_obj(void)
{
	if( NULL == touch_screen_kobject_ts )
	{
		touch_screen_kobject_ts = kobject_create_and_add("touch_screen", NULL);
		if (!touch_screen_kobject_ts)
		{
			tp_log_err("%s: create touch_screen kobjetct error!\n", __func__);
			return NULL;
		}
		else
		{
			tp_log_debug("%s: create sys/touch_screen successful!\n", __func__);
		}
	}
	else
	{
		tp_log_debug("%s: sys/touch_screen already exist!\n", __func__);
	}

	return touch_screen_kobject_ts;
}

/* <DTS2014090905785 wwx203500 20141009 begin */
struct kobject* tp_get_virtual_key_obj(char *name)
{
	if( NULL == virtual_key_kobject_ts )
	{
		virtual_key_kobject_ts = kobject_create_and_add(name, NULL);
		if (!virtual_key_kobject_ts)
		{
			tp_log_err("%s: create virtual_key kobjetct error!\n", __func__);
			return NULL;
		}
		else
		{
			tp_log_debug("%s: create virtual_key successful!\n", __func__);
		}
	}
	else
	{
		tp_log_debug("%s: virtual_key already exist!\n", __func__);
	}

	return virtual_key_kobject_ts;
}
/* DTS2014090905785 wwx203500 20141009 end> */

struct kobject* tp_get_glove_func_obj(void)
{
	struct kobject *properties_kobj;
	
	properties_kobj = tp_get_touch_screen_obj();
	if( NULL == properties_kobj )
	{
		tp_log_err("%s: Error, get kobj failed!\n", __func__);
		return NULL;
	}
	
	if( NULL == touch_glove_func_ts )
	{
		touch_glove_func_ts = kobject_create_and_add("glove_func", properties_kobj);
		if (!touch_glove_func_ts)
		{
			tp_log_err("%s: create glove_func kobjetct error!\n", __func__);
			return NULL;
		}
		else
		{
			tp_log_debug("%s: create sys/touch_screen/glove_func successful!\n", __func__);
		}
	}
	else
	{
		tp_log_debug("%s: sys/touch_screen/glove_func already exist!\n", __func__);
	}

	return touch_glove_func_ts;
}
/* < DTS2015012000564 zhangyonggang 20150120 begin */
/*add api func for sensor to get TP module*/
/*tp type define in enum f54_product_module_name*/
/*tp type store in global variable g_tp_type */
/*get_tp_type:sensor use to get tp type*/
int get_tp_type(void)
{
	return g_tp_type;
}
/*set_tp_type:drivers use to set tp type*/
void set_tp_type(int type)
{
	g_tp_type = type;
	tp_log_err("%s:tp_type=%d\n",__func__,type);
}
/* DTS2015012000564 zhangyonggang 20150120 end > */
/* < DTS2014052402264 shenjinming 20140526 begin */
/* remove function to synaptics_dsx_i2c.c file */
/* DTS2014052402264 shenjinming 20140526 end > */

/* DTS2014042402686 sunlibin 20140424 end> */

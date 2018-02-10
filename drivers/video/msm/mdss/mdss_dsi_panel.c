/* Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/qpnp/pin.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/leds.h>
#include <linux/qpnp/pwm.h>
#include <linux/err.h>

#include "mdss_dsi.h"
/*<DTS2014111701156 l00101002 20140924 begin*/
#include <linux/log_jank.h>
/*DTS2014111701156 l00101002 20140924 end>*/
/* <DTS2014042405897 d00238048 20140425 begin */
#include <misc/app_info.h>
/* DTS2014042405897 d00238048 20140425 end> */
/* < DTS2014051503541 daiyuhong 20140515 begin */
/*< DTS2014042905347 zhaoyuxia 20140429 begin */
#include <linux/hw_lcd_common.h>
#include <hw_lcd_debug.h>
/*DTS2014101301850 zhoujian 20141013 begin */
extern int get_offline_cpu(void);
extern unsigned int cpufreq_get(unsigned int cpu);
/*DTS2014101301850 zhoujian 20141013 end */
#ifdef CONFIG_HUAWEI_LCD
int lcd_debug_mask = LCD_INFO;
#define INVERSION_OFF 0
#define INVERSION_ON 1
module_param_named(lcd_debug_mask, lcd_debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);
static bool enable_initcode_debugging = FALSE;
module_param_named(enable_initcode_debugging, enable_initcode_debugging, bool, S_IRUGO | S_IWUSR);
#endif
/* DTS2014042905347 zhaoyuxia 20140429 end >*/
/* DTS2014051503541 daiyuhong 20140515 end > */
#define DT_CMD_HDR 6

/* NT35596 panel specific status variables */
#define NT35596_BUF_3_STATUS 0x02
#define NT35596_BUF_4_STATUS 0x40
#define NT35596_BUF_5_STATUS 0x80
#define NT35596_MAX_ERR_CNT 2

DEFINE_LED_TRIGGER(bl_led_trigger);

void mdss_dsi_panel_pwm_cfg(struct mdss_dsi_ctrl_pdata *ctrl)
{
	ctrl->pwm_bl = pwm_request(ctrl->pwm_lpg_chan, "lcd-bklt");
	if (ctrl->pwm_bl == NULL || IS_ERR(ctrl->pwm_bl)) {
		pr_err("%s: Error: lpg_chan=%d pwm request failed",
				__func__, ctrl->pwm_lpg_chan);
	}
}

static void mdss_dsi_panel_bklt_pwm(struct mdss_dsi_ctrl_pdata *ctrl, int level)
{
	int ret;
	u32 duty;

	if (ctrl->pwm_bl == NULL) {
		pr_err("%s: no PWM\n", __func__);
		return;
	}

	if (level == 0) {
		if (ctrl->pwm_enabled)
			pwm_disable(ctrl->pwm_bl);
		ctrl->pwm_enabled = 0;
		return;
	}

	duty = level * ctrl->pwm_period;
	duty /= ctrl->bklt_max;

	pr_debug("%s: bklt_ctrl=%d pwm_period=%d pwm_gpio=%d pwm_lpg_chan=%d\n",
			__func__, ctrl->bklt_ctrl, ctrl->pwm_period,
				ctrl->pwm_pmic_gpio, ctrl->pwm_lpg_chan);

	pr_debug("%s: ndx=%d level=%d duty=%d\n", __func__,
					ctrl->ndx, level, duty);

	if (ctrl->pwm_enabled) {
		pwm_disable(ctrl->pwm_bl);
		ctrl->pwm_enabled = 0;
	}

	ret = pwm_config_us(ctrl->pwm_bl, duty, ctrl->pwm_period);
	if (ret) {
		pr_err("%s: pwm_config_us() failed err=%d.\n", __func__, ret);
		return;
	}

	ret = pwm_enable(ctrl->pwm_bl);
	if (ret)
		pr_err("%s: pwm_enable() failed err=%d\n", __func__, ret);
	ctrl->pwm_enabled = 1;
}

/* < DTS2014071607596 renxigang 20140717 begin */
/*set the variable from globle to local avoid Competition when esd and running test read at the same time*/
#ifndef CONFIG_HUAWEI_LCD
static char dcs_cmd[2] = {0x54, 0x00}; /* DTYPE_DCS_READ */
static struct dsi_cmd_desc dcs_read_cmd = {
	{DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(dcs_cmd)},
	dcs_cmd
};
#endif
/* DTS2014071607596 renxigang 20140717 end >*/
u32 mdss_dsi_panel_cmd_read(struct mdss_dsi_ctrl_pdata *ctrl, char cmd0,
		char cmd1, void (*fxn)(int), char *rbuf, int len)
{
	struct dcs_cmd_req cmdreq;
/* < DTS2014071607596 renxigang 20140717 begin */
	/*delete DTS2014070102909*/
#ifdef CONFIG_HUAWEI_LCD
	char dcs_cmd[2] = {0x00, 0x00}; /* DTYPE_DCS_READ */
	struct dsi_cmd_desc dcs_read_cmd = {
	{	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(dcs_cmd)},
		dcs_cmd
	};
#endif
	dcs_cmd[0] = cmd0;
	dcs_cmd[1] = cmd1;
	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = &dcs_read_cmd;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_RX | CMD_REQ_COMMIT;
	cmdreq.rlen = len;
	cmdreq.rbuf = rbuf;
	cmdreq.cb = fxn; /* call back */
	mdss_dsi_cmdlist_put(ctrl, &cmdreq);
	/*
	 * blocked here, until call back called
	 */

	return 0;
}
/* DTS2014071607596 renxigang 20140717 end >*/
static void mdss_dsi_panel_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl,
			struct dsi_panel_cmds *pcmds)
{
	struct dcs_cmd_req cmdreq;

	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = pcmds->cmds;
	cmdreq.cmds_cnt = pcmds->cmd_cnt;
	cmdreq.flags = CMD_REQ_COMMIT;

	/*Panel ON/Off commands should be sent in DSI Low Power Mode*/
	if (pcmds->link_state == DSI_LP_MODE)
		cmdreq.flags  |= CMD_REQ_LP_MODE;

	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mdss_dsi_cmdlist_put(ctrl, &cmdreq);
}

static char led_pwm1[2] = {0x51, 0x0};	/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc backlight_cmd = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(led_pwm1)},
	led_pwm1
};

static void mdss_dsi_panel_bklt_dcs(struct mdss_dsi_ctrl_pdata *ctrl, int level)
{
	struct dcs_cmd_req cmdreq;

	pr_debug("%s: level=%d\n", __func__, level);

	led_pwm1[1] = (unsigned char)level;

	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = &backlight_cmd;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mdss_dsi_cmdlist_put(ctrl, &cmdreq);
/*< DTS2014042905347 zhaoyuxia 20140429 begin */
#ifdef CONFIG_HUAWEI_LCD
	LCD_LOG_DBG("%s: level=%d\n", __func__, level);
#endif
/* DTS2014042905347 zhaoyuxia 20140429 end >*/
}

/* < DTS2014053009543 zhaoyuxia 20140604 begin */
/* revert huawei modify */
static int mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;

	if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		rc = gpio_request(ctrl_pdata->disp_en_gpio,
						"disp_enable");
		if (rc) {
			pr_err("request disp_en gpio failed, rc=%d\n",
				       rc);
			goto disp_en_gpio_err;
		}
	}
	rc = gpio_request(ctrl_pdata->rst_gpio, "disp_rst_n");
	if (rc) {
		pr_err("request reset gpio failed, rc=%d\n",
			rc);
		goto rst_gpio_err;
	}
	if (gpio_is_valid(ctrl_pdata->bklt_en_gpio)) {
		rc = gpio_request(ctrl_pdata->bklt_en_gpio,
						"bklt_enable");
		if (rc) {
			pr_err("request bklt gpio failed, rc=%d\n",
				       rc);
			goto bklt_en_gpio_err;
		}
	}
	if (gpio_is_valid(ctrl_pdata->mode_gpio)) {
		rc = gpio_request(ctrl_pdata->mode_gpio, "panel_mode");
		if (rc) {
			pr_err("request panel mode gpio failed,rc=%d\n",
								rc);
			goto mode_gpio_err;
		}
	}
	return rc;

mode_gpio_err:
	if (gpio_is_valid(ctrl_pdata->bklt_en_gpio))
		gpio_free(ctrl_pdata->bklt_en_gpio);
bklt_en_gpio_err:
	gpio_free(ctrl_pdata->rst_gpio);
rst_gpio_err:
	if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
		gpio_free(ctrl_pdata->disp_en_gpio);
disp_en_gpio_err:
	return rc;
}

/* < DTS2014052100201 zhaoyuxia 20140526 begin */
/* optimize code */

int mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = NULL;
	int i, rc = 0;
	/*DTS2014101301850 zhoujian 20141013 begin */
	unsigned long timeout = jiffies;
	/*DTS2014101301850 zhoujian 20141013 end */
	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
			   __func__, __LINE__);
	}

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
			   __func__, __LINE__);
		return rc;
	}

	pr_debug("%s: enable = %d\n", __func__, enable);
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (enable) {
		rc = mdss_dsi_request_gpios(ctrl_pdata);
		if (rc) {
			pr_err("gpio request failed\n");
			return rc;
		}
	#ifdef CONFIG_HUAWEI_LCD
		LCD_MDELAY(20);

		for (i = 0; i < pdata->panel_info.rst_seq_len; ++i) {
			gpio_set_value((ctrl_pdata->rst_gpio),
				pdata->panel_info.rst_seq[i]);
			if (pdata->panel_info.rst_seq[++i])
				usleep(pinfo->rst_seq[i] * 1000);
		}
	#else
		if (!pinfo->panel_power_on) {
			if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
				gpio_set_value((ctrl_pdata->disp_en_gpio), 1);

			for (i = 0; i < pdata->panel_info.rst_seq_len; ++i) {
				gpio_set_value((ctrl_pdata->rst_gpio),
					pdata->panel_info.rst_seq[i]);
				if (pdata->panel_info.rst_seq[++i])
					usleep(pinfo->rst_seq[i] * 1000);
			}

			if (gpio_is_valid(ctrl_pdata->bklt_en_gpio))
				gpio_set_value((ctrl_pdata->bklt_en_gpio), 1);
		}
	#endif
		if (gpio_is_valid(ctrl_pdata->mode_gpio)) {
			if (pinfo->mode_gpio_state == MODE_GPIO_HIGH)
				gpio_set_value((ctrl_pdata->mode_gpio), 1);
			else if (pinfo->mode_gpio_state == MODE_GPIO_LOW)
				gpio_set_value((ctrl_pdata->mode_gpio), 0);
		}
		if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
			pr_debug("%s: Panel Not properly turned OFF\n",
						__func__);
			ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
			pr_debug("%s: Reset panel done\n", __func__);
		}
	} else {
/*< DTS2014103106441 huangli/hwx207935 20141031 begin*/
	#ifdef CONFIG_HUAWEI_LCD
		if(pinfo->mipi_rest_delay){
			LCD_LOG_INFO("%s:delay is %d\n",__func__,pinfo->mipi_rest_delay);
			LCD_MDELAY(pinfo->mipi_rest_delay);

			}
	#endif
/*DTS2014103106441 huangli/hwx207935 20141031 end >*/
		if (gpio_is_valid(ctrl_pdata->bklt_en_gpio)) {
			gpio_set_value((ctrl_pdata->bklt_en_gpio), 0);
			gpio_free(ctrl_pdata->bklt_en_gpio);
		}
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
			gpio_free(ctrl_pdata->disp_en_gpio);
		}
		gpio_set_value((ctrl_pdata->rst_gpio), 0);
		gpio_free(ctrl_pdata->rst_gpio);
		if (gpio_is_valid(ctrl_pdata->mode_gpio))
			gpio_free(ctrl_pdata->mode_gpio);
	#ifdef CONFIG_HUAWEI_LCD
		LCD_MDELAY(10);
	#endif

	}
	/* DTS2014101301850 zhoujian 20141013 begin > */
	/* add for timeout print log */
	LCD_LOG_INFO("%s: panel reset time = %u,offlinecpu = %d,curfreq = %d\n",
		__func__,jiffies_to_msecs(jiffies-timeout),get_offline_cpu(),cpufreq_get(0));
	/* DTS2014101301850 zhoujian 20141013 end > */
	return rc;
}
/* DTS2014053009543 zhaoyuxia 20140604 end > */
/* DTS2014042500432 zhaoyuxia 20140225 end > */
/* <DTS2014042409252 d00238048 20140505 begin */
#ifdef CONFIG_FB_AUTO_CABC
static int mdss_dsi_panel_cabc_ctrl(struct mdss_panel_data *pdata,struct msmfb_cabc_config cabc_cfg)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	if (pdata == NULL) {
		LCD_LOG_ERR("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
/*< DTS2014121807007 tianye/293347 20141218 begin*/
/* Avoid occur with setting backlight at sametime */
	msleep(30);
/*DTS2014121807007 tianye/293347 20141218 end >*/
	switch(cabc_cfg.mode)
	{
		case CABC_MODE_UI:
			if (ctrl_pdata->dsi_panel_cabc_ui_cmds.cmd_cnt)
				mdss_dsi_panel_cmds_send(ctrl_pdata, &ctrl_pdata->dsi_panel_cabc_ui_cmds);
			break;
		case CABC_MODE_MOVING:
		case CABC_MODE_STILL:
			if (ctrl_pdata->dsi_panel_cabc_video_cmds.cmd_cnt)
				mdss_dsi_panel_cmds_send(ctrl_pdata, &ctrl_pdata->dsi_panel_cabc_video_cmds);
			break;
		default:
			LCD_LOG_ERR("%s: invalid cabc mode: %d\n", __func__, cabc_cfg.mode);
			break;
	}
	LCD_LOG_INFO("exit %s : CABC mode= %d\n",__func__,cabc_cfg.mode);
	return 0;
}
#endif
/* DTS2014042409252 d00238048 20140505 end> */
static char caset[] = {0x2a, 0x00, 0x00, 0x03, 0x00};	/* DTYPE_DCS_LWRITE */
static char paset[] = {0x2b, 0x00, 0x00, 0x05, 0x00};	/* DTYPE_DCS_LWRITE */

static struct dsi_cmd_desc partial_update_enable_cmd[] = {
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(caset)}, caset},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(paset)}, paset},
};

static int mdss_dsi_panel_partial_update(struct mdss_panel_data *pdata)
{
	struct mipi_panel_info *mipi;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct dcs_cmd_req cmdreq;
	int rc = 0;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
	mipi  = &pdata->panel_info.mipi;

	pr_debug("%s: ctrl=%p ndx=%d\n", __func__, ctrl, ctrl->ndx);

	caset[1] = (((pdata->panel_info.roi_x) & 0xFF00) >> 8);
	caset[2] = (((pdata->panel_info.roi_x) & 0xFF));
	caset[3] = (((pdata->panel_info.roi_x - 1 + pdata->panel_info.roi_w)
								& 0xFF00) >> 8);
	caset[4] = (((pdata->panel_info.roi_x - 1 + pdata->panel_info.roi_w)
								& 0xFF));
	partial_update_enable_cmd[0].payload = caset;

	paset[1] = (((pdata->panel_info.roi_y) & 0xFF00) >> 8);
	paset[2] = (((pdata->panel_info.roi_y) & 0xFF));
	paset[3] = (((pdata->panel_info.roi_y - 1 + pdata->panel_info.roi_h)
								& 0xFF00) >> 8);
	paset[4] = (((pdata->panel_info.roi_y - 1 + pdata->panel_info.roi_h)
								& 0xFF));
	partial_update_enable_cmd[1].payload = paset;

	pr_debug("%s: enabling partial update\n", __func__);
	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = partial_update_enable_cmd;
	cmdreq.cmds_cnt = 2;
	cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mdss_dsi_cmdlist_put(ctrl, &cmdreq);

	return rc;
}

static void mdss_dsi_panel_switch_mode(struct mdss_panel_data *pdata,
							int mode)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mipi_panel_info *mipi;
	struct dsi_panel_cmds *pcmds;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}

	mipi  = &pdata->panel_info.mipi;

	if (!mipi->dynamic_switch_enabled)
		return;

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	if (mode == DSI_CMD_MODE)
		pcmds = &ctrl_pdata->video2cmd;
	else
		pcmds = &ctrl_pdata->cmd2video;

	mdss_dsi_panel_cmds_send(ctrl_pdata, pcmds);

	return;
}

static void mdss_dsi_panel_bl_ctrl(struct mdss_panel_data *pdata,
							u32 bl_level)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	/*
	 * Some backlight controllers specify a minimum duty cycle
	 * for the backlight brightness. If the brightness is less
	 * than it, the controller can malfunction.
	 */

	if ((bl_level < pdata->panel_info.bl_min) && (bl_level != 0))
		bl_level = pdata->panel_info.bl_min;

	switch (ctrl_pdata->bklt_ctrl) {
	case BL_WLED:
		led_trigger_event(bl_led_trigger, bl_level);
		break;
	case BL_PWM:
		mdss_dsi_panel_bklt_pwm(ctrl_pdata, bl_level);
		break;
	case BL_DCS_CMD:
		mdss_dsi_panel_bklt_dcs(ctrl_pdata, bl_level);
		if (mdss_dsi_is_master_ctrl(ctrl_pdata)) {
			struct mdss_dsi_ctrl_pdata *sctrl =
				mdss_dsi_get_slave_ctrl();
			if (!sctrl) {
				pr_err("%s: Invalid slave ctrl data\n",
					__func__);
				return;
			}
			mdss_dsi_panel_bklt_dcs(sctrl, bl_level);
		}
		break;
	default:
		pr_err("%s: Unknown bl_ctrl configuration\n",
			__func__);
		break;
	}
}

/*< DTS2014042905347 zhaoyuxia 20140429 begin */
static int mdss_dsi_panel_on(struct mdss_panel_data *pdata)
{
	struct mipi_panel_info *mipi;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
#ifdef CONFIG_HUAWEI_LCD
	struct dsi_panel_cmds cmds;
#endif
/*DTS2014101301850 zhoujian 20141013 begin */
	unsigned long timeout = jiffies;
/*DTS2014101301850 zhoujian 20141013 end */
	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
	mipi  = &pdata->panel_info.mipi;

	pr_debug("%s: ctrl=%p ndx=%d\n", __func__, ctrl, ctrl->ndx);

#ifndef CONFIG_HUAWEI_LCD
	if (ctrl->on_cmds.cmd_cnt)
		mdss_dsi_panel_cmds_send(ctrl, &ctrl->on_cmds);
#else
	memset(&cmds, sizeof(cmds), 0);
	/* < DTS2014051503541 daiyuhong 20140515 begin */
	/* if inversion mode has been setted open, we open inversion function after reset*/
	if((INVERSION_ON == ctrl->inversion_state) && ctrl ->dsi_panel_inverse_on_cmds.cmd_cnt )
	{
		mdss_dsi_panel_cmds_send(ctrl, &ctrl->dsi_panel_inverse_on_cmds);
		LCD_LOG_DBG("%s:display inversion open:inversion_state = [%d]\n",__func__,ctrl->inversion_state);
	}
	/* DTS2014051503541 daiyuhong 20140515 end > */
	if (enable_initcode_debugging && !hw_parse_dsi_cmds(&cmds))
	{
		LCD_LOG_INFO("read from debug file and write to LCD!\n");
		cmds.link_state = ctrl->on_cmds.link_state;
		if (cmds.cmd_cnt)
			mdss_dsi_panel_cmds_send(ctrl, &cmds);
		hw_free_dsi_cmds(&cmds);
	}
	else
	{
		if (ctrl->on_cmds.cmd_cnt)
			mdss_dsi_panel_cmds_send(ctrl, &ctrl->on_cmds);
	}
/* < DTS2014080106240 renxigang 20140801 begin */
/*< DTS2015012205825  tianye/293347 20150122 begin*/
/* remove APR web LCD report log information  */
#ifdef CONFIG_HUAWEI_DSM
/*DTS2015012205825 tianye/293347 20150122 end >*/
	lcd_pwr_status.lcd_dcm_pwr_status |= BIT(1);
	do_gettimeofday(&lcd_pwr_status.tvl_lcd_on);
	time_to_tm(lcd_pwr_status.tvl_lcd_on.tv_sec, 0, &lcd_pwr_status.tm_lcd_on);
#endif
/* DTS2014080106240 renxigang 20140801 end > */

	LCD_LOG_INFO("exit %s\n",__func__);
#endif
/* < DTS2014050808504 daiyuhong 20140508 begin */
/*set mipi status*/
#if defined(CONFIG_HUAWEI_KERNEL) && defined(CONFIG_DEBUG_FS)
	atomic_set(&mipi_path_status,MIPI_PATH_OPEN);
#endif
/* DTS2014050808504 daiyuhong 20140508 end > */
	pr_debug("%s:-\n", __func__);
/*DTS2014101301850 zhoujian 20141013 begin > */
/* add for timeout print log */
	LCD_LOG_INFO("%s: panel_on_time = %u,offlinecpu = %d,curfreq = %d\n",
			__func__,jiffies_to_msecs(jiffies-timeout),get_offline_cpu(),cpufreq_get(0));
/*DTS2014101301850 zhoujian 20141013 end > */
/*<DTS2014111701156 l00101002 20140924 begin*/
#ifdef CONFIG_LOG_JANK
    pr_jank(JL_KERNEL_LCD_POWER_ON, "%s#T:%5lu", "JL_KERNEL_LCD_POWER_ON",getrealtime());
#endif
/*DTS2014111701156 l00101002 20140924 end>*/
	return 0;
}

static int mdss_dsi_panel_off(struct mdss_panel_data *pdata)
{
	struct mipi_panel_info *mipi;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}
/* < DTS2014050808504 daiyuhong 20140508 begin */
/*set mipi status*/
#if defined(CONFIG_HUAWEI_KERNEL) && defined(CONFIG_DEBUG_FS)
	atomic_set(&mipi_path_status,MIPI_PATH_CLOSE);
#endif
/* DTS2014050808504 daiyuhong 20140508 end > */
	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	pr_debug("%s: ctrl=%p ndx=%d\n", __func__, ctrl, ctrl->ndx);

	mipi  = &pdata->panel_info.mipi;

	if (ctrl->off_cmds.cmd_cnt)
		mdss_dsi_panel_cmds_send(ctrl, &ctrl->off_cmds);

#ifdef CONFIG_HUAWEI_LCD
	LCD_LOG_INFO("exit %s\n",__func__);
#endif
	pr_debug("%s:-\n", __func__);
/*<DTS2014111701156 l00101002 20140924 begin*/
#ifdef CONFIG_LOG_JANK
    pr_jank(JL_KERNEL_LCD_POWER_OFF, "%s#T:%5lu", "JL_KERNEL_LCD_POWER_OFF",getrealtime());
#endif
/*DTS2014111701156 l00101002 20140924 end>*/
	return 0;
}

#ifdef CONFIG_HUAWEI_LCD
static int mdss_dsi_panel_inversion_ctrl(struct mdss_panel_data *pdata,u32 imode)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	int ret = 0;
	if (pdata == NULL) {
		LCD_LOG_ERR("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
	
	switch(imode)
	{
		case COLUMN_INVERSION: //column inversion mode
			if (ctrl_pdata->column_inversion_cmds.cmd_cnt)
				mdss_dsi_panel_cmds_send(ctrl_pdata ,&ctrl_pdata->column_inversion_cmds);
			break;
		case DOT_INVERSION:// dot inversion mode
			if (ctrl_pdata->dot_inversion_cmds.cmd_cnt)
				mdss_dsi_panel_cmds_send(ctrl_pdata ,&ctrl_pdata->dot_inversion_cmds);
			break;
		default:
			ret = -EINVAL;
			LCD_LOG_ERR("%s: invalid inversion mode: %d\n", __func__, imode);
			break;
	}
	LCD_LOG_INFO("exit %s ,dot inversion enable= %d \n",__func__,imode);
	return ret;

}
/* <DTS2014051303765 daiyuhong 20140513 begin */
/*Add status reg check function of tianma ili9806c LCD*/
/* <DTS2014050904959 daiyuhong 20140509 begin */
/* skip read reg when check panel status*/
/* < DTS2014111001776 zhaoyuxia 20141114 begin */
static int mdss_dsi_check_panel_status(struct mdss_panel_data *pdata)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	int ret = 0;
	int count = 5;
	/* < DTS2014060603272 zhengjinzeng 20140606 begin */  
	/* because the max read length is 3, so change returned value buffer size to char*3 */  
	char rdata[3] = {0}; 
	char expect_value = 0x9C;
	int read_length = 1;

	if (pdata == NULL) {
		LCD_LOG_ERR("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
	if(ctrl_pdata->skip_reg_read)
	{
		LCD_LOG_INFO("exit %s ,skip_reg_read=%d, panel status is ok\n",__func__,ctrl_pdata->skip_reg_read);
		return ret;
	}
	else
	{
		if (ctrl_pdata->long_read_flag)
		{
			expect_value = ctrl_pdata->reg_expect_value;
			read_length = ctrl_pdata->reg_expect_count;
		}

		do{
			mdss_dsi_panel_cmd_read(ctrl_pdata,0x0A,0x00,NULL,rdata,read_length);
			LCD_LOG_INFO("exit %s ,0x0A = 0x%x \n",__func__,rdata[0]);
			count--;
		}while((expect_value != rdata[0])&&(count>0));

		/* < DTS2014051603610 zhaoyuxia 20140516 begin */
		if(0 == count)
		{
			ret = -EINVAL;
			lcd_report_dsm_err(DSM_LCD_STATUS_ERROR_NO, rdata[0], 0x0A);
		}
		/* DTS2014051603610 zhaoyuxia 20140516 end > */
		/* DTS2014060603272 zhengjinzeng 20140606 end >*/ 
	
		return ret;
	}
}
/* DTS2014050904959 daiyuhong 20140509 end > */

/*< DTS2014052207729 renxigang 20140520 begin */
/*if Panel IC works well, return 1, else return -1 */
int panel_check_live_status(struct mdss_dsi_ctrl_pdata *ctrl)
{
	int count = 0;
	int j = 0;
	int i = 0;
	/* success on retrun 1,otherwise return 0 or -1 */
	int ret = 1;
	int compare_reg = 0;
	#define MAX_RETURN_COUNT_ESD 4
	#define REPEAT_COUNT 5
	char esd_buf[MAX_RETURN_COUNT_ESD]={0};
	for(j = 0;j < ctrl->esd_cmds.cmd_cnt;j++)
	{
		count = 0;
		do
		{
			compare_reg = 0;
			mdss_dsi_panel_cmd_read(ctrl,ctrl->esd_cmds.cmds[j].payload[0],0x00,NULL,esd_buf,ctrl->esd_cmds.cmds[j].dchdr.dlen - 1);
			count++;
			for(i=0;i<(ctrl->esd_cmds.cmds[j].dchdr.dlen-1);i++)
			{
				if(ctrl->esd_cmds.cmds[j].payload[0] == 0x0d && ctrl->esd_cmds.cmds[j].dchdr.dlen == 2)
				{
					/*if ctrl->inversion_state is 1,ctrl->inversion_state<<5 equal 0x20*/
					/*if ctrl->inversion_state is 0,ctrl->inversion_state<<5 equal 0x00*/
					/*ctrl->esd_cmds.cmds[j].payload[1] equal 0x00*/
					/*when we read the 0x0d register,0x20 means enter color inversion mode when inversion mode on,not esd issue*/
					ctrl->esd_cmds.cmds[j].payload[1] = ctrl->esd_cmds.cmds[j].payload[1] | (ctrl->inversion_state<<5);					
				}
				if(esd_buf[i] != ctrl->esd_cmds.cmds[j].payload[i+1])
				{
					compare_reg = -1;
					/* < DTS2014062405194 songliangliang 20140624 begin */
					/* add for esd error*/
					lcd_report_dsm_err(DSM_LCD_ESD_STATUS_ERROR_NO,esd_buf[i] , ctrl->esd_cmds.cmds[j].payload[0]);
					/* < DTS2014062405194 songliangliang 20140624 end */
					LCD_LOG_ERR("panel ic error: in %s,  reg 0x%02X %d byte should be 0x%02x, but read data =0x%02x \n",
						__func__,ctrl->esd_cmds.cmds[j].payload[0],i+1,ctrl->esd_cmds.cmds[j].payload[i+1],esd_buf[i]);
					break;
				}
			}
		}while((count<REPEAT_COUNT)&&(compare_reg == -1));
		if(count == REPEAT_COUNT)
		{
			/* < DTS2014062405194 songliangliang 20140624 begin */
			/* add for esd error*/
			lcd_report_dsm_err(DSM_LCD_ESD_REBOOT_ERROR_NO,esd_buf[i] , ctrl->esd_cmds.cmds[j].payload[0]);
			/* < DTS2014062405194 songliangliang 20140624 end */
			ret = -1;
			goto out;
		}
	}
out:
	return ret;
}
/* DTS2014111001776 zhaoyuxia 20141114 end > */
/* DTS2014052207729 renxigang 20140520 end >*/

/* < DTS2014051503541 daiyuhong 20140515 begin */
/*Add display color inversion function*/
static int mdss_dsi_lcd_set_display_inversion(struct mdss_panel_data *pdata,unsigned int inversion_mode)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	switch(inversion_mode)
	{
		/* in each inversion mode,we send the corresponding commonds and reset inversion state */
		case INVERSION_OFF:
			if (ctrl_pdata->dsi_panel_inverse_off_cmds.cmd_cnt)
				mdss_dsi_panel_cmds_send(ctrl_pdata, &ctrl_pdata->dsi_panel_inverse_off_cmds);
			ctrl_pdata->inversion_state = INVERSION_OFF;
			break;
		case INVERSION_ON:
			if (ctrl_pdata->dsi_panel_inverse_on_cmds.cmd_cnt)
				mdss_dsi_panel_cmds_send(ctrl_pdata, &ctrl_pdata->dsi_panel_inverse_on_cmds);
			ctrl_pdata->inversion_state = INVERSION_ON;
			break;
		default:
			LCD_LOG_ERR("%s: invalid inversion mode: %d\n", __func__,inversion_mode);
			break;
	}
	LCD_LOG_INFO("exit %s : inversion mode= %d\n",__func__,inversion_mode);
	return 0;
}

/* DTS2014051503541 daiyuhong 20140515 end > */
/*< DTS2014042818102 zhengjinzeng 20140428 begin */
static int mdss_dsi_check_mipi_crc(struct mdss_panel_data *pdata)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	int ret = 0;
	char err_num = 0;
	int read_length = 1;

	if (pdata == NULL){
		LCD_LOG_ERR("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	mdss_dsi_panel_cmd_read(ctrl_pdata,0x05,0x00,NULL,&err_num,read_length);
	LCD_LOG_INFO("exit %s,read 0x05 = %d.\n",__func__,err_num);

	if(err_num)
	{
		LCD_LOG_ERR("%s: mipi crc has %d errors.\n",__func__,err_num);
		ret = err_num;
	}
	return ret;
}
/* DTS2014042818102 zhengjinzeng 20140428 end >*/
#endif
/* DTS2014042905347 zhaoyuxia 20140429 end > */
/* DTS2014051303765 daiyuhong 20140513 end> */
static void mdss_dsi_parse_lane_swap(struct device_node *np, char *dlane_swap)
{
	const char *data;

	*dlane_swap = DSI_LANE_MAP_0123;
	data = of_get_property(np, "qcom,mdss-dsi-lane-map", NULL);
	if (data) {
		if (!strcmp(data, "lane_map_3012"))
			*dlane_swap = DSI_LANE_MAP_3012;
		else if (!strcmp(data, "lane_map_2301"))
			*dlane_swap = DSI_LANE_MAP_2301;
		else if (!strcmp(data, "lane_map_1230"))
			*dlane_swap = DSI_LANE_MAP_1230;
		else if (!strcmp(data, "lane_map_0321"))
			*dlane_swap = DSI_LANE_MAP_0321;
		else if (!strcmp(data, "lane_map_1032"))
			*dlane_swap = DSI_LANE_MAP_1032;
		else if (!strcmp(data, "lane_map_2103"))
			*dlane_swap = DSI_LANE_MAP_2103;
		else if (!strcmp(data, "lane_map_3210"))
			*dlane_swap = DSI_LANE_MAP_3210;
	}
}

static void mdss_dsi_parse_trigger(struct device_node *np, char *trigger,
		char *trigger_key)
{
	const char *data;

	*trigger = DSI_CMD_TRIGGER_SW;
	data = of_get_property(np, trigger_key, NULL);
	if (data) {
		if (!strcmp(data, "none"))
			*trigger = DSI_CMD_TRIGGER_NONE;
		else if (!strcmp(data, "trigger_te"))
			*trigger = DSI_CMD_TRIGGER_TE;
		else if (!strcmp(data, "trigger_sw_seof"))
			*trigger = DSI_CMD_TRIGGER_SW_SEOF;
		else if (!strcmp(data, "trigger_sw_te"))
			*trigger = DSI_CMD_TRIGGER_SW_TE;
	}
}


static int mdss_dsi_parse_dcs_cmds(struct device_node *np,
		struct dsi_panel_cmds *pcmds, char *cmd_key, char *link_key)
{
	const char *data;
	int blen = 0, len;
	char *buf, *bp;
	struct dsi_ctrl_hdr *dchdr;
	int i, cnt;

	data = of_get_property(np, cmd_key, &blen);
	if (!data) {
		pr_err("%s: failed, key=%s\n", __func__, cmd_key);
		return -ENOMEM;
	}

	buf = kzalloc(sizeof(char) * blen, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	memcpy(buf, data, blen);

	/* scan dcs commands */
	bp = buf;
	len = blen;
	cnt = 0;
	while (len >= sizeof(*dchdr)) {
		dchdr = (struct dsi_ctrl_hdr *)bp;
		dchdr->dlen = ntohs(dchdr->dlen);
		if (dchdr->dlen > len) {
			pr_err("%s: dtsi cmd=%x error, len=%d",
				__func__, dchdr->dtype, dchdr->dlen);
			goto exit_free;
		}
		bp += sizeof(*dchdr);
		len -= sizeof(*dchdr);
		bp += dchdr->dlen;
		len -= dchdr->dlen;
		cnt++;
	}

	if (len != 0) {
		pr_err("%s: dcs_cmd=%x len=%d error!",
				__func__, buf[0], blen);
		goto exit_free;
	}

	pcmds->cmds = kzalloc(cnt * sizeof(struct dsi_cmd_desc),
						GFP_KERNEL);
	if (!pcmds->cmds)
		goto exit_free;

	pcmds->cmd_cnt = cnt;
	pcmds->buf = buf;
	pcmds->blen = blen;

	bp = buf;
	len = blen;
	for (i = 0; i < cnt; i++) {
		dchdr = (struct dsi_ctrl_hdr *)bp;
		len -= sizeof(*dchdr);
		bp += sizeof(*dchdr);
		pcmds->cmds[i].dchdr = *dchdr;
		pcmds->cmds[i].payload = bp;
		bp += dchdr->dlen;
		len -= dchdr->dlen;
	}

	/*Set default link state to LP Mode*/
	pcmds->link_state = DSI_LP_MODE;

	if (link_key) {
		data = of_get_property(np, link_key, NULL);
		if (data && !strcmp(data, "dsi_hs_mode"))
			pcmds->link_state = DSI_HS_MODE;
		else
			pcmds->link_state = DSI_LP_MODE;
	}

	pr_debug("%s: dcs_cmd=%x len=%d, cmd_cnt=%d link_state=%d\n", __func__,
		pcmds->buf[0], pcmds->blen, pcmds->cmd_cnt, pcmds->link_state);

	return 0;

exit_free:
	kfree(buf);
	return -ENOMEM;
}


int mdss_panel_get_dst_fmt(u32 bpp, char mipi_mode, u32 pixel_packing,
				char *dst_format)
{
	int rc = 0;
	switch (bpp) {
	case 3:
		*dst_format = DSI_CMD_DST_FORMAT_RGB111;
		break;
	case 8:
		*dst_format = DSI_CMD_DST_FORMAT_RGB332;
		break;
	case 12:
		*dst_format = DSI_CMD_DST_FORMAT_RGB444;
		break;
	case 16:
		switch (mipi_mode) {
		case DSI_VIDEO_MODE:
			*dst_format = DSI_VIDEO_DST_FORMAT_RGB565;
			break;
		case DSI_CMD_MODE:
			*dst_format = DSI_CMD_DST_FORMAT_RGB565;
			break;
		default:
			*dst_format = DSI_VIDEO_DST_FORMAT_RGB565;
			break;
		}
		break;
	case 18:
		switch (mipi_mode) {
		case DSI_VIDEO_MODE:
			if (pixel_packing == 0)
				*dst_format = DSI_VIDEO_DST_FORMAT_RGB666;
			else
				*dst_format = DSI_VIDEO_DST_FORMAT_RGB666_LOOSE;
			break;
		case DSI_CMD_MODE:
			*dst_format = DSI_CMD_DST_FORMAT_RGB666;
			break;
		default:
			if (pixel_packing == 0)
				*dst_format = DSI_VIDEO_DST_FORMAT_RGB666;
			else
				*dst_format = DSI_VIDEO_DST_FORMAT_RGB666_LOOSE;
			break;
		}
		break;
	case 24:
		switch (mipi_mode) {
		case DSI_VIDEO_MODE:
			*dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
			break;
		case DSI_CMD_MODE:
			*dst_format = DSI_CMD_DST_FORMAT_RGB888;
			break;
		default:
			*dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
			break;
		}
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}


static int mdss_dsi_parse_fbc_params(struct device_node *np,
				struct mdss_panel_info *panel_info)
{
	int rc, fbc_enabled = 0;
	u32 tmp;

	fbc_enabled = of_property_read_bool(np,	"qcom,mdss-dsi-fbc-enable");
	if (fbc_enabled) {
		pr_debug("%s:%d FBC panel enabled.\n", __func__, __LINE__);
		panel_info->fbc.enabled = 1;
		rc = of_property_read_u32(np, "qcom,mdss-dsi-fbc-bpp", &tmp);
		panel_info->fbc.target_bpp =	(!rc ? tmp : panel_info->bpp);
		rc = of_property_read_u32(np, "qcom,mdss-dsi-fbc-packing",
				&tmp);
		panel_info->fbc.comp_mode = (!rc ? tmp : 0);
		panel_info->fbc.qerr_enable = of_property_read_bool(np,
			"qcom,mdss-dsi-fbc-quant-error");
		rc = of_property_read_u32(np, "qcom,mdss-dsi-fbc-bias", &tmp);
		panel_info->fbc.cd_bias = (!rc ? tmp : 0);
		panel_info->fbc.pat_enable = of_property_read_bool(np,
				"qcom,mdss-dsi-fbc-pat-mode");
		panel_info->fbc.vlc_enable = of_property_read_bool(np,
				"qcom,mdss-dsi-fbc-vlc-mode");
		panel_info->fbc.bflc_enable = of_property_read_bool(np,
				"qcom,mdss-dsi-fbc-bflc-mode");
		rc = of_property_read_u32(np, "qcom,mdss-dsi-fbc-h-line-budget",
				&tmp);
		panel_info->fbc.line_x_budget = (!rc ? tmp : 0);
		rc = of_property_read_u32(np, "qcom,mdss-dsi-fbc-budget-ctrl",
				&tmp);
		panel_info->fbc.block_x_budget = (!rc ? tmp : 0);
		rc = of_property_read_u32(np, "qcom,mdss-dsi-fbc-block-budget",
				&tmp);
		panel_info->fbc.block_budget = (!rc ? tmp : 0);
		rc = of_property_read_u32(np,
				"qcom,mdss-dsi-fbc-lossless-threshold", &tmp);
		panel_info->fbc.lossless_mode_thd = (!rc ? tmp : 0);
		rc = of_property_read_u32(np,
				"qcom,mdss-dsi-fbc-lossy-threshold", &tmp);
		panel_info->fbc.lossy_mode_thd = (!rc ? tmp : 0);
		rc = of_property_read_u32(np, "qcom,mdss-dsi-fbc-rgb-threshold",
				&tmp);
		panel_info->fbc.lossy_rgb_thd = (!rc ? tmp : 0);
		rc = of_property_read_u32(np,
				"qcom,mdss-dsi-fbc-lossy-mode-idx", &tmp);
		panel_info->fbc.lossy_mode_idx = (!rc ? tmp : 0);
	} else {
		pr_debug("%s:%d Panel does not support FBC.\n",
				__func__, __LINE__);
		panel_info->fbc.enabled = 0;
		panel_info->fbc.target_bpp =
			panel_info->bpp;
	}
	return 0;
}

static void mdss_panel_parse_te_params(struct device_node *np,
				       struct mdss_panel_info *panel_info)
{

	u32 tmp;
	int rc = 0;
	/*
	 * TE default: dsi byte clock calculated base on 70 fps;
	 * around 14 ms to complete a kickoff cycle if te disabled;
	 * vclk_line base on 60 fps; write is faster than read;
	 * init == start == rdptr;
	 */
	panel_info->te.tear_check_en =
		!of_property_read_bool(np, "qcom,mdss-tear-check-disable");
	rc = of_property_read_u32
		(np, "qcom,mdss-tear-check-sync-cfg-height", &tmp);
	panel_info->te.sync_cfg_height = (!rc ? tmp : 0xfff0);
	rc = of_property_read_u32
		(np, "qcom,mdss-tear-check-sync-init-val", &tmp);
	panel_info->te.vsync_init_val = (!rc ? tmp : panel_info->yres);
	rc = of_property_read_u32
		(np, "qcom,mdss-tear-check-sync-threshold-start", &tmp);
	panel_info->te.sync_threshold_start = (!rc ? tmp : 4);
	rc = of_property_read_u32
		(np, "qcom,mdss-tear-check-sync-threshold-continue", &tmp);
	panel_info->te.sync_threshold_continue = (!rc ? tmp : 4);
	rc = of_property_read_u32(np, "qcom,mdss-tear-check-start-pos", &tmp);
	panel_info->te.start_pos = (!rc ? tmp : panel_info->yres);
	rc = of_property_read_u32
		(np, "qcom,mdss-tear-check-rd-ptr-trigger-intr", &tmp);
	panel_info->te.rd_ptr_irq = (!rc ? tmp : panel_info->yres + 1);
	rc = of_property_read_u32(np, "qcom,mdss-tear-check-frame-rate", &tmp);
	panel_info->te.refx100 = (!rc ? tmp : 6000);
}


static int mdss_dsi_parse_reset_seq(struct device_node *np,
		u32 rst_seq[MDSS_DSI_RST_SEQ_LEN], u32 *rst_len,
		const char *name)
{
	int num = 0, i;
	int rc;
	struct property *data;
	u32 tmp[MDSS_DSI_RST_SEQ_LEN];
	*rst_len = 0;
	data = of_find_property(np, name, &num);
	num /= sizeof(u32);
	if (!data || !num || num > MDSS_DSI_RST_SEQ_LEN || num % 2) {
		pr_debug("%s:%d, error reading %s, length found = %d\n",
			__func__, __LINE__, name, num);
	} else {
		rc = of_property_read_u32_array(np, name, tmp, num);
		if (rc)
			pr_debug("%s:%d, error reading %s, rc = %d\n",
				__func__, __LINE__, name, rc);
		else {
			for (i = 0; i < num; ++i)
				rst_seq[i] = tmp[i];
			*rst_len = num;
		}
	}
	return 0;
}

static int mdss_dsi_gen_read_status(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	if (ctrl_pdata->status_buf.data[0] !=
					ctrl_pdata->status_value) {
		pr_err("%s: Read back value from panel is incorrect\n",
							__func__);
		return -EINVAL;
	} else {
		return 1;
	}
}

static int mdss_dsi_nt35596_read_status(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	if (ctrl_pdata->status_buf.data[0] !=
					ctrl_pdata->status_value) {
		ctrl_pdata->status_error_count = 0;
		pr_err("%s: Read back value from panel is incorrect\n",
							__func__);
		return -EINVAL;
	} else {
		if (ctrl_pdata->status_buf.data[3] != NT35596_BUF_3_STATUS) {
			ctrl_pdata->status_error_count = 0;
		} else {
			if ((ctrl_pdata->status_buf.data[4] ==
				NT35596_BUF_4_STATUS) ||
				(ctrl_pdata->status_buf.data[5] ==
				NT35596_BUF_5_STATUS))
				ctrl_pdata->status_error_count = 0;
			else
				ctrl_pdata->status_error_count++;
			if (ctrl_pdata->status_error_count >=
					NT35596_MAX_ERR_CNT) {
				ctrl_pdata->status_error_count = 0;
				pr_err("%s: Read value bad. Error_cnt = %i\n",
					 __func__,
					ctrl_pdata->status_error_count);
				return -EINVAL;
			}
		}
		return 1;
	}
}

static void mdss_dsi_parse_roi_alignment(struct device_node *np,
		struct mdss_panel_info *pinfo)
{
	int len = 0;
	u32 value[6];
	struct property *data;
	data = of_find_property(np, "qcom,panel-roi-alignment", &len);
	len /= sizeof(u32);
	if (!data || (len != 6)) {
		pr_debug("%s: Panel roi alignment not found", __func__);
	} else {
		int rc = of_property_read_u32_array(np,
				"qcom,panel-roi-alignment", value, len);
		if (rc)
			pr_debug("%s: Error reading panel roi alignment values",
					__func__);
		else {
			pinfo->xstart_pix_align = value[0];
			pinfo->width_pix_align = value[1];
			pinfo->ystart_pix_align = value[2];
			pinfo->height_pix_align = value[3];
			pinfo->min_width = value[4];
			pinfo->min_height = value[5];
		}

		pr_debug("%s: ROI alignment: [%d, %d, %d, %d, %d, %d]",
				__func__, pinfo->xstart_pix_align,
				pinfo->width_pix_align, pinfo->ystart_pix_align,
				pinfo->height_pix_align, pinfo->min_width,
				pinfo->min_height);
	}
}

static int mdss_dsi_parse_panel_features(struct device_node *np,
	struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct mdss_panel_info *pinfo;

	if (!np || !ctrl) {
		pr_err("%s: Invalid arguments\n", __func__);
		return -ENODEV;
	}

	pinfo = &ctrl->panel_data.panel_info;

	pinfo->cont_splash_enabled = of_property_read_bool(np,
		"qcom,cont-splash-enabled");

	pinfo->partial_update_enabled = of_property_read_bool(np,
		"qcom,partial-update-enabled");
	pr_info("%s:%d Partial update %s\n", __func__, __LINE__,
		(pinfo->partial_update_enabled ? "enabled" : "disabled"));
	if (pinfo->partial_update_enabled)
		ctrl->partial_update_fnc = mdss_dsi_panel_partial_update;

	pinfo->ulps_feature_enabled = of_property_read_bool(np,
		"qcom,ulps-enabled");
	pr_info("%s: ulps feature %s", __func__,
		(pinfo->ulps_feature_enabled ? "enabled" : "disabled"));
	pinfo->ulps_suspend_enabled = of_property_read_bool(np,
		"qcom,suspend-ulps-enabled");
	pr_info("%s: ulps during suspend feature %s", __func__,
		(pinfo->ulps_suspend_enabled ? "enabled" : "disabled"));
	pinfo->esd_check_enabled = of_property_read_bool(np,
		"qcom,esd-check-enabled");

	pinfo->mipi.dynamic_switch_enabled = of_property_read_bool(np,
		"qcom,dynamic-mode-switch-enabled");

	if (pinfo->mipi.dynamic_switch_enabled) {
		mdss_dsi_parse_dcs_cmds(np, &ctrl->video2cmd,
			"qcom,video-to-cmd-mode-switch-commands", NULL);

		mdss_dsi_parse_dcs_cmds(np, &ctrl->cmd2video,
			"qcom,cmd-to-video-mode-switch-commands", NULL);

		if (!ctrl->video2cmd.cmd_cnt || !ctrl->cmd2video.cmd_cnt) {
			pr_warn("No commands specified for dynamic switch\n");
			pinfo->mipi.dynamic_switch_enabled = 0;
		}
	}

	pr_info("%s: dynamic switch feature enabled: %d", __func__,
		pinfo->mipi.dynamic_switch_enabled);

	return 0;
}

/*< DTS2014052207729 renxigang 20140520 begin */
/***************************************************************
Function: mdss_paned_parse_esd_dt
Description: parse the esd concerned setting
Parameters:
	struct device_node *np: device node
	struct mdss_dsi_ctrl_pdata *ctrl_pdata: dsi control parameter struct
Return:0:success
Return:-ENOMEM:fail
***************************************************************/
#ifdef CONFIG_HUAWEI_LCD
static void mdss_panel_parse_esd_dt(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc;

	ctrl_pdata->esd_check_enable = of_property_read_bool(np, "qcom,panel-esd-check-enabled");
	/*< DTS2014061007154 renxigang 20140610 begin */
	if(ctrl_pdata->esd_check_enable)
	{
		rc = mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->esd_cmds,
			"qcom,panel-esd-read-commands","qcom,esd-read-cmds-state");
		if(rc < 0)
		{
			pr_err("%s:%d, parse esd command fail,esd check disabled\n",
					__func__, __LINE__);
			ctrl_pdata->esd_check_enable = false;
		}
		else
		{
			pr_info("%s:, esd check enabled \n",
					__func__);
		}
	}
	else
	{
		pr_info("%s:, esd check not enabled \n",
					__func__);
	}
	/* DTS2014061007154 renxigang 20140610 end >*/
}
#endif
/* DTS2014052207729 renxigang 20140520 end >*/

static int mdss_panel_parse_dt(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	u32 tmp;
	int rc, i, len;
	const char *data;
	static const char *pdest;
	struct mdss_panel_info *pinfo = &(ctrl_pdata->panel_data.panel_info);

	rc = of_property_read_u32(np, "qcom,mdss-dsi-panel-width", &tmp);
	if (rc) {
		pr_err("%s:%d, panel width not specified\n",
						__func__, __LINE__);
		return -EINVAL;
	}
	pinfo->xres = (!rc ? tmp : 640);

	rc = of_property_read_u32(np, "qcom,mdss-dsi-panel-height", &tmp);
	if (rc) {
		pr_err("%s:%d, panel height not specified\n",
						__func__, __LINE__);
		return -EINVAL;
	}
	pinfo->yres = (!rc ? tmp : 480);

	rc = of_property_read_u32(np,
		"qcom,mdss-pan-physical-width-dimension", &tmp);
	pinfo->physical_width = (!rc ? tmp : 0);
	rc = of_property_read_u32(np,
		"qcom,mdss-pan-physical-height-dimension", &tmp);
	pinfo->physical_height = (!rc ? tmp : 0);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-h-left-border", &tmp);
	pinfo->lcdc.xres_pad = (!rc ? tmp : 0);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-h-right-border", &tmp);
	if (!rc)
		pinfo->lcdc.xres_pad += tmp;
	rc = of_property_read_u32(np, "qcom,mdss-dsi-v-top-border", &tmp);
	pinfo->lcdc.yres_pad = (!rc ? tmp : 0);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-v-bottom-border", &tmp);
	if (!rc)
		pinfo->lcdc.yres_pad += tmp;
	rc = of_property_read_u32(np, "qcom,mdss-dsi-bpp", &tmp);
	if (rc) {
		pr_err("%s:%d, bpp not specified\n", __func__, __LINE__);
		return -EINVAL;
	}
	pinfo->bpp = (!rc ? tmp : 24);
	pinfo->mipi.mode = DSI_VIDEO_MODE;
	data = of_get_property(np, "qcom,mdss-dsi-panel-type", NULL);
	if (data && !strncmp(data, "dsi_cmd_mode", 12))
		pinfo->mipi.mode = DSI_CMD_MODE;
	tmp = 0;
	data = of_get_property(np, "qcom,mdss-dsi-pixel-packing", NULL);
	if (data && !strcmp(data, "loose"))
		pinfo->mipi.pixel_packing = 1;
	else
		pinfo->mipi.pixel_packing = 0;
	rc = mdss_panel_get_dst_fmt(pinfo->bpp,
		pinfo->mipi.mode, pinfo->mipi.pixel_packing,
		&(pinfo->mipi.dst_format));
	if (rc) {
		pr_debug("%s: problem determining dst format. Set Default\n",
			__func__);
		pinfo->mipi.dst_format =
			DSI_VIDEO_DST_FORMAT_RGB888;
	}
	pdest = of_get_property(np,
		"qcom,mdss-dsi-panel-destination", NULL);

	if (pdest) {
		if (strlen(pdest) != 9) {
			pr_err("%s: Unknown pdest specified\n", __func__);
			return -EINVAL;
		}
		if (!strcmp(pdest, "display_1"))
			pinfo->pdest = DISPLAY_1;
		else if (!strcmp(pdest, "display_2"))
			pinfo->pdest = DISPLAY_2;
		else {
			pr_debug("%s: incorrect pdest. Set Default\n",
				__func__);
			pinfo->pdest = DISPLAY_1;
		}
	} else {
		pr_debug("%s: pdest not specified. Set Default\n",
				__func__);
		pinfo->pdest = DISPLAY_1;
	}
	rc = of_property_read_u32(np, "qcom,mdss-dsi-h-front-porch", &tmp);
	pinfo->lcdc.h_front_porch = (!rc ? tmp : 6);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-h-back-porch", &tmp);
	pinfo->lcdc.h_back_porch = (!rc ? tmp : 6);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-h-pulse-width", &tmp);
	pinfo->lcdc.h_pulse_width = (!rc ? tmp : 2);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-h-sync-skew", &tmp);
	pinfo->lcdc.hsync_skew = (!rc ? tmp : 0);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-v-back-porch", &tmp);
	pinfo->lcdc.v_back_porch = (!rc ? tmp : 6);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-v-front-porch", &tmp);
	pinfo->lcdc.v_front_porch = (!rc ? tmp : 6);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-v-pulse-width", &tmp);
	pinfo->lcdc.v_pulse_width = (!rc ? tmp : 2);
	rc = of_property_read_u32(np,
		"qcom,mdss-dsi-underflow-color", &tmp);
	pinfo->lcdc.underflow_clr = (!rc ? tmp : 0xff);
	rc = of_property_read_u32(np,
		"qcom,mdss-dsi-border-color", &tmp);
	pinfo->lcdc.border_clr = (!rc ? tmp : 0);
	data = of_get_property(np, "qcom,mdss-dsi-panel-orientation", NULL);
	if (data) {
		pr_debug("panel orientation is %s\n", data);
		if (!strcmp(data, "180"))
			pinfo->panel_orientation = MDP_ROT_180;
		else if (!strcmp(data, "hflip"))
			pinfo->panel_orientation = MDP_FLIP_LR;
		else if (!strcmp(data, "vflip"))
			pinfo->panel_orientation = MDP_FLIP_UD;
	}

	ctrl_pdata->bklt_ctrl = UNKNOWN_CTRL;
	data = of_get_property(np, "qcom,mdss-dsi-bl-pmic-control-type", NULL);
	if (data) {
		if (!strncmp(data, "bl_ctrl_wled", 12)) {
			led_trigger_register_simple("bkl-trigger",
				&bl_led_trigger);
			pr_debug("%s: SUCCESS-> WLED TRIGGER register\n",
				__func__);
			ctrl_pdata->bklt_ctrl = BL_WLED;
		} else if (!strncmp(data, "bl_ctrl_pwm", 11)) {
			ctrl_pdata->bklt_ctrl = BL_PWM;
			rc = of_property_read_u32(np,
				"qcom,mdss-dsi-bl-pmic-pwm-frequency", &tmp);
			if (rc) {
				pr_err("%s:%d, Error, panel pwm_period\n",
						__func__, __LINE__);
				return -EINVAL;
			}
			ctrl_pdata->pwm_period = tmp;
			rc = of_property_read_u32(np,
				"qcom,mdss-dsi-bl-pmic-bank-select", &tmp);
			if (rc) {
				pr_err("%s:%d, Error, dsi lpg channel\n",
						__func__, __LINE__);
				return -EINVAL;
			}
			ctrl_pdata->pwm_lpg_chan = tmp;
			tmp = of_get_named_gpio(np,
				"qcom,mdss-dsi-pwm-gpio", 0);
			ctrl_pdata->pwm_pmic_gpio = tmp;
		} else if (!strncmp(data, "bl_ctrl_dcs", 11)) {
			ctrl_pdata->bklt_ctrl = BL_DCS_CMD;
		}
	}
	rc = of_property_read_u32(np, "qcom,mdss-brightness-max-level", &tmp);
	pinfo->brightness_max = (!rc ? tmp : MDSS_MAX_BL_BRIGHTNESS);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-bl-min-level", &tmp);
	pinfo->bl_min = (!rc ? tmp : 0);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-bl-max-level", &tmp);
	pinfo->bl_max = (!rc ? tmp : 255);
	ctrl_pdata->bklt_max = pinfo->bl_max;

	rc = of_property_read_u32(np, "qcom,mdss-dsi-interleave-mode", &tmp);
	pinfo->mipi.interleave_mode = (!rc ? tmp : 0);

	pinfo->mipi.vsync_enable = of_property_read_bool(np,
		"qcom,mdss-dsi-te-check-enable");
	pinfo->mipi.hw_vsync_mode = of_property_read_bool(np,
		"qcom,mdss-dsi-te-using-te-pin");

	rc = of_property_read_u32(np,
		"qcom,mdss-dsi-h-sync-pulse", &tmp);
	pinfo->mipi.pulse_mode_hsa_he = (!rc ? tmp : false);

	pinfo->mipi.hfp_power_stop = of_property_read_bool(np,
		"qcom,mdss-dsi-hfp-power-mode");
	pinfo->mipi.hsa_power_stop = of_property_read_bool(np,
		"qcom,mdss-dsi-hsa-power-mode");
	pinfo->mipi.hbp_power_stop = of_property_read_bool(np,
		"qcom,mdss-dsi-hbp-power-mode");
	pinfo->mipi.last_line_interleave_en = of_property_read_bool(np,
		"qcom,mdss-dsi-last-line-interleave");
	pinfo->mipi.bllp_power_stop = of_property_read_bool(np,
		"qcom,mdss-dsi-bllp-power-mode");
	pinfo->mipi.eof_bllp_power_stop = of_property_read_bool(
		np, "qcom,mdss-dsi-bllp-eof-power-mode");
	pinfo->mipi.traffic_mode = DSI_NON_BURST_SYNCH_PULSE;
	data = of_get_property(np, "qcom,mdss-dsi-traffic-mode", NULL);
	if (data) {
		if (!strcmp(data, "non_burst_sync_event"))
			pinfo->mipi.traffic_mode = DSI_NON_BURST_SYNCH_EVENT;
		else if (!strcmp(data, "burst_mode"))
			pinfo->mipi.traffic_mode = DSI_BURST_MODE;
	}
	rc = of_property_read_u32(np,
		"qcom,mdss-dsi-te-dcs-command", &tmp);
	pinfo->mipi.insert_dcs_cmd =
			(!rc ? tmp : 1);
	rc = of_property_read_u32(np,
		"qcom,mdss-dsi-wr-mem-continue", &tmp);
	pinfo->mipi.wr_mem_continue =
			(!rc ? tmp : 0x3c);
	rc = of_property_read_u32(np,
		"qcom,mdss-dsi-wr-mem-start", &tmp);
	pinfo->mipi.wr_mem_start =
			(!rc ? tmp : 0x2c);
	rc = of_property_read_u32(np,
		"qcom,mdss-dsi-te-pin-select", &tmp);
	pinfo->mipi.te_sel =
			(!rc ? tmp : 1);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-virtual-channel-id", &tmp);
	pinfo->mipi.vc = (!rc ? tmp : 0);
	pinfo->mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	data = of_get_property(np, "qcom,mdss-dsi-color-order", NULL);
	if (data) {
		if (!strcmp(data, "rgb_swap_rbg"))
			pinfo->mipi.rgb_swap = DSI_RGB_SWAP_RBG;
		else if (!strcmp(data, "rgb_swap_bgr"))
			pinfo->mipi.rgb_swap = DSI_RGB_SWAP_BGR;
		else if (!strcmp(data, "rgb_swap_brg"))
			pinfo->mipi.rgb_swap = DSI_RGB_SWAP_BRG;
		else if (!strcmp(data, "rgb_swap_grb"))
			pinfo->mipi.rgb_swap = DSI_RGB_SWAP_GRB;
		else if (!strcmp(data, "rgb_swap_gbr"))
			pinfo->mipi.rgb_swap = DSI_RGB_SWAP_GBR;
	}
	pinfo->mipi.data_lane0 = of_property_read_bool(np,
		"qcom,mdss-dsi-lane-0-state");
	pinfo->mipi.data_lane1 = of_property_read_bool(np,
		"qcom,mdss-dsi-lane-1-state");
	pinfo->mipi.data_lane2 = of_property_read_bool(np,
		"qcom,mdss-dsi-lane-2-state");
	pinfo->mipi.data_lane3 = of_property_read_bool(np,
		"qcom,mdss-dsi-lane-3-state");

	rc = of_property_read_u32(np, "qcom,mdss-dsi-t-clk-pre", &tmp);
	pinfo->mipi.t_clk_pre = (!rc ? tmp : 0x24);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-t-clk-post", &tmp);
	pinfo->mipi.t_clk_post = (!rc ? tmp : 0x03);

	pinfo->mipi.rx_eot_ignore = of_property_read_bool(np,
		"qcom,mdss-dsi-rx-eot-ignore");
	pinfo->mipi.tx_eot_append = of_property_read_bool(np,
		"qcom,mdss-dsi-tx-eot-append");

	rc = of_property_read_u32(np, "qcom,mdss-dsi-stream", &tmp);
	pinfo->mipi.stream = (!rc ? tmp : 0);

	data = of_get_property(np, "qcom,mdss-dsi-panel-mode-gpio-state", NULL);
	if (data) {
		if (!strcmp(data, "high"))
			pinfo->mode_gpio_state = MODE_GPIO_HIGH;
		else if (!strcmp(data, "low"))
			pinfo->mode_gpio_state = MODE_GPIO_LOW;
	} else {
		pinfo->mode_gpio_state = MODE_GPIO_NOT_VALID;
	}

	rc = of_property_read_u32(np, "qcom,mdss-dsi-panel-framerate", &tmp);
	pinfo->mipi.frame_rate = (!rc ? tmp : 60);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-panel-clockrate", &tmp);
	pinfo->clk_rate = (!rc ? tmp : 0);
	data = of_get_property(np, "qcom,mdss-dsi-panel-timings", &len);
	if ((!data) || (len != 12)) {
		pr_err("%s:%d, Unable to read Phy timing settings",
		       __func__, __LINE__);
		goto error;
	}
	for (i = 0; i < len; i++)
		pinfo->mipi.dsi_phy_db.timing[i] = data[i];

	pinfo->mipi.lp11_init = of_property_read_bool(np,
					"qcom,mdss-dsi-lp11-init");
	rc = of_property_read_u32(np, "qcom,mdss-dsi-init-delay-us", &tmp);
	pinfo->mipi.init_delay = (!rc ? tmp : 0);
/*< DTS2014103106441 huangli/hwx207935 20141031 begin*/
	#ifdef CONFIG_HUAWEI_LCD
	rc = of_property_read_u32(np, "qcom,mdss-dsi-mipi-rst-delay", &tmp);
	pinfo->mipi_rest_delay = (!rc ? tmp : 0);
	#endif
/*DTS2014103106441 huangli/hwx207935 20141031 end >*/
	mdss_dsi_parse_roi_alignment(np, pinfo);

	mdss_dsi_parse_trigger(np, &(pinfo->mipi.mdp_trigger),
		"qcom,mdss-dsi-mdp-trigger");

	mdss_dsi_parse_trigger(np, &(pinfo->mipi.dma_trigger),
		"qcom,mdss-dsi-dma-trigger");

	mdss_dsi_parse_lane_swap(np, &(pinfo->mipi.dlane_swap));

	mdss_dsi_parse_fbc_params(np, pinfo);

	mdss_panel_parse_te_params(np, pinfo);

	mdss_dsi_parse_reset_seq(np, pinfo->rst_seq, &(pinfo->rst_seq_len),
		"qcom,mdss-dsi-reset-sequence");

	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->on_cmds,
		"qcom,mdss-dsi-on-command", "qcom,mdss-dsi-on-command-state");

	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->off_cmds,
		"qcom,mdss-dsi-off-command", "qcom,mdss-dsi-off-command-state");

/* <DTS2014042409252 d00238048 20140505 begin */
#ifdef CONFIG_FB_AUTO_CABC
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->dsi_panel_cabc_ui_cmds,
		"qcom,panel-cabc-ui-cmds", "qcom,cabc-ui-cmds-dsi-state"); 
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->dsi_panel_cabc_video_cmds,
		"qcom,panel-cabc-video-cmds", "qcom,cabc-video-cmds-dsi-state");
#endif
/* DTS2014042409252 d00238048 20140505 end> */
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->status_cmds,
			"qcom,mdss-dsi-panel-status-command",
				"qcom,mdss-dsi-panel-status-command-state");
	rc = of_property_read_u32(np, "qcom,mdss-dsi-panel-status-value", &tmp);
	ctrl_pdata->status_value = (!rc ? tmp : 0);


	ctrl_pdata->status_mode = ESD_MAX;
	rc = of_property_read_string(np,
				"qcom,mdss-dsi-panel-status-check-mode", &data);
	if (!rc) {
		if (!strcmp(data, "bta_check")) {
			ctrl_pdata->status_mode = ESD_BTA;
		} else if (!strcmp(data, "reg_read")) {
			ctrl_pdata->status_mode = ESD_REG;
			ctrl_pdata->status_cmds_rlen = 0;
			ctrl_pdata->check_read_status =
						mdss_dsi_gen_read_status;
		} else if (!strcmp(data, "reg_read_nt35596")) {
			ctrl_pdata->status_mode = ESD_REG_NT35596;
			ctrl_pdata->status_error_count = 0;
			ctrl_pdata->status_cmds_rlen = 8;
			ctrl_pdata->check_read_status =
						mdss_dsi_nt35596_read_status;
		}
	}

	rc = mdss_dsi_parse_panel_features(np, ctrl_pdata);
	if (rc) {
		pr_err("%s: failed to parse panel features\n", __func__);
		goto error;
	}
/*< DTS2014052207729 renxigang 20140520 begin */
/* < DTS2014051503541 daiyuhong 20140515 begin */
/* <DTS2014051303765 daiyuhong 20140513 begin */
/* <DTS2014050904959 daiyuhong 20140509 begin */
/*< DTS2014042905347 zhaoyuxia 20140429 begin */
/* < DTS2014051403007 zhaoyuxia 20140515 begin */
/* add delaytine-before-bl flag */
#ifdef CONFIG_HUAWEI_LCD
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->dot_inversion_cmds,
		"qcom,panel-dot-inversion-mode-cmds", "qcom,dot-inversion-cmds-dsi-state");
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->column_inversion_cmds,
		"qcom,panel-column-inversion-mode-cmds", "qcom,column-inversion-cmds-dsi-state");
	rc = of_property_read_u32(np, "huawei,long-read-flag", &tmp);
	ctrl_pdata->long_read_flag= (!rc ? tmp : 0);
	rc = of_property_read_u32(np, "huawei,skip-reg-read-flag", &tmp);
	ctrl_pdata->skip_reg_read= (!rc ? tmp : 0);

	rc = of_property_read_u32(np, "huawei,reg-read-expect-value", &tmp);
	ctrl_pdata->reg_expect_value = (!rc ? tmp : 0x1c);

	rc = of_property_read_u32(np, "huawei,reg-long-read-count", &tmp);
	ctrl_pdata->reg_expect_count = (!rc ? tmp : 3);
	rc = of_property_read_u32(np, "huawei,delaytime-before-bl", &tmp);
	pinfo->delaytime_before_bl = (!rc ? tmp : 0);
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->dsi_panel_inverse_on_cmds,
		"qcom,panel-inverse-on-cmds", "qcom,inverse-on-cmds-dsi-state");
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->dsi_panel_inverse_off_cmds,
		"qcom,panel-inverse-off-cmds", "qcom,inverse-on-cmds-dsi-state");
	mdss_panel_parse_esd_dt(np,ctrl_pdata);
#endif
/* DTS2014051403007 zhaoyuxia 20140515 end > */
/* DTS2014042905347 zhaoyuxia 20140429 end > */
/* DTS2014050904959 daiyuhong 20140509 end > */
/* DTS2014051303765 daiyuhong 20140513 end> */
/* DTS2014051503541 daiyuhong 20140515 end > */
/* DTS2014052207729 renxigang 20140520 end >*/
	return 0;

error:
	return -EINVAL;
}

int mdss_dsi_panel_init(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata,
	bool cmd_cfg_cont_splash)
{
	int rc = 0;
	static const char *panel_name;
	struct mdss_panel_info *pinfo;
/* <DTS2014042405897 d00238048 20140425 begin */
	static const char *info_node = "lcd type";
/* DTS2014042405897 d00238048 20140425 end> */

	if (!node || !ctrl_pdata) {
		pr_err("%s: Invalid arguments\n", __func__);
		return -ENODEV;
	}

	pinfo = &ctrl_pdata->panel_data.panel_info;

	pr_debug("%s:%d\n", __func__, __LINE__);
	pinfo->panel_name[0] = '\0';
	panel_name = of_get_property(node, "qcom,mdss-dsi-panel-name", NULL);
	if (!panel_name) {
		pr_info("%s:%d, Panel name not specified\n",
						__func__, __LINE__);
	} else {
		pr_info("%s: Panel Name = %s\n", __func__, panel_name);

/* <DTS2014042409252 d00238048 20140505 begin */
#ifdef CONFIG_HUAWEI_LCD
/* <DTS2014042405897 d00238048 20140425 begin */
	rc = app_info_set(info_node, panel_name);
/* DTS2014042405897 d00238048 20140425 end> */
#endif
/* DTS2014042409252 d00238048 20140505 end> */
		strlcpy(&pinfo->panel_name[0], panel_name, MDSS_MAX_PANEL_LEN);
	}

	rc = mdss_panel_parse_dt(node, ctrl_pdata);
	if (rc) {
		pr_err("%s:%d panel dt parse failed\n", __func__, __LINE__);
		return rc;
	}

	if (!cmd_cfg_cont_splash)
		pinfo->cont_splash_enabled = false;
	pr_info("%s: Continuous splash %s", __func__,
		pinfo->cont_splash_enabled ? "enabled" : "disabled");

	pinfo->dynamic_switch_pending = false;
	pinfo->is_lpm_mode = false;

	ctrl_pdata->on = mdss_dsi_panel_on;
	ctrl_pdata->off = mdss_dsi_panel_off;
	ctrl_pdata->panel_data.set_backlight = mdss_dsi_panel_bl_ctrl;
/* < DTS2014051503541 daiyuhong 20140515 begin */
/*< DTS2014042905347 zhaoyuxia 20140429 begin */
/*< DTS2014042818102 zhengjinzeng 20140428 begin */
#ifdef CONFIG_HUAWEI_LCD
	ctrl_pdata->panel_data.set_inversion_mode = mdss_dsi_panel_inversion_ctrl;
	ctrl_pdata->panel_data.check_panel_status= mdss_dsi_check_panel_status;
	ctrl_pdata->panel_data.lcd_set_display_inversion = mdss_dsi_lcd_set_display_inversion;
	ctrl_pdata->panel_data.check_panel_mipi_crc = mdss_dsi_check_mipi_crc;
#endif
/* DTS2014042818102 zhengjinzeng 20140428 end >*/
/* DTS2014042905347 zhaoyuxia 20140429 end >*/
/* DTS2014051503541 daiyuhong 20140515 end > */
/* <DTS2014042409252 d00238048 20140505 begin */
#ifdef CONFIG_FB_AUTO_CABC
	ctrl_pdata->panel_data.config_cabc = mdss_dsi_panel_cabc_ctrl;
#endif
/* DTS2014042409252 d00238048 20140505 end> */

/* < DTS2014050808504 daiyuhong 20140508 begin */
/*save ctrl_pdata */
#if defined(CONFIG_HUAWEI_KERNEL) && defined(CONFIG_DEBUG_FS)
	lcd_dbg_set_dsi_ctrl_pdata(ctrl_pdata);
#endif
/* DTS2014050808504 daiyuhong 20140508 end > */
	ctrl_pdata->switch_mode = mdss_dsi_panel_switch_mode;

	return 0;
}

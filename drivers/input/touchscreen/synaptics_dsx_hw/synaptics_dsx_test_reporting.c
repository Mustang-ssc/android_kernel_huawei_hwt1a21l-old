/* < DTS2014042402686 sunlibin 20140424 begin */
/*Add synaptics new driver "Synaptics DSX I2C V2.0"*/
/*
 * Synaptics DSX touchscreen driver
 *
 * Copyright (C) 2012 Synaptics Incorporated
 *
 * Copyright (C) 2012 Alexandra Chin <alexandra.chin@tw.synaptics.com>
 * Copyright (C) 2012 Scott Lin <scott.lin@tw.synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/ctype.h>
#include <linux/hrtimer.h>
/* < DTS2013123004404 sunlibin 20131230 begin */
#include <linux/synaptics_dsx.h>
/* DTS2013123004404 sunlibin 20131230 end> */
#include <linux/synaptics_dsx_i2c.h>
/* < DTS2014010309198 sunlibin 20140104 begin */
#include <linux/hw_tp_common.h>
/* DTS2014010309198 sunlibin 20140104 end > */
/* < DTS2014012604495 sunlibin 20140126 begin */
#include <linux/synaptics_cap_limit.h>
/* DTS2014012604495 sunlibin 20140126 end> */

/* < DTS2014080506603  caowei 201400806 begin */
#include "synaptics_dsx_esd.h"
/* DTS2014080506603  caowei 201400806 end >*/
/* < DTS2015012105205  zhoumin wx222300 20150121 begin */
#ifdef CONFIG_HUAWEI_DSM
/* < DTS2014082605600 s00171075 20140830 begin */
#include <linux/dsm_pub.h>
/* DTS2014082605600 s00171075 20140830 end > */
#endif
/* DTS2015012105205  zhoumin wx222300 20150121 end > */
#define WATCHDOG_HRTIMER
#define WATCHDOG_TIMEOUT_S 2
#define FORCE_TIMEOUT_100MS 10
#define STATUS_WORK_INTERVAL 20 /* ms */
/* < DTS2014012604495 sunlibin 20140126 begin */
/*Move to hw_tp_common.h*/
/* DTS2014012604495 sunlibin 20140126 end> */
/*
#define RAW_HEX
#define HUMAN_READABLE
*/
/* < DTS2014042407105 sunlibin 20140504 begin */
#define RAW_HEX
#define MAX_I2C_MSG_LENS 0xFF
#define MAX_IRQ_WAIT_PATIENCE 50
#define STATUS_WORK_TIMEOUT 100 /* ms */
/* DTS2014042407105 sunlibin 20140504 end> */

#define STATUS_IDLE 0
#define STATUS_BUSY 1

#define DATA_REPORT_INDEX_OFFSET 1
#define DATA_REPORT_DATA_OFFSET 3

#define SENSOR_RX_MAPPING_OFFSET 1
#define SENSOR_TX_MAPPING_OFFSET 2

#define COMMAND_GET_REPORT 1
#define COMMAND_FORCE_CAL 2
#define COMMAND_FORCE_UPDATE 4

#define CONTROL_42_SIZE 2
#define CONTROL_43_54_SIZE 13
#define CONTROL_55_56_SIZE 2
#define CONTROL_58_SIZE 1
#define CONTROL_59_SIZE 2
#define CONTROL_60_62_SIZE 3
#define CONTROL_63_SIZE 1
#define CONTROL_64_67_SIZE 4
#define CONTROL_68_73_SIZE 8
#define CONTROL_74_SIZE 2
#define CONTROL_76_SIZE 1
#define CONTROL_77_78_SIZE 2
#define CONTROL_79_83_SIZE 5
#define CONTROL_84_85_SIZE 2
#define CONTROL_86_SIZE 1
#define CONTROL_87_SIZE 1

#define HIGH_RESISTANCE_DATA_SIZE 6
#define FULL_RAW_CAP_MIN_MAX_DATA_SIZE 4
#define TREX_DATA_SIZE 7

#define NO_AUTO_CAL_MASK 0x01

#define concat(a, b) a##b

#define GROUP(_attrs) {\
	.attrs = _attrs,\
}

#define attrify(propname) (&dev_attr_##propname.attr)

#define show_prototype(propname)\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_show)(\
		struct device *dev,\
		struct device_attribute *attr,\
		char *buf);\
\
struct device_attribute dev_attr_##propname =\
		__ATTR(propname, S_IRUGO,\
		concat(synaptics_rmi4_f54, _##propname##_show),\
		synaptics_rmi4_store_error);

#define store_prototype(propname)\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_store)(\
		struct device *dev,\
		struct device_attribute *attr,\
		const char *buf, size_t count);\
\
struct device_attribute dev_attr_##propname =\
		__ATTR(propname, S_IWUSR | S_IWGRP,\
		synaptics_rmi4_show_error,\
		concat(synaptics_rmi4_f54, _##propname##_store));

#define show_store_prototype(propname)\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_show)(\
		struct device *dev,\
		struct device_attribute *attr,\
		char *buf);\
\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_store)(\
		struct device *dev,\
		struct device_attribute *attr,\
		const char *buf, size_t count);\
\
struct device_attribute dev_attr_##propname =\
		__ATTR(propname, (S_IRUGO | S_IWUSR | S_IWGRP),\
		concat(synaptics_rmi4_f54, _##propname##_show),\
		concat(synaptics_rmi4_f54, _##propname##_store));

#define simple_show_func(rtype, propname, fmt)\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_show)(\
		struct device *dev,\
		struct device_attribute *attr,\
		char *buf)\
{\
	return snprintf(buf, PAGE_SIZE, fmt, f54->rtype.propname);\
} \

#define simple_show_func_unsigned(rtype, propname)\
simple_show_func(rtype, propname, "%u\n")

#define show_func(rtype, rgrp, propname, fmt)\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_show)(\
		struct device *dev,\
		struct device_attribute *attr,\
		char *buf)\
{\
	int retval;\
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;\
\
	mutex_lock(&f54->rtype##_mutex);\
\
	retval = f54->fn_ptr->read(rmi4_data,\
			f54->rtype.rgrp->address,\
			f54->rtype.rgrp->data,\
			sizeof(f54->rtype.rgrp->data));\
	mutex_unlock(&f54->rtype##_mutex);\
	if (retval < 0) {\
		dev_err(&rmi4_data->i2c_client->dev,\
				"%s: Failed to read " #rtype\
				" " #rgrp "\n",\
				__func__);\
		return retval;\
	} \
\
	return snprintf(buf, PAGE_SIZE, fmt,\
			f54->rtype.rgrp->propname);\
} \

#define show_store_func(rtype, rgrp, propname, fmt)\
show_func(rtype, rgrp, propname, fmt)\
\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_store)(\
		struct device *dev,\
		struct device_attribute *attr,\
		const char *buf, size_t count)\
{\
	int retval;\
	unsigned long setting;\
	unsigned long o_setting;\
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;\
\
	retval = sstrtoul(buf, 10, &setting);\
	if (retval)\
		return retval;\
\
	mutex_lock(&f54->rtype##_mutex);\
	retval = f54->fn_ptr->read(rmi4_data,\
			f54->rtype.rgrp->address,\
			f54->rtype.rgrp->data,\
			sizeof(f54->rtype.rgrp->data));\
	if (retval < 0) {\
		mutex_unlock(&f54->rtype##_mutex);\
		dev_err(&rmi4_data->i2c_client->dev,\
				"%s: Failed to read " #rtype\
				" " #rgrp "\n",\
				__func__);\
		return retval;\
	} \
\
	if (f54->rtype.rgrp->propname == setting) {\
		mutex_unlock(&f54->rtype##_mutex);\
		return count;\
	} \
\
	o_setting = f54->rtype.rgrp->propname;\
	f54->rtype.rgrp->propname = setting;\
\
	retval = f54->fn_ptr->write(rmi4_data,\
			f54->rtype.rgrp->address,\
			f54->rtype.rgrp->data,\
			sizeof(f54->rtype.rgrp->data));\
	if (retval < 0) {\
		dev_err(&rmi4_data->i2c_client->dev,\
				"%s: Failed to write " #rtype\
				" " #rgrp "\n",\
				__func__);\
		f54->rtype.rgrp->propname = o_setting;\
		mutex_unlock(&f54->rtype##_mutex);\
		return retval;\
	} \
\
	mutex_unlock(&f54->rtype##_mutex);\
	return count;\
} \

#define show_store_func_unsigned(rtype, rgrp, propname)\
show_store_func(rtype, rgrp, propname, "%u\n")

#define show_replicated_func(rtype, rgrp, propname, fmt)\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_show)(\
		struct device *dev,\
		struct device_attribute *attr,\
		char *buf)\
{\
	int retval;\
	int size = 0;\
	unsigned char ii;\
	unsigned char length;\
	unsigned char *temp;\
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;\
\
	mutex_lock(&f54->rtype##_mutex);\
\
	length = f54->rtype.rgrp->length;\
\
	retval = f54->fn_ptr->read(rmi4_data,\
			f54->rtype.rgrp->address,\
			(unsigned char *)f54->rtype.rgrp->data,\
			length);\
	mutex_unlock(&f54->rtype##_mutex);\
	if (retval < 0) {\
		dev_dbg(&rmi4_data->i2c_client->dev,\
				"%s: Failed to read " #rtype\
				" " #rgrp "\n",\
				__func__);\
	} \
\
	temp = buf;\
\
	for (ii = 0; ii < length; ii++) {\
		retval = snprintf(temp, PAGE_SIZE - size, fmt " ",\
				f54->rtype.rgrp->data[ii].propname);\
		if (retval < 0) {\
			dev_err(&rmi4_data->i2c_client->dev,\
					"%s: Faild to write output\n",\
					__func__);\
			return retval;\
		} \
		size += retval;\
		temp += retval;\
	} \
\
	retval = snprintf(temp, PAGE_SIZE - size, "\n");\
	if (retval < 0) {\
		dev_err(&rmi4_data->i2c_client->dev,\
				"%s: Faild to write null terminator\n",\
				__func__);\
		return retval;\
	} \
\
	return size + retval;\
} \

#define show_replicated_func_unsigned(rtype, rgrp, propname)\
show_replicated_func(rtype, rgrp, propname, "%u")

#define show_store_replicated_func(rtype, rgrp, propname, fmt)\
show_replicated_func(rtype, rgrp, propname, fmt)\
\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_store)(\
		struct device *dev,\
		struct device_attribute *attr,\
		const char *buf, size_t count)\
{\
	int retval;\
	unsigned int setting;\
	unsigned char ii;\
	unsigned char length;\
	const unsigned char *temp;\
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;\
\
	mutex_lock(&f54->rtype##_mutex);\
\
	length = f54->rtype.rgrp->length;\
\
	retval = f54->fn_ptr->read(rmi4_data,\
			f54->rtype.rgrp->address,\
			(unsigned char *)f54->rtype.rgrp->data,\
			length);\
	if (retval < 0) {\
		dev_dbg(&rmi4_data->i2c_client->dev,\
				"%s: Failed to read " #rtype\
				" " #rgrp "\n",\
				__func__);\
	} \
\
	temp = buf;\
\
	for (ii = 0; ii < length; ii++) {\
		if (sscanf(temp, fmt, &setting) == 1) {\
			f54->rtype.rgrp->data[ii].propname = setting;\
		} else {\
			retval = f54->fn_ptr->read(rmi4_data,\
					f54->rtype.rgrp->address,\
					(unsigned char *)f54->rtype.rgrp->data,\
					length);\
			mutex_unlock(&f54->rtype##_mutex);\
			return -EINVAL;\
		} \
\
		while (*temp != 0) {\
			temp++;\
			if (isspace(*(temp - 1)) && !isspace(*temp))\
				break;\
		} \
	} \
\
	retval = f54->fn_ptr->write(rmi4_data,\
			f54->rtype.rgrp->address,\
			(unsigned char *)f54->rtype.rgrp->data,\
			length);\
	mutex_unlock(&f54->rtype##_mutex);\
	if (retval < 0) {\
		dev_err(&rmi4_data->i2c_client->dev,\
				"%s: Failed to write " #rtype\
				" " #rgrp "\n",\
				__func__);\
		return retval;\
	} \
\
	return count;\
} \

#define show_store_replicated_func_unsigned(rtype, rgrp, propname)\
show_store_replicated_func(rtype, rgrp, propname, "%u")

enum f54_report_types {
	F54_8BIT_IMAGE = 1,
	F54_16BIT_IMAGE = 2,
	F54_RAW_16BIT_IMAGE = 3,
	F54_HIGH_RESISTANCE = 4,
	F54_TX_TO_TX_SHORT = 5,
	F54_RX_TO_RX1 = 7,
	F54_TRUE_BASELINE = 9,
	F54_FULL_RAW_CAP_MIN_MAX = 13,
	F54_RX_OPENS1 = 14,
	F54_TX_OPEN = 15,
	F54_TX_TO_GROUND = 16,
	F54_RX_TO_RX2 = 17,
	F54_RX_OPENS2 = 18,
	F54_FULL_RAW_CAP = 19,
	F54_FULL_RAW_CAP_RX_COUPLING_COMP = 20,
	F54_SENSOR_SPEED = 22,
	F54_ADC_RANGE = 23,
	F54_TREX_OPENS = 24,
	F54_TREX_TO_GND = 25,
	F54_TREX_SHORTS = 26,
	INVALID_REPORT_TYPE = -1,
};

/* < DTS2013121708848 sunlibin 20131217 begin */
/*Add synaptics capacitor test function */
/* < DTS2014012604495 sunlibin 20140126 begin */
/*Move to synaptics_cap_limit.h */
#ifdef MMITEST
static unsigned char rx = TOUCH_RX_DEFAULT;
static unsigned char tx = TOUCH_TX_DEFAULT;

static bool sysfs_is_busy = false;

enum mmi_results {
	TEST_FAILED,
	TEST_PASS,
};
static char *g_mmi_buf_f54test_result = NULL;
static char *g_mmi_highresistance_report = NULL;
static char *g_mmi_maxmincapacitance_report = NULL;
static char *g_mmi_RxtoRxshort_report = NULL;
static char *g_mmi_buf_f54raw_data = NULL;

/* < DTS2014011405074 sunlibin 20140114 begin */
#define F54_MAX_CAP_DATA_SIZE	6
#define F54_MAX_CAP_TITLE_SIZE	50
#define F54_HIGH_RESISTANCE_DATA_NUM	3
#define F54_RAW_CAP_MIN_MAX_DATA_NUM	2
#define F54_MAX_CAP_RESULT_SIZE	50
/* DTS2014011405074 sunlibin 20140114 end > */

/*<DTS2014031803547 yuxuesong 20140318 begin */
#define F54_FULL_RAW_CAP_MIN_MAX_SIZE (F54_MAX_CAP_DATA_SIZE*F54_RAW_CAP_MIN_MAX_DATA_NUM+F54_MAX_CAP_TITLE_SIZE)

#define F54_FULL_RAW_CAP_RX_COUPLING_COMP_SIZE(x, y) \
	( (F54_MAX_CAP_DATA_SIZE * (x) * (y)) + F54_MAX_CAP_TITLE_SIZE )

#define F54_HIGH_RESISTANCE_SIZE (F54_MAX_CAP_DATA_SIZE*F54_HIGH_RESISTANCE_DATA_NUM+F54_MAX_CAP_TITLE_SIZE)

#define F54_RX_TO_RX1_SIZE(x) (F54_MAX_CAP_DATA_SIZE * (x) \
				+ F54_MAX_CAP_TITLE_SIZE)
/* DTS2014031803547 yuxuesong 20140318 end> */

#endif/*MMITEST*/
/* DTS2014012604495 sunlibin 20140126 end> */
/* DTS2013121708848 sunlibin 20131217 end >*/

struct f54_query {
	union {
		struct {
			/* query 0 */
			unsigned char num_of_rx_electrodes;

			/* query 1 */
			unsigned char num_of_tx_electrodes;

			/* query 2 */
			unsigned char f54_query2_b0__1:2;
			unsigned char has_baseline:1;
			unsigned char has_image8:1;
			unsigned char f54_query2_b4__5:2;
			unsigned char has_image16:1;
			unsigned char f54_query2_b7:1;

			/* queries 3.0 and 3.1 */
			unsigned short clock_rate;

			/* query 4 */
			unsigned char touch_controller_family;

			/* query 5 */
			unsigned char has_pixel_touch_threshold_adjustment:1;
			unsigned char f54_query5_b1__7:7;

			/* query 6 */
			unsigned char has_sensor_assignment:1;
			unsigned char has_interference_metric:1;
			unsigned char has_sense_frequency_control:1;
			unsigned char has_firmware_noise_mitigation:1;
			unsigned char has_ctrl11:1;
			unsigned char has_two_byte_report_rate:1;
			unsigned char has_one_byte_report_rate:1;
			unsigned char has_relaxation_control:1;

			/* query 7 */
			unsigned char curve_compensation_mode:2;
			unsigned char f54_query7_b2__7:6;

			/* query 8 */
			unsigned char f54_query8_b0:1;
			unsigned char has_iir_filter:1;
			unsigned char has_cmn_removal:1;
			unsigned char has_cmn_maximum:1;
			unsigned char has_touch_hysteresis:1;
			unsigned char has_edge_compensation:1;
			unsigned char has_per_frequency_noise_control:1;
			unsigned char has_enhanced_stretch:1;

			/* query 9 */
			unsigned char has_force_fast_relaxation:1;
			unsigned char has_multi_metric_state_machine:1;
			unsigned char has_signal_clarity:1;
			unsigned char has_variance_metric:1;
			unsigned char has_0d_relaxation_control:1;
			unsigned char has_0d_acquisition_control:1;
			unsigned char has_status:1;
			unsigned char has_slew_metric:1;

			/* query 10 */
			unsigned char has_h_blank:1;
			unsigned char has_v_blank:1;
			unsigned char has_long_h_blank:1;
			unsigned char has_startup_fast_relaxation:1;
			unsigned char has_esd_control:1;
			unsigned char has_noise_mitigation2:1;
			unsigned char has_noise_state:1;
			unsigned char has_energy_ratio_relaxation:1;

			/* query 11 */
			unsigned char has_excessive_noise_reporting:1;
			unsigned char has_slew_option:1;
			unsigned char has_two_overhead_bursts:1;
			unsigned char has_query13:1;
			unsigned char has_one_overhead_burst:1;
			unsigned char f54_query11_b5:1;
			unsigned char has_ctrl88:1;
			unsigned char has_query15:1;

			/* query 12 */
			unsigned char number_of_sensing_frequencies:4;
			unsigned char f54_query12_b4__7:4;

			/* query 13 */
			unsigned char has_ctrl86:1;
			unsigned char has_ctrl87:1;
			unsigned char has_ctrl87_sub0:1;
			unsigned char has_ctrl87_sub1:1;
			unsigned char has_ctrl87_sub2:1;
			unsigned char has_cidim:1;
			unsigned char has_noise_mitigation_enhancement:1;
			unsigned char has_rail_im:1;
		} __packed;
		unsigned char data[15];
	};
};

struct f54_control_0 {
	union {
		struct {
			unsigned char no_relax:1;
			unsigned char no_scan:1;
			unsigned char force_fast_relaxation:1;
			unsigned char startup_fast_relaxation:1;
			unsigned char gesture_cancels_sfr:1;
			unsigned char enable_energy_ratio_relaxation:1;
			unsigned char excessive_noise_attn_enable:1;
			unsigned char f54_control0_b7:1;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_1 {
	union {
		struct {
			unsigned char bursts_per_cluster:4;
			unsigned char f54_ctrl1_b4__7:4;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_2 {
	union {
		struct {
			unsigned short saturation_cap;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_3 {
	union {
		struct {
			unsigned char pixel_touch_threshold;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_4__6 {
	union {
		struct {
			/* control 4 */
			unsigned char rx_feedback_cap:2;
			unsigned char bias_current:2;
			unsigned char f54_ctrl4_b4__7:4;

			/* control 5 */
			unsigned char low_ref_cap:2;
			unsigned char low_ref_feedback_cap:2;
			unsigned char low_ref_polarity:1;
			unsigned char f54_ctrl5_b5__7:3;

			/* control 6 */
			unsigned char high_ref_cap:2;
			unsigned char high_ref_feedback_cap:2;
			unsigned char high_ref_polarity:1;
			unsigned char f54_ctrl6_b5__7:3;
		} __packed;
		struct {
			unsigned char data[3];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_7 {
	union {
		struct {
			unsigned char cbc_cap:3;
			unsigned char cbc_polarity:1;
			unsigned char cbc_tx_carrier_selection:1;
			unsigned char f54_ctrl7_b5__7:3;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_8__9 {
	union {
		struct {
			/* control 8 */
			unsigned short integration_duration:10;
			unsigned short f54_ctrl8_b10__15:6;

			/* control 9 */
			unsigned char reset_duration;
		} __packed;
		struct {
			unsigned char data[3];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_10 {
	union {
		struct {
			unsigned char noise_sensing_bursts_per_image:4;
			unsigned char f54_ctrl10_b4__7:4;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_11 {
	union {
		struct {
			unsigned short f54_ctrl11;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_12__13 {
	union {
		struct {
			/* control 12 */
			unsigned char slow_relaxation_rate;

			/* control 13 */
			unsigned char fast_relaxation_rate;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_14 {
	union {
		struct {
			unsigned char rxs_on_xaxis:1;
			unsigned char curve_comp_on_txs:1;
			unsigned char f54_ctrl14_b2__7:6;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_15n {
	unsigned char sensor_rx_assignment;
};

struct f54_control_15 {
	struct f54_control_15n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_16n {
	unsigned char sensor_tx_assignment;
};

struct f54_control_16 {
	struct f54_control_16n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_17n {
	unsigned char burst_count_b8__10:3;
	unsigned char disable:1;
	unsigned char f54_ctrl17_b4:1;
	unsigned char filter_bandwidth:3;
};

struct f54_control_17 {
	struct f54_control_17n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_18n {
	unsigned char burst_count_b0__7;
};

struct f54_control_18 {
	struct f54_control_18n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_19n {
	unsigned char stretch_duration;
};

struct f54_control_19 {
	struct f54_control_19n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_20 {
	union {
		struct {
			unsigned char disable_noise_mitigation:1;
			unsigned char f54_ctrl20_b1__7:7;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_21 {
	union {
		struct {
			unsigned short freq_shift_noise_threshold;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_22__26 {
	union {
		struct {
			/* control 22 */
			unsigned char f54_ctrl22;

			/* control 23 */
			unsigned short medium_noise_threshold;

			/* control 24 */
			unsigned short high_noise_threshold;

			/* control 25 */
			unsigned char noise_density;

			/* control 26 */
			unsigned char frame_count;
		} __packed;
		struct {
			unsigned char data[7];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_27 {
	union {
		struct {
			unsigned char iir_filter_coef;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_28 {
	union {
		struct {
			unsigned short quiet_threshold;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_29 {
	union {
		struct {
			/* control 29 */
			unsigned char f54_ctrl29_b0__6:7;
			unsigned char cmn_filter_disable:1;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_30 {
	union {
		struct {
			unsigned char cmn_filter_max;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_31 {
	union {
		struct {
			unsigned char touch_hysteresis;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_32__35 {
	union {
		struct {
			/* control 32 */
			unsigned short rx_low_edge_comp;

			/* control 33 */
			unsigned short rx_high_edge_comp;

			/* control 34 */
			unsigned short tx_low_edge_comp;

			/* control 35 */
			unsigned short tx_high_edge_comp;
		} __packed;
		struct {
			unsigned char data[8];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_36n {
	unsigned char axis1_comp;
};

struct f54_control_36 {
	struct f54_control_36n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_37n {
	unsigned char axis2_comp;
};

struct f54_control_37 {
	struct f54_control_37n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_38n {
	unsigned char noise_control_1;
};

struct f54_control_38 {
	struct f54_control_38n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_39n {
	unsigned char noise_control_2;
};

struct f54_control_39 {
	struct f54_control_39n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_40n {
	unsigned char noise_control_3;
};

struct f54_control_40 {
	struct f54_control_40n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_41 {
	union {
		struct {
			unsigned char no_signal_clarity:1;
			unsigned char f54_ctrl41_b1__7:7;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_57 {
	union {
		struct {
			unsigned char cbc_cap_0d:3;
			unsigned char cbc_polarity_0d:1;
			unsigned char cbc_tx_carrier_selection_0d:1;
			unsigned char f54_ctrl57_b5__7:3;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_88 {
	union {
		struct {
			unsigned char tx_low_reference_polarity:1;
			unsigned char tx_high_reference_polarity:1;
			unsigned char abs_low_reference_polarity:1;
			unsigned char abs_polarity:1;
			unsigned char cbc_polarity:1;
			unsigned char cbc_tx_carrier_selection:1;
			unsigned char charge_pump_enable:1;
			unsigned char cbc_abs_auto_servo:1;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control {
	struct f54_control_0 *reg_0;
	struct f54_control_1 *reg_1;
	struct f54_control_2 *reg_2;
	struct f54_control_3 *reg_3;
	struct f54_control_4__6 *reg_4__6;
	struct f54_control_7 *reg_7;
	struct f54_control_8__9 *reg_8__9;
	struct f54_control_10 *reg_10;
	struct f54_control_11 *reg_11;
	struct f54_control_12__13 *reg_12__13;
	struct f54_control_14 *reg_14;
	struct f54_control_15 *reg_15;
	struct f54_control_16 *reg_16;
	struct f54_control_17 *reg_17;
	struct f54_control_18 *reg_18;
	struct f54_control_19 *reg_19;
	struct f54_control_20 *reg_20;
	struct f54_control_21 *reg_21;
	struct f54_control_22__26 *reg_22__26;
	struct f54_control_27 *reg_27;
	struct f54_control_28 *reg_28;
	struct f54_control_29 *reg_29;
	struct f54_control_30 *reg_30;
	struct f54_control_31 *reg_31;
	struct f54_control_32__35 *reg_32__35;
	struct f54_control_36 *reg_36;
	struct f54_control_37 *reg_37;
	struct f54_control_38 *reg_38;
	struct f54_control_39 *reg_39;
	struct f54_control_40 *reg_40;
	struct f54_control_41 *reg_41;
	struct f54_control_57 *reg_57;
	struct f54_control_88 *reg_88;
};

struct synaptics_rmi4_f54_handle {
	bool no_auto_cal;
	unsigned char status;
	unsigned char intr_mask;
	unsigned char intr_reg_num;
	unsigned char rx_assigned;
	unsigned char tx_assigned;
	unsigned char *report_data;
	unsigned short query_base_addr;
	unsigned short control_base_addr;
	unsigned short data_base_addr;
	unsigned short command_base_addr;
	unsigned short fifoindex;
	unsigned int report_size;
	unsigned int data_buffer_size;
	enum f54_report_types report_type;
	struct mutex status_mutex;
	struct mutex data_mutex;
	struct mutex control_mutex;
	struct f54_query query;
	struct f54_control control;
	struct kobject *attr_dir;
	struct hrtimer watchdog;
	struct work_struct timeout_work;
	struct delayed_work status_work;
	struct workqueue_struct *status_workqueue;
	struct synaptics_rmi4_access_ptr *fn_ptr;
	struct synaptics_rmi4_data *rmi4_data;
/* < DTS2014012604495 sunlibin 20140126 begin */
	short f54_raw_max_cap;
	short f54_raw_min_cap;
	short *f54_full_raw_max_cap;
	short *f54_full_raw_min_cap;
	short *f54_highRes_max;
	short *f54_highRes_min;
	int f54_rx2rx_diagonal_max;
	int f54_rx2rx_diagonal_min;
	int f54_rx2rx_others;
	char f54_tx2tx_limit;
/* DTS2014012604495 sunlibin 20140126 end> */
/* < DTS2014033108554 songrongyuan 20140408 begin */
	int rx2rx_tmp_count;
/* DTS2014033108554 songrongyuan 20140408 end > */
};

struct f55_query {
	union {
		struct {
			/* query 0 */
			unsigned char num_of_rx_electrodes;

			/* query 1 */
			unsigned char num_of_tx_electrodes;

			/* query 2 */
			unsigned char has_sensor_assignment:1;
			unsigned char has_edge_compensation:1;
			unsigned char curve_compensation_mode:2;
			unsigned char has_ctrl6:1;
			unsigned char has_alternate_transmitter_assignment:1;
			unsigned char has_single_layer_multi_touch:1;
			unsigned char has_query5:1;
		} __packed;
		unsigned char data[3];
	};
};

struct synaptics_rmi4_f55_handle {
	unsigned char *rx_assignment;
	unsigned char *tx_assignment;
	unsigned short query_base_addr;
	unsigned short control_base_addr;
	unsigned short data_base_addr;
	unsigned short command_base_addr;
	struct f55_query query;
};

show_prototype(status)
show_prototype(report_size)
show_store_prototype(no_auto_cal)
show_store_prototype(report_type)
show_store_prototype(fifoindex)
store_prototype(do_preparation)
store_prototype(get_report)
store_prototype(force_cal)
store_prototype(resume_touch)
show_prototype(num_of_mapped_rx)
show_prototype(num_of_mapped_tx)
show_prototype(num_of_rx_electrodes)
show_prototype(num_of_tx_electrodes)
show_prototype(has_image16)
show_prototype(has_image8)
show_prototype(has_baseline)
show_prototype(clock_rate)
show_prototype(touch_controller_family)
show_prototype(has_pixel_touch_threshold_adjustment)
show_prototype(has_sensor_assignment)
show_prototype(has_interference_metric)
show_prototype(has_sense_frequency_control)
show_prototype(has_firmware_noise_mitigation)
show_prototype(has_two_byte_report_rate)
show_prototype(has_one_byte_report_rate)
show_prototype(has_relaxation_control)
show_prototype(curve_compensation_mode)
show_prototype(has_iir_filter)
show_prototype(has_cmn_removal)
show_prototype(has_cmn_maximum)
show_prototype(has_touch_hysteresis)
show_prototype(has_edge_compensation)
show_prototype(has_per_frequency_noise_control)
show_prototype(has_signal_clarity)
show_prototype(number_of_sensing_frequencies)

show_store_prototype(no_relax)
show_store_prototype(no_scan)
show_store_prototype(bursts_per_cluster)
show_store_prototype(saturation_cap)
show_store_prototype(pixel_touch_threshold)
show_store_prototype(rx_feedback_cap)
show_store_prototype(low_ref_cap)
show_store_prototype(low_ref_feedback_cap)
show_store_prototype(low_ref_polarity)
show_store_prototype(high_ref_cap)
show_store_prototype(high_ref_feedback_cap)
show_store_prototype(high_ref_polarity)
show_store_prototype(cbc_cap)
show_store_prototype(cbc_polarity)
show_store_prototype(cbc_tx_carrier_selection)
show_store_prototype(integration_duration)
show_store_prototype(reset_duration)
show_store_prototype(noise_sensing_bursts_per_image)
show_store_prototype(slow_relaxation_rate)
show_store_prototype(fast_relaxation_rate)
show_store_prototype(rxs_on_xaxis)
show_store_prototype(curve_comp_on_txs)
show_prototype(sensor_rx_assignment)
show_prototype(sensor_tx_assignment)
show_prototype(burst_count)
show_prototype(disable)
show_prototype(filter_bandwidth)
show_prototype(stretch_duration)
show_store_prototype(disable_noise_mitigation)
show_store_prototype(freq_shift_noise_threshold)
show_store_prototype(medium_noise_threshold)
show_store_prototype(high_noise_threshold)
show_store_prototype(noise_density)
show_store_prototype(frame_count)
show_store_prototype(iir_filter_coef)
show_store_prototype(quiet_threshold)
show_store_prototype(cmn_filter_disable)
show_store_prototype(cmn_filter_max)
show_store_prototype(touch_hysteresis)
show_store_prototype(rx_low_edge_comp)
show_store_prototype(rx_high_edge_comp)
show_store_prototype(tx_low_edge_comp)
show_store_prototype(tx_high_edge_comp)
show_store_prototype(axis1_comp)
show_store_prototype(axis2_comp)
show_prototype(noise_control_1)
show_prototype(noise_control_2)
show_prototype(noise_control_3)
show_store_prototype(no_signal_clarity)
show_store_prototype(cbc_cap_0d)
show_store_prototype(cbc_polarity_0d)
show_store_prototype(cbc_tx_carrier_selection_0d)
/* < DTS2013121708848 sunlibin 20131217 begin */
/*Add synaptics capacitor test function */
#ifdef MMITEST
store_prototype(mmi_test)
show_prototype(mmi_test_result)
#endif
/* DTS2013121708848 sunlibin 20131217 end >*/

static ssize_t synaptics_rmi4_f54_data_read(struct file *data_file,
		struct kobject *kobj, struct bin_attribute *attributes,
		char *buf, loff_t pos, size_t count);

static struct attribute *attrs[] = {
	attrify(status),
	attrify(report_size),
	attrify(no_auto_cal),
	attrify(report_type),
	attrify(fifoindex),
	attrify(do_preparation),
	attrify(get_report),
	attrify(force_cal),
	attrify(resume_touch),
	attrify(num_of_mapped_rx),
	attrify(num_of_mapped_tx),
	attrify(num_of_rx_electrodes),
	attrify(num_of_tx_electrodes),
	attrify(has_image16),
	attrify(has_image8),
	attrify(has_baseline),
	attrify(clock_rate),
	attrify(touch_controller_family),
	attrify(has_pixel_touch_threshold_adjustment),
	attrify(has_sensor_assignment),
	attrify(has_interference_metric),
	attrify(has_sense_frequency_control),
	attrify(has_firmware_noise_mitigation),
	attrify(has_two_byte_report_rate),
	attrify(has_one_byte_report_rate),
	attrify(has_relaxation_control),
	attrify(curve_compensation_mode),
	attrify(has_iir_filter),
	attrify(has_cmn_removal),
	attrify(has_cmn_maximum),
	attrify(has_touch_hysteresis),
	attrify(has_edge_compensation),
	attrify(has_per_frequency_noise_control),
	attrify(has_signal_clarity),
	attrify(number_of_sensing_frequencies),
/* < DTS2013121708848 sunlibin 20131217 begin */
/*Add synaptics capacitor test function */
#ifdef MMITEST
	attrify(mmi_test),
	attrify(mmi_test_result),
#endif
/* DTS2013121708848 sunlibin 20131217 end >*/
	NULL,
};

static struct attribute_group attr_group = GROUP(attrs);

static struct attribute *attrs_reg_0[] = {
	attrify(no_relax),
	attrify(no_scan),
	NULL,
};

static struct attribute *attrs_reg_1[] = {
	attrify(bursts_per_cluster),
	NULL,
};

static struct attribute *attrs_reg_2[] = {
	attrify(saturation_cap),
	NULL,
};

static struct attribute *attrs_reg_3[] = {
	attrify(pixel_touch_threshold),
	NULL,
};

static struct attribute *attrs_reg_4__6[] = {
	attrify(rx_feedback_cap),
	attrify(low_ref_cap),
	attrify(low_ref_feedback_cap),
	attrify(low_ref_polarity),
	attrify(high_ref_cap),
	attrify(high_ref_feedback_cap),
	attrify(high_ref_polarity),
	NULL,
};

static struct attribute *attrs_reg_7[] = {
	attrify(cbc_cap),
	attrify(cbc_polarity),
	attrify(cbc_tx_carrier_selection),
	NULL,
};

static struct attribute *attrs_reg_8__9[] = {
	attrify(integration_duration),
	attrify(reset_duration),
	NULL,
};

static struct attribute *attrs_reg_10[] = {
	attrify(noise_sensing_bursts_per_image),
	NULL,
};

static struct attribute *attrs_reg_11[] = {
	NULL,
};

static struct attribute *attrs_reg_12__13[] = {
	attrify(slow_relaxation_rate),
	attrify(fast_relaxation_rate),
	NULL,
};

static struct attribute *attrs_reg_14__16[] = {
	attrify(rxs_on_xaxis),
	attrify(curve_comp_on_txs),
	attrify(sensor_rx_assignment),
	attrify(sensor_tx_assignment),
	NULL,
};

static struct attribute *attrs_reg_17__19[] = {
	attrify(burst_count),
	attrify(disable),
	attrify(filter_bandwidth),
	attrify(stretch_duration),
	NULL,
};

static struct attribute *attrs_reg_20[] = {
	attrify(disable_noise_mitigation),
	NULL,
};

static struct attribute *attrs_reg_21[] = {
	attrify(freq_shift_noise_threshold),
	NULL,
};

static struct attribute *attrs_reg_22__26[] = {
	attrify(medium_noise_threshold),
	attrify(high_noise_threshold),
	attrify(noise_density),
	attrify(frame_count),
	NULL,
};

static struct attribute *attrs_reg_27[] = {
	attrify(iir_filter_coef),
	NULL,
};

static struct attribute *attrs_reg_28[] = {
	attrify(quiet_threshold),
	NULL,
};

static struct attribute *attrs_reg_29[] = {
	attrify(cmn_filter_disable),
	NULL,
};

static struct attribute *attrs_reg_30[] = {
	attrify(cmn_filter_max),
	NULL,
};

static struct attribute *attrs_reg_31[] = {
	attrify(touch_hysteresis),
	NULL,
};

static struct attribute *attrs_reg_32__35[] = {
	attrify(rx_low_edge_comp),
	attrify(rx_high_edge_comp),
	attrify(tx_low_edge_comp),
	attrify(tx_high_edge_comp),
	NULL,
};

static struct attribute *attrs_reg_36[] = {
	attrify(axis1_comp),
	NULL,
};

static struct attribute *attrs_reg_37[] = {
	attrify(axis2_comp),
	NULL,
};

static struct attribute *attrs_reg_38__40[] = {
	attrify(noise_control_1),
	attrify(noise_control_2),
	attrify(noise_control_3),
	NULL,
};

static struct attribute *attrs_reg_41[] = {
	attrify(no_signal_clarity),
	NULL,
};

static struct attribute *attrs_reg_57[] = {
	attrify(cbc_cap_0d),
	attrify(cbc_polarity_0d),
	attrify(cbc_tx_carrier_selection_0d),
	NULL,
};

static struct attribute_group attrs_ctrl_regs[] = {
	GROUP(attrs_reg_0),
	GROUP(attrs_reg_1),
	GROUP(attrs_reg_2),
	GROUP(attrs_reg_3),
	GROUP(attrs_reg_4__6),
	GROUP(attrs_reg_7),
	GROUP(attrs_reg_8__9),
	GROUP(attrs_reg_10),
	GROUP(attrs_reg_11),
	GROUP(attrs_reg_12__13),
	GROUP(attrs_reg_14__16),
	GROUP(attrs_reg_17__19),
	GROUP(attrs_reg_20),
	GROUP(attrs_reg_21),
	GROUP(attrs_reg_22__26),
	GROUP(attrs_reg_27),
	GROUP(attrs_reg_28),
	GROUP(attrs_reg_29),
	GROUP(attrs_reg_30),
	GROUP(attrs_reg_31),
	GROUP(attrs_reg_32__35),
	GROUP(attrs_reg_36),
	GROUP(attrs_reg_37),
	GROUP(attrs_reg_38__40),
	GROUP(attrs_reg_41),
	GROUP(attrs_reg_57),
};

static bool attrs_ctrl_regs_exist[ARRAY_SIZE(attrs_ctrl_regs)];

static struct bin_attribute dev_report_data = {
	.attr = {
		.name = "report_data",
		.mode = S_IRUGO,
	},
	.size = 0,
	.read = synaptics_rmi4_f54_data_read,
};

static struct synaptics_rmi4_f54_handle *f54;
static struct synaptics_rmi4_f55_handle *f55;

DECLARE_COMPLETION(f54_remove_complete);

/* < DTS2014082605600 s00171075 20140830 begin */
static struct synaptics_rmi4_fn_desc *g_rmi_fd = NULL;
/* < DTS2015012105205  zhoumin wx222300 20150121 begin */
#ifdef CONFIG_HUAWEI_DSM
ssize_t synaptics_dsm_f54_pdt_err_info( int err_numb )
{

	ssize_t size = 0;
	ssize_t total_size = 0;
	struct dsm_client *tp_dclient = tp_dsm_get_client();

	/* F54 read pdt err number */
	size =dsm_client_record(tp_dclient, "F54 read pdt err number:%d\n", err_numb );
	total_size += size;
	
	/* F54 record pdt err info */
	if(NULL != g_rmi_fd)
	{
		size = dsm_client_record(tp_dclient, 
					"struct synaptics_rmi4_fn_desc{\n"
					" query_base_addr       :%d\n"
					" cmd_base_addr         :%d\n"
					" ctrl_base_addr        :%d\n"
					" data_base_addr        :%d\n"
					" intr_src_count(3)     :%d\n"
					" fn_number             :%d\n"
					"}\n",
					g_rmi_fd->query_base_addr,
					g_rmi_fd->cmd_base_addr,
					g_rmi_fd->ctrl_base_addr,
					g_rmi_fd->data_base_addr,
					g_rmi_fd->intr_src_count,
					g_rmi_fd->fn_number);
		total_size += size;
	}

	return total_size;
}
#endif
/* DTS2015012105205  zhoumin wx222300 20150121 end > */
/* DTS2014082605600 s00171075 20140830 end > */

static bool is_report_type_valid(enum f54_report_types report_type)
{
	switch (report_type) {
	case F54_8BIT_IMAGE:
	case F54_16BIT_IMAGE:
	case F54_RAW_16BIT_IMAGE:
	case F54_HIGH_RESISTANCE:
	case F54_TX_TO_TX_SHORT:
	case F54_RX_TO_RX1:
	case F54_TRUE_BASELINE:
	case F54_FULL_RAW_CAP_MIN_MAX:
	case F54_RX_OPENS1:
	case F54_TX_OPEN:
	case F54_TX_TO_GROUND:
	case F54_RX_TO_RX2:
	case F54_RX_OPENS2:
	case F54_FULL_RAW_CAP:
	case F54_FULL_RAW_CAP_RX_COUPLING_COMP:
	case F54_SENSOR_SPEED:
	case F54_ADC_RANGE:
	case F54_TREX_OPENS:
	case F54_TREX_TO_GND:
	case F54_TREX_SHORTS:
		return true;
		break;
	default:
		f54->report_type = INVALID_REPORT_TYPE;
		f54->report_size = 0;
		return false;
	}
}

static void set_report_size(void)
{
	int retval;
/* < DTS2014012604495 sunlibin 20140126 begin */
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	rx = f54->rx_assigned;
	tx = f54->tx_assigned;
/* DTS2014012604495 sunlibin 20140126 end> */

	/*<DTS2014031803547 yuxuesong 20140318 begin */
	/* Pick from DTS2014021804310 */
	/* < DTS2014042407105 sunlibin 20140504 begin */
	/* BEGIN PN: DTS2014101407779,Modified by yuanbo, 2010/10/14*/
	tp_log_info("%s: reportType=%d, f54->rx_assigned=%d,f54->tx_assigned=%d\n", __func__, f54->report_type,f54->rx_assigned,f54->tx_assigned);
	/* END   PN: DTS2014101407779,Modified by yuanbo, 2010/10/14*/
	/* DTS2014042407105 sunlibin 20140504 end> */
	/* DTS2014031803547 yuxuesong 20140318 end> */
	switch (f54->report_type) {
	case F54_8BIT_IMAGE:
		f54->report_size = rx * tx;
		break;
	case F54_16BIT_IMAGE:
	case F54_RAW_16BIT_IMAGE:
	case F54_TRUE_BASELINE:
	case F54_FULL_RAW_CAP:
	case F54_FULL_RAW_CAP_RX_COUPLING_COMP:
	case F54_SENSOR_SPEED:
		f54->report_size = 2 * rx * tx;
		break;
	case F54_HIGH_RESISTANCE:
		f54->report_size = HIGH_RESISTANCE_DATA_SIZE;
		break;
	case F54_TX_TO_TX_SHORT:
	case F54_TX_OPEN:
	case F54_TX_TO_GROUND:
/* < DTS2014010609052 sunlibin 20140106 begin */
/*Add for g630*/
		f54->report_size = 3;//(tx + 7) / 8;
/* DTS2014010609052 sunlibin 20140106 end > */
		break;
	case F54_RX_TO_RX1:
	case F54_RX_OPENS1:
		if (rx < tx)
			f54->report_size = 2 * rx * rx;
		else
			f54->report_size = 2 * rx * tx;
		break;
	case F54_FULL_RAW_CAP_MIN_MAX:
		f54->report_size = FULL_RAW_CAP_MIN_MAX_DATA_SIZE;
		break;
	case F54_RX_TO_RX2:
	case F54_RX_OPENS2:
		if (rx <= tx)
			f54->report_size = 0;
		else
			f54->report_size = 2 * rx * (rx - tx);
		break;
	case F54_ADC_RANGE:
		if (f54->query.has_signal_clarity) {
			mutex_lock(&f54->control_mutex);
			retval = f54->fn_ptr->read(rmi4_data,
					f54->control.reg_41->address,
					f54->control.reg_41->data,
					sizeof(f54->control.reg_41->data));
			mutex_unlock(&f54->control_mutex);
			if (retval < 0) {
				dev_dbg(&rmi4_data->i2c_client->dev,
						"%s: Failed to read control reg_41\n",
						__func__);
				f54->report_size = 0;
				break;
			}
			if (!f54->control.reg_41->no_signal_clarity) {
				if (tx % 4)
					tx += 4 - (tx % 4);
			}
		}
		f54->report_size = 2 * rx * tx;
		break;
	case F54_TREX_OPENS:
	case F54_TREX_TO_GND:
	case F54_TREX_SHORTS:
		f54->report_size = TREX_DATA_SIZE;
		break;
	default:
		f54->report_size = 0;
	}
	/* BEGIN PN: DTS2014101407779,Added by yuanbo, 2010/10/14*/
	tp_log_info("%s: f54->report_size=%d \n", __func__, f54->report_size);
	/* END   PN: DTS2014101407779,Added by yuanbo, 2010/10/14*/

	return;
}

static int set_interrupt(bool set)
{
	int retval;
	unsigned char ii;
	unsigned char zero = 0x00;
	unsigned char *intr_mask;
	unsigned short f01_ctrl_reg;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	intr_mask = rmi4_data->intr_mask;
	f01_ctrl_reg = rmi4_data->f01_ctrl_base_addr + 1 + f54->intr_reg_num;

	if (!set) {
		retval = f54->fn_ptr->write(rmi4_data,
				f01_ctrl_reg,
				&zero,
				sizeof(zero));
		if (retval < 0)
			return retval;
	}

	for (ii = 0; ii < rmi4_data->num_of_intr_regs; ii++) {
		if (intr_mask[ii] != 0x00) {
			f01_ctrl_reg = rmi4_data->f01_ctrl_base_addr + 1 + ii;
			if (set) {
				retval = f54->fn_ptr->write(rmi4_data,
						f01_ctrl_reg,
						&zero,
						sizeof(zero));
				if (retval < 0)
					return retval;
			} else {
				retval = f54->fn_ptr->write(rmi4_data,
						f01_ctrl_reg,
						&(intr_mask[ii]),
						sizeof(intr_mask[ii]));
				if (retval < 0)
					return retval;
			}
		}
	}

	f01_ctrl_reg = rmi4_data->f01_ctrl_base_addr + 1 + f54->intr_reg_num;

	if (set) {
		retval = f54->fn_ptr->write(rmi4_data,
				f01_ctrl_reg,
				&f54->intr_mask,
				1);
		if (retval < 0)
			return retval;
	}

	return 0;
}

static int do_preparation(void)
{
	int retval;
	unsigned char value;
	unsigned char command;
	unsigned char timeout_count;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	mutex_lock(&f54->control_mutex);

	if (f54->query.touch_controller_family == 1) {
		value = 0;
		retval = f54->fn_ptr->write(rmi4_data,
				f54->control.reg_7->address,
				&value,
				sizeof(f54->control.reg_7->data));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to disable CBC\n",
					__func__);
			mutex_unlock(&f54->control_mutex);
			return retval;
		}
	} else if (f54->query.has_ctrl88 == 1) {
		retval = f54->fn_ptr->read(rmi4_data,
				f54->control.reg_88->address,
				f54->control.reg_88->data,
				sizeof(f54->control.reg_88->data));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to disable CBC (read ctrl88)\n",
					__func__);
			mutex_unlock(&f54->control_mutex);
			return retval;
		}
		f54->control.reg_88->cbc_polarity = 0;
		f54->control.reg_88->cbc_tx_carrier_selection = 0;
		retval = f54->fn_ptr->write(rmi4_data,
				f54->control.reg_88->address,
				f54->control.reg_88->data,
				sizeof(f54->control.reg_88->data));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to disable CBC (write ctrl88)\n",
					__func__);
			mutex_unlock(&f54->control_mutex);
			return retval;
		}
	}

	if (f54->query.has_0d_acquisition_control) {
		value = 0;
		retval = f54->fn_ptr->write(rmi4_data,
				f54->control.reg_57->address,
				&value,
				sizeof(f54->control.reg_57->data));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to disable 0D CBC\n",
					__func__);
			mutex_unlock(&f54->control_mutex);
			return retval;
		}
	}

	if (f54->query.has_signal_clarity) {
		value = 1;
		retval = f54->fn_ptr->write(rmi4_data,
				f54->control.reg_41->address,
				&value,
				sizeof(f54->control.reg_41->data));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to disable signal clarity\n",
					__func__);
			mutex_unlock(&f54->control_mutex);
			return retval;
		}
	}

	mutex_unlock(&f54->control_mutex);

	command = (unsigned char)COMMAND_FORCE_UPDATE;

	retval = f54->fn_ptr->write(rmi4_data,
			f54->command_base_addr,
			&command,
			sizeof(command));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to write force update command\n",
				__func__);
		return retval;
	}

	timeout_count = 0;
	do {
		retval = f54->fn_ptr->read(rmi4_data,
				f54->command_base_addr,
				&value,
				sizeof(value));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to read command register\n",
					__func__);
			return retval;
		}

		if (value == 0x00)
			break;

		msleep(100);
		timeout_count++;
	} while (timeout_count < FORCE_TIMEOUT_100MS);

	if (timeout_count == FORCE_TIMEOUT_100MS) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Timed out waiting for force update\n",
				__func__);
		return -ETIMEDOUT;
	}

	command = (unsigned char)COMMAND_FORCE_CAL;

	retval = f54->fn_ptr->write(rmi4_data,
			f54->command_base_addr,
			&command,
			sizeof(command));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to write force cal command\n",
				__func__);
		return retval;
	}

	timeout_count = 0;
	do {
		retval = f54->fn_ptr->read(rmi4_data,
				f54->command_base_addr,
				&value,
				sizeof(value));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to read command register\n",
					__func__);
			return retval;
		}

		if (value == 0x00)
			break;

		msleep(100);
		timeout_count++;
	} while (timeout_count < FORCE_TIMEOUT_100MS);

	if (timeout_count == FORCE_TIMEOUT_100MS) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Timed out waiting for force cal\n",
				__func__);
		return -ETIMEDOUT;
	}

	return 0;
}

#ifdef WATCHDOG_HRTIMER
static void timeout_set_status(struct work_struct *work)
{
	int retval;
	unsigned char command;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	tp_log_debug("%s:in!\n", __func__);
	mutex_lock(&f54->status_mutex);
	if (f54->status == STATUS_BUSY) {
		retval = f54->fn_ptr->read(rmi4_data,
				f54->command_base_addr,
				&command,
				sizeof(command));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to read command register\n",
					__func__);
		} else if (command & COMMAND_GET_REPORT) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Report type not supported by FW\n",
					__func__);
		} else {
			queue_delayed_work(f54->status_workqueue,
					&f54->status_work,
					0);
			mutex_unlock(&f54->status_mutex);
			return;
		}
		f54->report_type = INVALID_REPORT_TYPE;
		f54->report_size = 0;
	}
	mutex_unlock(&f54->status_mutex);

	return;
}

static enum hrtimer_restart get_report_timeout(struct hrtimer *timer)
{
	tp_log_debug("%s:in!\n", __func__);
	schedule_work(&(f54->timeout_work));

	return HRTIMER_NORESTART;
}
#endif

#ifdef RAW_HEX
static void print_raw_hex_report(void)
{
	unsigned int ii;

	/* <DTS2014071505823 wwx203500 20142407 begin */
	tp_log_info("%s: Report data (raw hex)\n", __func__);
	/* DTS2014071505823 wwx203500 20142407 end> */

	switch (f54->report_type) {
	case F54_16BIT_IMAGE:
	case F54_RAW_16BIT_IMAGE:
	case F54_HIGH_RESISTANCE:
	case F54_TRUE_BASELINE:
	case F54_FULL_RAW_CAP_MIN_MAX:
	case F54_FULL_RAW_CAP:
	case F54_FULL_RAW_CAP_RX_COUPLING_COMP:
	case F54_SENSOR_SPEED:
	case F54_ADC_RANGE:
	/* < DTS2014042407105 sunlibin 20140504 begin */
	/* Add debug log for testing data */
		for (ii = 0; ii < f54->report_size; ii += 2) {
			tp_log_debug("%s :%03d: %d\n",__func__,
					ii / 2,
					(f54->report_data[ii + 1]<<8)|f54->report_data[ii]);
		}
		break;
	default:
		for (ii = 0; ii < f54->report_size; ii++)
			/* <DTS2014071505823 wwx203500 20142407 begin */
			tp_log_info("%03d:default 0x%02x\n", ii, f54->report_data[ii]);
			/* DTS2014071505823 wwx203500 20142407 end> */
		break;
	/* DTS2014042407105 sunlibin 20140504 end> */
	}

	return;
}
#endif

#ifdef HUMAN_READABLE
static void print_image_report(void)
{
	unsigned int ii;
	unsigned int jj;
	short *report_data;

	switch (f54->report_type) {
	case F54_16BIT_IMAGE:
	case F54_RAW_16BIT_IMAGE:
	case F54_TRUE_BASELINE:
	case F54_FULL_RAW_CAP:
	case F54_FULL_RAW_CAP_RX_COUPLING_COMP:
		/* <DTS2014071505823 wwx203500 20142407 begin */
		tp_log_info("%s: Report data (image)\n", __func__);
		/* DTS2014071505823 wwx203500 20142407 end> */

		report_data = (short *)f54->report_data;

		for (ii = 0; ii < f54->tx_assigned; ii++) {
			for (jj = 0; jj < f54->rx_assigned; jj++) {
				if (*report_data < -64)
					pr_cont(".");
				else if (*report_data < 0)
					pr_cont("-");
				else if (*report_data > 64)
					pr_cont("*");
				else if (*report_data > 0)
					pr_cont("+");
				else
					pr_cont("0");

				report_data++;
			}
	/* <DTS2014071505823 wwx203500 20142407 begin */
			tp_log_info("");
	/* DTS2014071505823 wwx203500 20142407 end> */
		}
	/* <DTS2014071505823 wwx203500 20142407 begin */
		tp_log_info("%s: End of report\n", __func__);
	/* DTS2014071505823 wwx203500 20142407 end> */
		break;
	default:
	/* <DTS2014071505823 wwx203500 20142407 begin */
		tp_log_info("%s: Image not supported for report type %d\n",
	/* DTS2014071505823 wwx203500 20142407 end> */
				__func__, f54->report_type);
	}

	return;
}
#endif

static void free_control_mem(void)
{
	struct f54_control control = f54->control;

	kfree(control.reg_0);
	kfree(control.reg_1);
	kfree(control.reg_2);
	kfree(control.reg_3);
	kfree(control.reg_4__6);
	kfree(control.reg_7);
	kfree(control.reg_8__9);
	kfree(control.reg_10);
	kfree(control.reg_11);
	kfree(control.reg_12__13);
	kfree(control.reg_14);
	kfree(control.reg_15);
	kfree(control.reg_16);
	kfree(control.reg_17);
	kfree(control.reg_18);
	kfree(control.reg_19);
	kfree(control.reg_20);
	kfree(control.reg_21);
	kfree(control.reg_22__26);
	kfree(control.reg_27);
	kfree(control.reg_28);
	kfree(control.reg_29);
	kfree(control.reg_30);
	kfree(control.reg_31);
	kfree(control.reg_32__35);
	kfree(control.reg_36);
	kfree(control.reg_37);
	kfree(control.reg_38);
	kfree(control.reg_39);
	kfree(control.reg_40);
	kfree(control.reg_41);
	kfree(control.reg_57);

	return;
}

static void remove_sysfs(void)
{
	int reg_num;

	sysfs_remove_bin_file(f54->attr_dir, &dev_report_data);

	sysfs_remove_group(f54->attr_dir, &attr_group);

	for (reg_num = 0; reg_num < ARRAY_SIZE(attrs_ctrl_regs); reg_num++)
		sysfs_remove_group(f54->attr_dir, &attrs_ctrl_regs[reg_num]);

	kobject_put(f54->attr_dir);

	return;
}

static ssize_t synaptics_rmi4_f54_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->status);
}

static ssize_t synaptics_rmi4_f54_report_size_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->report_size);
}

static ssize_t synaptics_rmi4_f54_no_auto_cal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->no_auto_cal);
}

static ssize_t synaptics_rmi4_f54_no_auto_cal_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned char data;
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting > 1)
		return -EINVAL;

	retval = f54->fn_ptr->read(rmi4_data,
			f54->control_base_addr,
			&data,
			sizeof(data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read control register\n",
				__func__);
		return retval;
	}

	if ((data & NO_AUTO_CAL_MASK) == setting)
		return count;

	data = (data & ~NO_AUTO_CAL_MASK) | (data & NO_AUTO_CAL_MASK);

	retval = f54->fn_ptr->write(rmi4_data,
			f54->control_base_addr,
			&data,
			sizeof(data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to write control register\n",
				__func__);
		return retval;
	}

	f54->no_auto_cal = (setting == 1);

	return count;
}

static ssize_t synaptics_rmi4_f54_report_type_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->report_type);
}

/* < DTS2013121708848 sunlibin 20131217 begin */
/*Add synaptics capacitor test function */
#ifdef MMITEST
static int synaptics_rmi4_f54_report_type_set(char setting)
{
	int retval = 0;
	unsigned char data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	if (!is_report_type_valid((enum f54_report_types)setting)) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Report type not supported by driver\n",
				__func__);
		return -EINVAL;
	}

	mutex_lock(&f54->status_mutex);

	if (f54->status != STATUS_BUSY) {
		f54->report_type = (enum f54_report_types)setting;
		data = (unsigned char)setting;
		retval = f54->fn_ptr->write(rmi4_data,
				f54->data_base_addr,
				&data,
				sizeof(data));
		mutex_unlock(&f54->status_mutex);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to write data register\n",
					__func__);
			return retval;
		}
		return 0;
	} else {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Previous get report still ongoing\n",
				__func__);
		mutex_unlock(&f54->status_mutex);
		return -EINVAL;
	}
}
#endif
/* DTS2013121708848 sunlibin 20131217 end >*/

static ssize_t synaptics_rmi4_f54_report_type_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned char data;
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (!is_report_type_valid((enum f54_report_types)setting)) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Report type not supported by driver\n",
				__func__);
		return -EINVAL;
	}

	mutex_lock(&f54->status_mutex);

	if (f54->status != STATUS_BUSY) {
		f54->report_type = (enum f54_report_types)setting;
		data = (unsigned char)setting;
		retval = f54->fn_ptr->write(rmi4_data,
				f54->data_base_addr,
				&data,
				sizeof(data));
		mutex_unlock(&f54->status_mutex);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to write data register\n",
					__func__);
			return retval;
		}
		return count;
	} else {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Previous get report still ongoing\n",
				__func__);
		mutex_unlock(&f54->status_mutex);
		return -EINVAL;
	}
}

static ssize_t synaptics_rmi4_f54_fifoindex_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int retval;
	unsigned char data[2];
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = f54->fn_ptr->read(rmi4_data,
			f54->data_base_addr + DATA_REPORT_INDEX_OFFSET,
			data,
			sizeof(data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read data registers\n",
				__func__);
		return retval;
	}

	batohs(&f54->fifoindex, data);

	return snprintf(buf, PAGE_SIZE, "%u\n", f54->fifoindex);
}
static ssize_t synaptics_rmi4_f54_fifoindex_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned char data[2];
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	f54->fifoindex = setting;

	hstoba(data, (unsigned short)setting);

	retval = f54->fn_ptr->write(rmi4_data,
			f54->data_base_addr + DATA_REPORT_INDEX_OFFSET,
			data,
			sizeof(data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to write data registers\n",
				__func__);
		return retval;
	}

	return count;
}

/* < DTS2013121708848 sunlibin 20131217 begin */
/*Add synaptics capacitor test function */
#ifdef MMITEST
static int synaptics_rmi4_f54_do_preparation_set(void)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	mutex_lock(&f54->status_mutex);

	if (f54->status != STATUS_IDLE) {
		if (f54->status != STATUS_BUSY) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Invalid status (%d)\n",
					__func__, f54->status);
		} else {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Previous get report still ongoing\n",
					__func__);
		}
		mutex_unlock(&f54->status_mutex);
		return -EBUSY;
	}

	mutex_unlock(&f54->status_mutex);

	retval = do_preparation();
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to do preparation\n",
				__func__);
		return retval;
	}
	return 0;
}
#endif
/* DTS2013121708848 sunlibin 20131217 end >*/

static ssize_t synaptics_rmi4_f54_do_preparation_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting != 1)
		return -EINVAL;

	mutex_lock(&f54->status_mutex);

	if (f54->status != STATUS_IDLE) {
		if (f54->status != STATUS_BUSY) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Invalid status (%d)\n",
					__func__, f54->status);
		} else {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Previous get report still ongoing\n",
					__func__);
		}
		mutex_unlock(&f54->status_mutex);
		return -EBUSY;
	}

	mutex_unlock(&f54->status_mutex);

	retval = do_preparation();
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to do preparation\n",
				__func__);
		return retval;
	}

	return count;
}

/* < DTS2013121708848 sunlibin 20131217 begin */
/*Add synaptics capacitor test function */
#ifdef MMITEST
static ssize_t synaptics_rmi4_f54_get_report_set(char setting)
{
	int retval;
	unsigned char command;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	if (setting != 1)
		return -EINVAL;

	command = (unsigned char)COMMAND_GET_REPORT;

	if (!is_report_type_valid(f54->report_type)) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Invalid report type\n",
				__func__);
		return -EINVAL;
	}

	mutex_lock(&f54->status_mutex);

	if (f54->status != STATUS_IDLE) {
		if (f54->status != STATUS_BUSY) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Invalid status (%d)\n",
					__func__, f54->status);
		} else {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Previous get report still ongoing\n",
					__func__);
		}
		mutex_unlock(&f54->status_mutex);
		return -EBUSY;
	}

	set_interrupt(true);

	f54->status = STATUS_BUSY;

	retval = f54->fn_ptr->write(rmi4_data,
			f54->command_base_addr,
			&command,
			sizeof(command));
	mutex_unlock(&f54->status_mutex);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to write get report command\n",
				__func__);
		return retval;
	}

#ifdef WATCHDOG_HRTIMER
	hrtimer_start(&f54->watchdog,
			ktime_set(WATCHDOG_TIMEOUT_S, 0),
			HRTIMER_MODE_REL);
#endif

	return 0;
}
#endif
/* DTS2013121708848 sunlibin 20131217 end >*/

static ssize_t synaptics_rmi4_f54_get_report_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned char command;
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting != 1)
		return -EINVAL;

	command = (unsigned char)COMMAND_GET_REPORT;

	if (!is_report_type_valid(f54->report_type)) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Invalid report type\n",
				__func__);
		return -EINVAL;
	}

	mutex_lock(&f54->status_mutex);

	if (f54->status != STATUS_IDLE) {
		if (f54->status != STATUS_BUSY) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Invalid status (%d)\n",
					__func__, f54->status);
		} else {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Previous get report still ongoing\n",
					__func__);
		}
		mutex_unlock(&f54->status_mutex);
		return -EBUSY;
	}

	set_interrupt(true);

	f54->status = STATUS_BUSY;

	retval = f54->fn_ptr->write(rmi4_data,
			f54->command_base_addr,
			&command,
			sizeof(command));
	mutex_unlock(&f54->status_mutex);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to write get report command\n",
				__func__);
		return retval;
	}

#ifdef WATCHDOG_HRTIMER
	hrtimer_start(&f54->watchdog,
			ktime_set(WATCHDOG_TIMEOUT_S, 0),
			HRTIMER_MODE_REL);
#endif

	return count;
}

static ssize_t synaptics_rmi4_f54_force_cal_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned char command;
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting != 1)
		return -EINVAL;

	command = (unsigned char)COMMAND_FORCE_CAL;

	if (f54->status == STATUS_BUSY)
		return -EBUSY;

	retval = f54->fn_ptr->write(rmi4_data,
			f54->command_base_addr,
			&command,
			sizeof(command));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to write force cal command\n",
				__func__);
		return retval;
	}

	return count;
}

/* < DTS2013121708848 sunlibin 20131217 begin */
/*Add synaptics capacitor test function */
#ifdef MMITEST
/* < DTS2014033108554 songrongyuan 20140408 begin */
/* To add the log for failure branch */
static void mmi_rawimage_report(unsigned char *buffer)
{
 	int i, j, k=0;
	short *DataArray;
	short temp;
	enum mmi_results TestResult = TEST_PASS;
/* < DTS2014011405074 sunlibin 20140114 begin */
	char buf[F54_MAX_CAP_DATA_SIZE] = {0};

	/*<DTS2014031803547 yuxuesong 20140318 begin */
	if	(NULL == g_mmi_buf_f54raw_data){
		g_mmi_buf_f54raw_data = kmalloc(F54_FULL_RAW_CAP_RX_COUPLING_COMP_SIZE(rx,tx) , GFP_KERNEL);
		if (!g_mmi_buf_f54raw_data){
			tp_log_err("%s:Fail to malloc g_mmi_buf_f54raw_data buffer!\n",__func__);
			return;
		}
	}

	memset(g_mmi_buf_f54raw_data, 0, F54_FULL_RAW_CAP_RX_COUPLING_COMP_SIZE(rx,tx));
	/* DTS2014031803547 yuxuesong 20140318 end> */
/* DTS2014011405074 sunlibin 20140114 end > */
	/* <DTS2014062507056 weiqiangqiang 20140625 begin */
	snprintf(g_mmi_buf_f54raw_data, F54_FULL_RAW_CAP_RX_COUPLING_COMP_SIZE(rx,tx),"RawImageData:\n");
	/* DTS2014062507056 weiqiangqiang 20140625 end> */

	DataArray = (short*)kmalloc(sizeof(short)*tx*rx, GFP_KERNEL);

	for (i = 0; i < tx; i++) {
	   	for (j = 0; j < rx; j++) {
			temp = buffer[k] | (buffer[k+1] << 8);
			DataArray[i*rx+j] = temp;	//float is not allowed in kernel space
			k = k + 2;

			/* <DTS2014062507056 weiqiangqiang 20140625 begin */
			snprintf(buf, sizeof(buf),"%d ", temp);			
			/* DTS2014062507056 weiqiangqiang 20140625 end> */
			strncat(g_mmi_buf_f54raw_data, buf ,sizeof(buf));
		}
		strncat(g_mmi_buf_f54raw_data, "\n", 1);
   	}

	/* < DTS2014070819376 sunlibin 20140709 begin */
	/*For Probability of tp cap test crush*/
	if ((NULL == f54->f54_full_raw_max_cap)||(NULL == f54->f54_full_raw_min_cap))
	{
		TestResult = TEST_FAILED;
		tp_log_err("%s: Failed,rawimage limit = NULL!\n",__func__);
		goto rawimage_test_end;
	}
	/* DTS2014070819376 sunlibin 20140709 end> */

	for (i = 0; i < tx; i++) {
	   	for (j = 0; j < rx; j++) {
/* < DTS2014012604495 sunlibin 20140126 begin */
			if ((DataArray[i*rx+j] >= *(f54->f54_full_raw_max_cap+i*rx+j)) ||
				(DataArray[i*rx+j] <= *(f54->f54_full_raw_min_cap+i*rx+j)))
			{
				TestResult = TEST_FAILED;
				/*<DTS2014031803547 yuxuesong 20140318 begin */
				/* Pick from DTS2014021804310 */
				tp_log_err("%s: Failed,DataArray(%d,%d) = %d\n",__func__,i,j,DataArray[i*rx+j]);
				goto rawimage_test_end;
				/* DTS2014031803547 yuxuesong 20140318 end> */
			}
/* DTS2014012604495 sunlibin 20140126 end> */
	  	}
	}

/*<DTS2014031803547 yuxuesong 20140318 begin */
/* Pick from DTS2014021804310 */
rawimage_test_end:
/* DTS2014031803547 yuxuesong 20140318 end> */
	if (TestResult)
		/* <DTS2014062507056 weiqiangqiang 20140625 begin */
	    strncat(g_mmi_buf_f54test_result,"1P-",sizeof("1P-"));
		/* DTS2014062507056 weiqiangqiang 20140625 end> */
	else
		/* <DTS2014062507056 weiqiangqiang 20140625 begin */
	    strncat(g_mmi_buf_f54test_result,"1F-",sizeof("1P-"));
		/* DTS2014062507056 weiqiangqiang 20140625 end> */
	
	kfree(DataArray);
	return;
}

static int RxtoRx1ShortTest(unsigned char* buffer)
{

	int i, j, k=0;
	int count = 0;
/* < DTS2014012604495 sunlibin 20140126 begin */
	int DiagonalUpperLimit = f54->f54_rx2rx_diagonal_max;
	int DiagonalLowerLimit = f54->f54_rx2rx_diagonal_min;
	int OthersUpperLimit = f54->f54_rx2rx_others;
/* DTS2014012604495 sunlibin 20140126 end> */
	short ImageArray;
/* < DTS2014011405074 sunlibin 20140114 begin */
	char buf[F54_MAX_CAP_DATA_SIZE] = {0};
/* DTS2014011405074 sunlibin 20140114 end > */

	for (i = 0; i < tx; i++)
	{
		for (j = 0; j <rx; j++)
		{
			ImageArray = buffer[k]|(buffer[k+1] << 8);
			k = k + 2;
			/* <DTS2014062507056 weiqiangqiang 20140625 begin */
			snprintf(buf,sizeof(buf), "%5d ", ImageArray);
			/* DTS2014062507056 weiqiangqiang 20140625 end> */

			if (i == j){
				strncat(g_mmi_RxtoRxshort_report, buf, sizeof(buf));
				if ((ImageArray <= DiagonalUpperLimit) && (ImageArray >= DiagonalLowerLimit))
					count++;
				else
					tp_log_err("%s: Failed,ImageArray(%d,%d) = %d\n",__func__,i,j,ImageArray);
			}else{
				if (ImageArray <= OthersUpperLimit)
					count++;
				else
					tp_log_err("%s: Failed,ImageArray(%d,%d) = %d\n",__func__,i,j,ImageArray);
			}
		}
		//strncat(g_mmi_RxtoRxshort_report, "\n", 1);
	}
	return count;
}

static int RxtoRx2ShortTest(unsigned char* buffer)
{

	int i, j, k=0;
	int count = 0;
/* < DTS2014012604495 sunlibin 20140126 begin */
	int DiagonalUpperLimit = f54->f54_rx2rx_diagonal_max;
	int DiagonalLowerLimit = f54->f54_rx2rx_diagonal_min;
	int OthersUpperLimit = f54->f54_rx2rx_others;
/* DTS2014012604495 sunlibin 20140126 end> */
	short ImageArray;
/* < DTS2014011405074 sunlibin 20140114 begin */
	char buf[F54_MAX_CAP_DATA_SIZE] = {0};
/* DTS2014011405074 sunlibin 20140114 end > */
	
	for (i = 0; i < (rx-tx); i++)
	{
		for (j = 0; j <rx; j++)
		{
			ImageArray = buffer[k]|(buffer[k+1] << 8);
			k = k + 2;
			/* <DTS2014062507056 weiqiangqiang 20140625 begin */
			snprintf(buf, sizeof(buf),"%5d ", ImageArray);
			/* DTS2014062507056 weiqiangqiang 20140625 end> */

			if ((i+tx) == j){
				strncat(g_mmi_RxtoRxshort_report, buf, sizeof(buf));
				if((ImageArray <= DiagonalUpperLimit) && (ImageArray >= DiagonalLowerLimit))
					count++;
				else
					tp_log_err("%s: Failed,ImageArray(%d,%d) = %d\n",__func__,i,j,ImageArray);
			}else{
				if(ImageArray <= OthersUpperLimit)
					count++;
				else
					tp_log_err("%s: Failed,ImageArray(%d,%d) = %d\n",__func__,i,j,ImageArray);
			}
		}
		//strncat(g_mmi_RxtoRxshort_report, "\n", 1);
	}
	strncat(g_mmi_RxtoRxshort_report, "\n", 1);
	return count;
}

static void mmi_RxtoRxshort1_report(unsigned char* buffer)
{

	int count=0;
	enum mmi_results TestResult = TEST_PASS;

	f54->rx2rx_tmp_count = 0;
	/*< DTS2014031803547 yuxuesong 20140318 begin */
	if	(NULL == g_mmi_RxtoRxshort_report){
		g_mmi_RxtoRxshort_report = kmalloc(F54_RX_TO_RX1_SIZE(rx), GFP_KERNEL);
		if (!g_mmi_RxtoRxshort_report){
			tp_log_err("%s:Fail to malloc g_mmi_RxtoRxshort_report buffer!\n",__func__);
			return;
		}
	}

/* < DTS2014011405074 sunlibin 20140114 begin */
	memset(g_mmi_RxtoRxshort_report, 0, F54_RX_TO_RX1_SIZE(rx));//only Diagonal for V3
/* DTS2014011405074 sunlibin 20140114 end > */
	/* DTS2014031803547 yuxuesong 20140318 end> */
	/* <DTS2014062507056 weiqiangqiang 20140625 begin */
	snprintf(g_mmi_RxtoRxshort_report, F54_RX_TO_RX1_SIZE(rx),"RxtoRxshort:\n");
	/* DTS2014062507056 weiqiangqiang 20140625 end> */

	count = RxtoRx1ShortTest(buffer);
	//count += RxtoRx2ShortTest(buffer);

	if(count != (tx * rx)){
		TestResult = TEST_FAILED;
		tp_log_err("%s:test failed,count = %d LINE = %d\n",__func__,count,__LINE__);
	}

	f54->rx2rx_tmp_count = count;
	return;
}

static void mmi_RxtoRxshort2_report(unsigned char* buffer)
{

	int count = f54->rx2rx_tmp_count;	
	enum mmi_results TestResult = TEST_PASS;
	
	count += RxtoRx2ShortTest(buffer);

	/* < DTS2014070819376 sunlibin 20140709 begin */
	/*For Probability of tp cap test crush*/
	if ((0 == f54->f54_rx2rx_diagonal_max)||(0 == f54->f54_rx2rx_diagonal_min)||(0 == f54->f54_rx2rx_others))
	{
		tp_log_err("%s: Failed,rx2rx limit = zero!\n",__func__);
	}
	/* DTS2014070819376 sunlibin 20140709 end> */

	if(count == (rx * rx)){
	 	TestResult = TEST_PASS;
		/* <DTS2014062507056 weiqiangqiang 20140625 begin */
	 	strncat(g_mmi_buf_f54test_result,"4P-", sizeof("4P-"));
		/* DTS2014062507056 weiqiangqiang 20140625 end> */
	} else{
		TestResult = TEST_FAILED;
		tp_log_err("%s:test failed,count = %d LINE = %d\n",__func__,count,__LINE__);
		/* <DTS2014062507056 weiqiangqiang 20140625 begin */
	 	strncat(g_mmi_buf_f54test_result,"4F-",sizeof("4F-"));
		/* DTS2014062507056 weiqiangqiang 20140625 end> */
	}

	return;
}

static void mmi_txtotx_short_report(unsigned char* buffer)
{
 	int i, j, index;
	int numberOfBytes = (f54->tx_assigned + 7) / 8;
	char val;
	enum mmi_results TestResult = TEST_PASS;

	for (i = 0; i < numberOfBytes; i++) {
		for (j = 0; j < 8; j++) {
			index =  i*8+j;
			if (index >= f54->tx_assigned)
				break;
			val = (buffer[i] & (1 << j)) >>j;
			if (numberOfBytes < tx) {
/* < DTS2014012604495 sunlibin 20140126 begin */
				if (val != f54->f54_tx2tx_limit){
					TestResult = TEST_FAILED;
					tp_log_err("%s: Failed,val(%d,%d) = %d",__func__,i,j,val);
				}
/* DTS2014012604495 sunlibin 20140126 end> */
			}
		}
   	}

	if (TestResult)
		/* <DTS2014062507056 weiqiangqiang 20140625 begin */
	    strncat(g_mmi_buf_f54test_result,"2P-",sizeof("2P-"));
		/* DTS2014062507056 weiqiangqiang 20140625 end> */
	else
		/* <DTS2014062507056 weiqiangqiang 20140625 begin */
	    strncat(g_mmi_buf_f54test_result,"2F-",sizeof("2F-"));
		/* DTS2014062507056 weiqiangqiang 20140625 end> */
	
	return;
}

static void mmi_txtoground_short_report(unsigned char* buffer, size_t report_size)
{
	enum mmi_results TestResult = TEST_PASS;

	char Txstatus;
	int result = 0;
	int i,j;
	
	for (i = 0;i < report_size;i++)
	{
		for (j = 0; j < 8; j++)
		{
			Txstatus = (buffer[i] & (1 << j)) >> j;
				if (1 == Txstatus)
					result++;
			}
		}
	
		if ((tx) == result)
		{
			TestResult = TEST_PASS;
			/* <DTS2014062507056 weiqiangqiang 20140625 begin */
			strncat(g_mmi_buf_f54test_result,"3P-",sizeof("3P-"));
			/* DTS2014062507056 weiqiangqiang 20140625 end> */
		}
		else
		{
			/*<DTS2014031803547 yuxuesong 20140318 begin */
			tp_log_err("%s: Failed in txtoground, result = %d\n",__func__,result);
			/* DTS2014031803547 yuxuesong 20140318 end> */
			TestResult = TEST_FAILED;
			/* <DTS2014062507056 weiqiangqiang 20140625 begin */
			 strncat(g_mmi_buf_f54test_result,"3F-",sizeof("3F-"));	
			/* DTS2014062507056 weiqiangqiang 20140625 end> */
		}

	return;
}

static void mmi_maxmincapacitance_report(unsigned char* buffer)
{
	enum mmi_results TestResult = TEST_PASS;

/* < DTS2014012604495 sunlibin 20140126 begin */
	short maxcapacitance = f54->f54_raw_max_cap;
	short mincapacitance = f54->f54_raw_min_cap;
/* DTS2014012604495 sunlibin 20140126 end> */
	short max = 0;
	short min = 0;
/* < DTS2014011405074 sunlibin 20140114 begin */
	char buf[F54_MAX_CAP_DATA_SIZE*F54_RAW_CAP_MIN_MAX_DATA_NUM] = {0};

	/*<DTS2014031803547 yuxuesong 20140318 begin */
	if(NULL == g_mmi_maxmincapacitance_report){
		g_mmi_maxmincapacitance_report = kmalloc(F54_FULL_RAW_CAP_MIN_MAX_SIZE, GFP_KERNEL);
		if (!g_mmi_maxmincapacitance_report){
			tp_log_err("%s:Fail to malloc g_mmi_maxmincapacitance_report buffer!\n",__func__);
			return;
		}
	}

	memset(g_mmi_maxmincapacitance_report, 0, F54_FULL_RAW_CAP_MIN_MAX_SIZE);
	/* DTS2014031803547 yuxuesong 20140318 end> */
/* DTS2014011405074 sunlibin 20140114 end > */
	/* <DTS2014062507056 weiqiangqiang 20140625 begin */
	snprintf(g_mmi_maxmincapacitance_report, F54_FULL_RAW_CAP_MIN_MAX_SIZE,"maxmincapacitance:\n");
	/* DTS2014062507056 weiqiangqiang 20140625 end> */
	max = (buffer[0])|(buffer[1] << 8);
	min = (buffer[2])|(buffer[3] << 8);

	/* < DTS2014070819376 sunlibin 20140709 begin */
	/*For Probability of tp cap test crush*/
	/* <DTS2014062507056 weiqiangqiang 20140625 begin */
	snprintf(buf, sizeof(buf)," %d %d", max, min);
	/* DTS2014062507056 weiqiangqiang 20140625 end> */
	strncat(g_mmi_maxmincapacitance_report, buf, sizeof(buf));
	strncat(g_mmi_maxmincapacitance_report, "\n", 1);	

	if (0 == f54->f54_raw_max_cap)
	{
		TestResult = TEST_FAILED;
		tp_log_err("%s: Failed,highRes limit = zero!\n",__func__);
		goto maxmin_test_end;
	}
	
	if ((max < maxcapacitance)&& (min > mincapacitance)){
		TestResult = TEST_PASS;
	}else{
		TestResult = TEST_FAILED;;		
		tp_log_err("%s: Failed,max= %d,min= %d \n",__func__,max,min);
		goto maxmin_test_end;
	}
		
maxmin_test_end:
	if(TestResult)
		/* <DTS2014062507056 weiqiangqiang 20140625 begin */
		strncat(g_mmi_buf_f54test_result,"5P-",sizeof("5P-"));
		/* DTS2014062507056 weiqiangqiang 20140625 end> */
	else
		/* <DTS2014062507056 weiqiangqiang 20140625 begin */
		strncat(g_mmi_buf_f54test_result,"5F-",sizeof("5F-"));
		/* DTS2014062507056 weiqiangqiang 20140625 end> */
	/* DTS2014070819376 sunlibin 20140709 end> */

	return;
}

static void mmi_highresistance_report(unsigned char* buffer)
{
	int i, k=0;
	short temp;
	enum mmi_results TestResult = TEST_PASS;
/* < DTS2014011405074 sunlibin 20140114 begin */
	short HighResistanceResult[F54_HIGH_RESISTANCE_DATA_NUM];
	char buf[F54_MAX_CAP_DATA_SIZE] = {0};

	/*<DTS2014031803547 yuxuesong 20140318 begin */
	if (NULL == g_mmi_highresistance_report){
		g_mmi_highresistance_report = kmalloc(F54_HIGH_RESISTANCE_SIZE, GFP_KERNEL);
		if (!g_mmi_highresistance_report){
			tp_log_err("%s:Fail to malloc g_mmi_highresistance_report buffer!\n",__func__);
			return;
		}
	}

	memset(g_mmi_highresistance_report, 0, F54_HIGH_RESISTANCE_SIZE);
	/* DTS2014031803547 yuxuesong 20140318 end> */
	sprintf(g_mmi_highresistance_report, "highresistance:\n");

	/* < DTS2014070819376 sunlibin 20140709 begin */
	/*For Probability of tp cap test crush*/
	for (i = 0; i <F54_HIGH_RESISTANCE_DATA_NUM; i++, k+=2) {
		temp = buffer[k] | (buffer[k+1] << 8);
		HighResistanceResult[i] = temp;

		snprintf(buf, sizeof(buf),"%d ", HighResistanceResult[i]);
		strncat(g_mmi_highresistance_report, buf, sizeof(buf));
	}
/* DTS2014011405074 sunlibin 20140114 end > */

	strncat(g_mmi_highresistance_report, "\n", 1);

	if ((NULL == f54->f54_highRes_max)||(NULL == f54->f54_highRes_min))
	{
		TestResult = TEST_FAILED;
		tp_log_err("%s: Failed,highRes limit = NULL!\n",__func__);
		goto highRes_test_end;
	}

	for (i = 0; i <F54_HIGH_RESISTANCE_DATA_NUM; i++) {
		if ((HighResistanceResult[i] > *(f54->f54_highRes_max+i)) ||
			(HighResistanceResult[i] < *(f54->f54_highRes_min+i))){
			TestResult = TEST_FAILED;
			tp_log_err("%s: Failed,HighResistanceResult[%d]=%d \n",__func__,i,HighResistanceResult[i]);
			goto highRes_test_end;
		}
	}
	
highRes_test_end:
	/* DTS2014070819376 sunlibin 20140709 end> */
	if(TestResult)
		/* <DTS2014062507056 weiqiangqiang 20140625 begin */
		strncat(g_mmi_buf_f54test_result,"6P-", sizeof("6P-"));
		/* DTS2014062507056 weiqiangqiang 20140625 end> */
	else
		/* <DTS2014062507056 weiqiangqiang 20140625 begin */
		strncat(g_mmi_buf_f54test_result,"6F-", sizeof("6F-"));
		/* DTS2014062507056 weiqiangqiang 20140625 end> */

	return;
}
/* DTS2014033108554 songrongyuan 20140408 end > */

/*<DTS2014031803547 yuxuesong 20140318 begin */
static int mmi_runtest(int report_type)
{
	int retval = 0;
	/* Pick from DTS2014021804310 */
	/* more time to wait f54->status */
	/* < DTS2014042407105 sunlibin 20140504 begin */
	/* More patience for timeout */
	unsigned char patience = MAX_IRQ_WAIT_PATIENCE;//50;
	/* DTS2014042407105 sunlibin 20140504 end> */
	int report_size;
	unsigned char *buffer = NULL;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	
	retval = synaptics_rmi4_f54_report_type_set(report_type);
	if (retval)
		goto test_exit;

	retval = synaptics_rmi4_f54_get_report_set(1);
	if (retval)
		goto test_exit;

	do {
		/* < DTS2014042407105 sunlibin 20140504 begin */
		/* Less time to wait each patience to reduce time consumption */
		msleep(STATUS_WORK_TIMEOUT);//100
		/* DTS2014042407105 sunlibin 20140504 end> */
		if (f54->status != STATUS_BUSY)
			break;
	} while (--patience > 0);

	/* Pick from DTS2014021804310 */
	/* < DTS2014042407105 sunlibin 20140504 begin */
	tp_log_info("%s  reportType=%d, patience=%d, status=%d \n", __func__, report_type, patience, f54->status);
	/* DTS2014042407105 sunlibin 20140504 end> */

	/*set the error number to retval*/
	retval = -ENOMEM;

	report_size = f54->report_size;
	tp_log_debug("%s  report_size=%d \n", __func__, report_size);
	if (!report_size)
		goto test_exit;
	buffer = kzalloc(report_size, GFP_KERNEL);
	if (!buffer){
		dev_err(&rmi4_data->i2c_client->dev, "%s: Faild to kzalloc %d buffer\n",
			__func__, report_size);
		goto test_exit;
	}

	//load test limit
	//LoadHighResistanceLimits();

	//simulate ReadBlockData()
	mutex_lock(&f54->data_mutex);
	if (f54->report_data) {
		memcpy(buffer, f54->report_data, f54->report_size);
		mutex_unlock(&f54->data_mutex);
	} else {
		mutex_unlock(&f54->data_mutex);
		goto test_exit;
	}

/* < DTS2014011405074 sunlibin 20140114 begin */
	switch (report_type) {
	case F54_FULL_RAW_CAP_MIN_MAX:
		/*move malloc buffer to mmi_maxmincapacitance_report()*/
		mmi_maxmincapacitance_report(buffer);
		break;
	case F54_FULL_RAW_CAP_RX_COUPLING_COMP:
		/*move malloc buffer to mmi_rawimage_report()*/
		mmi_rawimage_report(buffer);
		break;
	case F54_TX_TO_TX_SHORT:
		mmi_txtotx_short_report(buffer);
		break;
	case F54_HIGH_RESISTANCE:
		/*move malloc buffer to mmi_highresistance_report()*/
		mmi_highresistance_report(buffer);
		break;
	case F54_TX_TO_GROUND:
		mmi_txtoground_short_report(buffer, report_size);
		break;
	case F54_RX_TO_RX1:
		/*move malloc buffer to mmi_RxtoRxshort1_report()*/
		mmi_RxtoRxshort1_report(buffer);
		break;
	case F54_RX_TO_RX2:
		//g_mmi_RxtoRxshort_report = kmalloc(4536, GFP_KERNEL);
		mmi_RxtoRxshort2_report(buffer);
		break;
	default:
		break;
	}
/* DTS2014011405074 sunlibin 20140114 end > */

	kfree(buffer);
	return 0;

test_exit:
	dev_err(&rmi4_data->i2c_client->dev,
			"%s: Faild to run test\n",
			__func__);
	if (buffer)
		kfree(buffer);
	return retval;
}
/* DTS2014031803547 yuxuesong 20140318 end> */

/* < DTS2014072402403 yangyang 20140805 begin */
/* < DTS2014102002219 songrongyuan 20141020 begin */
static ssize_t synaptics_rmi4_f54_mmi_test_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	/*always return true in order to suit the struction of mmi test */
	return count;
}
/* DTS2014102002219 songrongyuan 20141020 end> */
/* < DTS2014102002219 songrongyuan 20141020 begin */
static int synaptics_rmi4_f54_mmi_test(void)
{
	int retval = 0;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	tp_log_debug("%s#%d\n",__func__, __LINE__);
/* < DTS2014011405074 sunlibin 20140114 begin */
	/* < DTS2013123006678 zhangmin 20131230 begin */
	/* < DTS2014080506603  caowei 201400806 begin */
	/* < DTS2014090101734  caowei 201400902 begin */
	if (rmi4_data->board->esd_support) {
		synaptics_dsx_esd_suspend();
	}
	/* DTS2014090101734  caowei 201400902 end >*/
	/* DTS2014080506603  caowei 201400806 end >*/
	if (!g_mmi_buf_f54test_result)
	{
		g_mmi_buf_f54test_result = kmalloc(F54_MAX_CAP_RESULT_SIZE, GFP_KERNEL);
		if (!g_mmi_buf_f54test_result) {
			tp_log_err("%s#%d g_mmi_buf_f54test_result kmalloc failed.\n",__func__,__LINE__);
			retval = -ENOMEM;
			goto exit;
		}
	}
	/* DTS2013123006678 zhangmin 20131230 end > */
	if (g_mmi_buf_f54test_result)
		memset(g_mmi_buf_f54test_result, 0, F54_MAX_CAP_RESULT_SIZE);
	else {
		tp_log_err("%s#%d g_mmi_buf_f54test_result is 0\n",__func__, __LINE__);
		goto exit;
	}
/* DTS2014011405074 sunlibin 20140114 end > */
		
	retval = synaptics_rmi4_f54_do_preparation_set();
	if (retval) {
		tp_log_err("%s#%d synaptics_rmi4_f54_do_preparation_set call failed.\n",__func__, __LINE__);
		goto exit;
	}
	/* <DTS2014062507056 weiqiangqiang 20140625 begin */
	strncat(g_mmi_buf_f54test_result, "0P-",sizeof("0P-"));
	/* DTS2014062507056 weiqiangqiang 20140625 end> */

	/* < DTS2014031803547 yuxuesong 20140318 begin */
	/* Pick from DTS2014021804310 */
	/* < DTS2014042407105 sunlibin 20140504 begin */
	tp_log_info("%s begin \n", __func__);
	/* DTS2014042407105 sunlibin 20140504 end> */
	/* DTS2014031803547 yuxuesong 20140318 end> */
	retval = mmi_runtest(F54_HIGH_RESISTANCE);
	if (retval)
		goto exit;

	retval = mmi_runtest(F54_FULL_RAW_CAP_MIN_MAX);
	if (retval)
		goto exit;

	retval = mmi_runtest(F54_FULL_RAW_CAP_RX_COUPLING_COMP);
	if (retval)
		goto exit;

	retval = mmi_runtest(F54_RX_TO_RX1);	
	if (retval)
		goto exit;

	retval = mmi_runtest(F54_RX_TO_RX2);	
	if (retval)
		goto exit;

	retval = mmi_runtest(F54_TX_TO_GROUND);
	if (retval)
		goto exit;

	retval = mmi_runtest(F54_TX_TO_TX_SHORT);
	if (retval)
		goto exit;

	sysfs_is_busy = true;
	f54->rmi4_data->reset_device(rmi4_data);
	
	/* < DTS2014080506603  caowei 201400806 begin */
	/* < DTS2014090101734  caowei 201400902 begin */
	if (rmi4_data->board->esd_support) {
		synaptics_dsx_esd_resume();
	}
	/* DTS2014090101734  caowei 201400902 end >*/
	/* DTS2014080506603  caowei 201400806 end >*/
	return 0;

exit:
	/*< DTS2014031803547 yuxuesong 20140318 begin */
	/* Pick from DTS2014021804310 */
	/* if one test item failed, we also should rest device */
	sysfs_is_busy = true;
	f54->rmi4_data->reset_device(rmi4_data);
	/* DTS2014031803547 yuxuesong 20140318 end> */
	/* <DTS2014071505823 wwx203500 20142407 begin */
	tp_log_err("%s: Failed to run mmi test\n",
	/* DTS2014071505823 wwx203500 20142407 end> */
			__func__);
	/* < DTS2014042407105 sunlibin 20140504 begin */
	/* In case that test abort unexpectly and no F in result make it pass. */
	if(NULL != g_mmi_buf_f54test_result)
		/* <DTS2014062507056 weiqiangqiang 20140625 begin */
		strncat(g_mmi_buf_f54test_result,"F-",sizeof("F-"));
		/* DTS2014062507056 weiqiangqiang 20140625 end> */
	/* DTS2014042407105 sunlibin 20140504 end> */
	/* < DTS2014080506603  caowei 201400806 begin */
	/* < DTS2014090101734  caowei 201400902 begin */
	if (rmi4_data->board->esd_support) {
		synaptics_dsx_esd_resume();
	}
	/* DTS2014090101734  caowei 201400902 end >*/
	/* DTS2014080506603  caowei 201400806 end >*/
	return retval;
}
/* DTS2014072402403 yangyang 20140805 end > */
/* DTS2014102002219 songrongyuan 20141020 end> */
static ssize_t synaptics_rmi4_f54_mmi_test_result_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	/*< DTS2014031803547 yuxuesong 20140318 begin */
	/* Pick from DTS2014021905176 */
	int len = 0;
	/* < DTS2014042407105 sunlibin 20140504 begin */
	/* < DTS2014102002219 songrongyuan 20141020 begin */
	int retval = 0;

	retval = synaptics_rmi4_f54_mmi_test();
	if(retval)
	{
		tp_log_err("%s:synaptics_rmi4_f54_mmi_test faild",__func__);
	}
	else
	{
		tp_log_info("%s:synaptics_rmi4_f54_mmi_test success",__func__);
	}
	/* DTS2014102002219 songrongyuan 20141020 end> */
	/* <DTS2014071505823 wwx203500 20142407 begin */
	tp_log_warning("%s begin \n", __func__);
	/* DTS2014071505823 wwx203500 20142407 end> */
	/* DTS2014042407105 sunlibin 20140504 end> */
	if(NULL != g_mmi_buf_f54test_result)
	{
		/* < DTS2014042407105 sunlibin 20140504 begin */
		/* <DTS2014062507056 weiqiangqiang 20140625 begin */
		strncat(buf, g_mmi_buf_f54test_result,strlen(g_mmi_buf_f54test_result));
		/* DTS2014062507056 weiqiangqiang 20140625 end> */
		/* DTS2014042407105 sunlibin 20140504 end> */
		/* <DTS2014062507056 weiqiangqiang 20140625 begin */
		strncat(buf,"\n",1);
		/* DTS2014062507056 weiqiangqiang 20140625 end> */
		len += strlen(g_mmi_buf_f54test_result);
	}

	if(NULL != g_mmi_buf_f54raw_data)
	{
		/* <DTS2014062507056 weiqiangqiang 20140625 begin */
		strncat(buf, g_mmi_buf_f54raw_data,strlen(g_mmi_buf_f54raw_data));
		/* DTS2014062507056 weiqiangqiang 20140625 end> */
		len += strlen(g_mmi_buf_f54raw_data);
	}

	if(NULL != g_mmi_highresistance_report)
	{
		/* <DTS2014062507056 weiqiangqiang 20140625 begin */
		strncat(buf, g_mmi_highresistance_report,strlen(g_mmi_highresistance_report));
		/* DTS2014062507056 weiqiangqiang 20140625 end> */
		len += strlen(g_mmi_highresistance_report);
	}

	if(NULL != g_mmi_maxmincapacitance_report)
	{
		/* <DTS2014062507056 weiqiangqiang 20140625 begin */
		strncat(buf, g_mmi_maxmincapacitance_report,strlen(g_mmi_maxmincapacitance_report));
		/* DTS2014062507056 weiqiangqiang 20140625 end> */
		len += strlen(g_mmi_maxmincapacitance_report);
	}
	
	if(NULL != g_mmi_RxtoRxshort_report)
	{
		/* <DTS2014062507056 weiqiangqiang 20140625 begin */
		strncat(buf, g_mmi_RxtoRxshort_report,strlen(g_mmi_RxtoRxshort_report));
		/* DTS2014062507056 weiqiangqiang 20140625 end> */
		len += strlen(g_mmi_RxtoRxshort_report);
	}

	/* <DTS2014062507056 weiqiangqiang 20140625 begin */
	strncat(buf, "\0",1);
	/* DTS2014062507056 weiqiangqiang 20140625 end> */
	len += 1;

	/* DTS2014031803547 yuxuesong 20140318 end> */
	return len;
}

/* < DTS2014010309198 sunlibin 20140104 begin */
/*move to hw_tp_common.c*/
/* < DTS2014072402403 yangyang 20140805 begin */
static ssize_t hw_synaptics_mmi_test_store(struct kobject *dev,
		struct kobj_attribute *attr, const char *buf, size_t size)
{
	/* < DTS2014053000677 sunlibin 20140530 begin */
	/*Sync DTS2014051400740 from 8926,in case mmi fail */
	struct synaptics_rmi4_data *rmi4_data = NULL;
	struct device *cdev = NULL;
	tp_log_debug("%s#%d\n",__func__, __LINE__);
	if(!f54) {
		tp_log_err("%s#%d: f54 is NULL\n",__func__,__LINE__);
		return -EINVAL;
	}
	rmi4_data = f54->rmi4_data;
	cdev = &rmi4_data->input_dev->dev;
	/* DTS2014053000677 sunlibin 20140530 end> */
	if (!cdev){
		tp_log_err("%s: device is null", __func__);
		return -EINVAL;
	}
	return synaptics_rmi4_f54_mmi_test_store(cdev, NULL, buf, size);
}
/* DTS2014072402403 yangyang 20140805 end > */

static ssize_t hw_synaptics_mmi_test_result_show(struct kobject *dev,
		struct kobj_attribute *attr, char *buf)
{
	/* < DTS2014053000677 sunlibin 20140530 begin */
	/*Sync DTS2014051400740 from 8926,in case mmi fail */
	struct synaptics_rmi4_data *rmi4_data = NULL;
	struct device *cdev = NULL;
	if(!f54)
		return -EINVAL;
	rmi4_data = f54->rmi4_data;
	cdev = &rmi4_data->input_dev->dev;
	/* DTS2014053000677 sunlibin 20140530 end> */
	if (!cdev){
		tp_log_err("%s: device is null", __func__);
		return -EINVAL;
	}
	return synaptics_rmi4_f54_mmi_test_result_show(cdev, NULL, buf);
}

static struct kobj_attribute synaptics_mmi_test = {
	.attr = {.name = "synaptics_mmi_test", .mode = 0664},
	.show = NULL,
	.store = hw_synaptics_mmi_test_store,
};

static struct kobj_attribute synaptics_mmi_test_result = {
	.attr = {.name = "synaptics_mmi_test_result", .mode = 0444},
	.show = hw_synaptics_mmi_test_result_show,
	.store = NULL,
};
/* < DTS2014053000677 sunlibin 20140530 begin */
/*Sync DTS2014051400740 from 8926,in case mmi fail */
static int add_synaptics_mmi_test_interfaces(void)
{
	int error = 0;
	static int flag = 0;
	struct kobject *properties_kobj;

	if(0 != flag){
		return 0;
	}

	/* < DTS2014042407105 sunlibin 20140504 begin */
	tp_log_info("%s: in!\n", __func__);
	/* DTS2014042407105 sunlibin 20140504 end> */
	properties_kobj = tp_get_touch_screen_obj();
	if( NULL == properties_kobj )
	{
		tp_log_err("%s: Error, get kobj failed!\n", __func__);
		return -1;
	}

	/*add the node synaptics_mmi_test_result for apk to read*/
	error = sysfs_create_file(properties_kobj, &synaptics_mmi_test_result.attr);
	if (error)
	{
		kobject_put(properties_kobj);
		tp_log_err("%s: synaptics_mmi_test_result create file error = %d\n", __func__,error);
		return -ENODEV;
	}

	/*add the node synaptics_mmi_test apk to write*/
	error = sysfs_create_file(properties_kobj, &synaptics_mmi_test.attr);
	if (error)
	{
		kobject_put(properties_kobj);
		tp_log_err("%s: synaptics_mmi_test create file error\n", __func__);
		return -ENODEV;
	}
	flag = 1;
	return 0;
}
/* DTS2014053000677 sunlibin 20140530 end> */
/* DTS2014010309198 sunlibin 20140104 end > */

#endif
/* DTS2013121708848 sunlibin 20131217 end >*/

static ssize_t synaptics_rmi4_f54_resume_touch_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned long setting;

	retval = sstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting != 1)
		return -EINVAL;

	set_interrupt(false);

	return count;
}

static ssize_t synaptics_rmi4_f54_num_of_mapped_rx_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->rx_assigned);
}

static ssize_t synaptics_rmi4_f54_num_of_mapped_tx_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->tx_assigned);
}

simple_show_func_unsigned(query, num_of_rx_electrodes)
simple_show_func_unsigned(query, num_of_tx_electrodes)
simple_show_func_unsigned(query, has_image16)
simple_show_func_unsigned(query, has_image8)
simple_show_func_unsigned(query, has_baseline)
simple_show_func_unsigned(query, clock_rate)
simple_show_func_unsigned(query, touch_controller_family)
simple_show_func_unsigned(query, has_pixel_touch_threshold_adjustment)
simple_show_func_unsigned(query, has_sensor_assignment)
simple_show_func_unsigned(query, has_interference_metric)
simple_show_func_unsigned(query, has_sense_frequency_control)
simple_show_func_unsigned(query, has_firmware_noise_mitigation)
simple_show_func_unsigned(query, has_two_byte_report_rate)
simple_show_func_unsigned(query, has_one_byte_report_rate)
simple_show_func_unsigned(query, has_relaxation_control)
simple_show_func_unsigned(query, curve_compensation_mode)
simple_show_func_unsigned(query, has_iir_filter)
simple_show_func_unsigned(query, has_cmn_removal)
simple_show_func_unsigned(query, has_cmn_maximum)
simple_show_func_unsigned(query, has_touch_hysteresis)
simple_show_func_unsigned(query, has_edge_compensation)
simple_show_func_unsigned(query, has_per_frequency_noise_control)
simple_show_func_unsigned(query, has_signal_clarity)
simple_show_func_unsigned(query, number_of_sensing_frequencies)

show_store_func_unsigned(control, reg_0, no_relax)
show_store_func_unsigned(control, reg_0, no_scan)
show_store_func_unsigned(control, reg_1, bursts_per_cluster)
show_store_func_unsigned(control, reg_2, saturation_cap)
show_store_func_unsigned(control, reg_3, pixel_touch_threshold)
show_store_func_unsigned(control, reg_4__6, rx_feedback_cap)
show_store_func_unsigned(control, reg_4__6, low_ref_cap)
show_store_func_unsigned(control, reg_4__6, low_ref_feedback_cap)
show_store_func_unsigned(control, reg_4__6, low_ref_polarity)
show_store_func_unsigned(control, reg_4__6, high_ref_cap)
show_store_func_unsigned(control, reg_4__6, high_ref_feedback_cap)
show_store_func_unsigned(control, reg_4__6, high_ref_polarity)
show_store_func_unsigned(control, reg_7, cbc_cap)
show_store_func_unsigned(control, reg_7, cbc_polarity)
show_store_func_unsigned(control, reg_7, cbc_tx_carrier_selection)
show_store_func_unsigned(control, reg_8__9, integration_duration)
show_store_func_unsigned(control, reg_8__9, reset_duration)
show_store_func_unsigned(control, reg_10, noise_sensing_bursts_per_image)
show_store_func_unsigned(control, reg_12__13, slow_relaxation_rate)
show_store_func_unsigned(control, reg_12__13, fast_relaxation_rate)
show_store_func_unsigned(control, reg_14, rxs_on_xaxis)
show_store_func_unsigned(control, reg_14, curve_comp_on_txs)
show_store_func_unsigned(control, reg_20, disable_noise_mitigation)
show_store_func_unsigned(control, reg_21, freq_shift_noise_threshold)
show_store_func_unsigned(control, reg_22__26, medium_noise_threshold)
show_store_func_unsigned(control, reg_22__26, high_noise_threshold)
show_store_func_unsigned(control, reg_22__26, noise_density)
show_store_func_unsigned(control, reg_22__26, frame_count)
show_store_func_unsigned(control, reg_27, iir_filter_coef)
show_store_func_unsigned(control, reg_28, quiet_threshold)
show_store_func_unsigned(control, reg_29, cmn_filter_disable)
show_store_func_unsigned(control, reg_30, cmn_filter_max)
show_store_func_unsigned(control, reg_31, touch_hysteresis)
show_store_func_unsigned(control, reg_32__35, rx_low_edge_comp)
show_store_func_unsigned(control, reg_32__35, rx_high_edge_comp)
show_store_func_unsigned(control, reg_32__35, tx_low_edge_comp)
show_store_func_unsigned(control, reg_32__35, tx_high_edge_comp)
show_store_func_unsigned(control, reg_41, no_signal_clarity)
show_store_func_unsigned(control, reg_57, cbc_cap_0d)
show_store_func_unsigned(control, reg_57, cbc_polarity_0d)
show_store_func_unsigned(control, reg_57, cbc_tx_carrier_selection_0d)

show_replicated_func_unsigned(control, reg_15, sensor_rx_assignment)
show_replicated_func_unsigned(control, reg_16, sensor_tx_assignment)
show_replicated_func_unsigned(control, reg_17, disable)
show_replicated_func_unsigned(control, reg_17, filter_bandwidth)
show_replicated_func_unsigned(control, reg_19, stretch_duration)
show_replicated_func_unsigned(control, reg_38, noise_control_1)
show_replicated_func_unsigned(control, reg_39, noise_control_2)
show_replicated_func_unsigned(control, reg_40, noise_control_3)

show_store_replicated_func_unsigned(control, reg_36, axis1_comp)
show_store_replicated_func_unsigned(control, reg_37, axis2_comp)

static ssize_t synaptics_rmi4_f54_burst_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int retval;
	int size = 0;
	unsigned char ii;
	unsigned char *temp;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	mutex_lock(&f54->control_mutex);

	retval = f54->fn_ptr->read(rmi4_data,
			f54->control.reg_17->address,
			(unsigned char *)f54->control.reg_17->data,
			f54->control.reg_17->length);
	if (retval < 0) {
		dev_dbg(&rmi4_data->i2c_client->dev,
				"%s: Failed to read control reg_17\n",
				__func__);
	}

	retval = f54->fn_ptr->read(rmi4_data,
			f54->control.reg_18->address,
			(unsigned char *)f54->control.reg_18->data,
			f54->control.reg_18->length);
	if (retval < 0) {
		dev_dbg(&rmi4_data->i2c_client->dev,
				"%s: Failed to read control reg_18\n",
				__func__);
	}

	mutex_unlock(&f54->control_mutex);

	temp = buf;

	for (ii = 0; ii < f54->control.reg_17->length; ii++) {
		retval = snprintf(temp, PAGE_SIZE - size, "%u ", (1 << 8) *
			f54->control.reg_17->data[ii].burst_count_b8__10 +
			f54->control.reg_18->data[ii].burst_count_b0__7);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Faild to write output\n",
					__func__);
			return retval;
		}
		size += retval;
		temp += retval;
	}

	retval = snprintf(temp, PAGE_SIZE - size, "\n");
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Faild to write null terminator\n",
				__func__);
		return retval;
	}

	return size + retval;
}

static ssize_t synaptics_rmi4_f54_data_read(struct file *data_file,
		struct kobject *kobj, struct bin_attribute *attributes,
		char *buf, loff_t pos, size_t count)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	mutex_lock(&f54->data_mutex);

	if (count < f54->report_size) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Report type %d data size (%d) too large\n",
				__func__, f54->report_type, f54->report_size);
		mutex_unlock(&f54->data_mutex);
		return -EINVAL;
	}

	if (f54->report_data) {
		memcpy(buf, f54->report_data, f54->report_size);
		mutex_unlock(&f54->data_mutex);
		return f54->report_size;
	} else {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Report type %d data not available\n",
				__func__, f54->report_type);
		mutex_unlock(&f54->data_mutex);
		return -EINVAL;
	}
}

static int synaptics_rmi4_f54_set_sysfs(void)
{
	int retval;
	int reg_num;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	/* < DTS2014042407105 sunlibin 20140504 begin */
	tp_log_info("%s: in!\n", __func__);
	/* DTS2014042407105 sunlibin 20140504 end> */
	f54->attr_dir = kobject_create_and_add("f54",
			&rmi4_data->input_dev->dev.kobj);
	if (!f54->attr_dir) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to create sysfs directory\n",
				__func__);
		goto exit_1;
	}

	retval = sysfs_create_bin_file(f54->attr_dir, &dev_report_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to create sysfs bin file\n",
				__func__);
		goto exit_2;
	}

	retval = sysfs_create_group(f54->attr_dir, &attr_group);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to create sysfs attributes\n",
				__func__);
		goto exit_3;
	}

	for (reg_num = 0; reg_num < ARRAY_SIZE(attrs_ctrl_regs); reg_num++) {
		if (attrs_ctrl_regs_exist[reg_num]) {
			retval = sysfs_create_group(f54->attr_dir,
					&attrs_ctrl_regs[reg_num]);
			if (retval < 0) {
				dev_err(&rmi4_data->i2c_client->dev,
						"%s: Failed to create sysfs attributes\n",
						__func__);
				goto exit_4;
			}
		}
	}
	/* < DTS2014053000677 sunlibin 20140530 begin */
	/*Sync DTS2014051400740 from 8926,in case mmi fail */
	/* DTS2014053000677 sunlibin 20140530 end> */

	return 0;

exit_4:
	sysfs_remove_group(f54->attr_dir, &attr_group);

	for (reg_num--; reg_num >= 0; reg_num--)
		sysfs_remove_group(f54->attr_dir, &attrs_ctrl_regs[reg_num]);

exit_3:
	sysfs_remove_bin_file(f54->attr_dir, &dev_report_data);

exit_2:
	kobject_put(f54->attr_dir);

exit_1:
	return -ENODEV;
}

static int synaptics_rmi4_f54_set_ctrl(void)
{
	unsigned char length;
	unsigned char reg_num = 0;
	unsigned char num_of_sensing_freqs;
	unsigned short reg_addr = f54->control_base_addr;
	struct f54_control *control = &f54->control;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	num_of_sensing_freqs = f54->query.number_of_sensing_frequencies;

	/* control 0 */
	attrs_ctrl_regs_exist[reg_num] = true;
	control->reg_0 = kzalloc(sizeof(*(control->reg_0)),
			GFP_KERNEL);
	if (!control->reg_0)
		goto exit_no_mem;
	control->reg_0->address = reg_addr;
	reg_addr += sizeof(control->reg_0->data);
	reg_num++;

	/* control 1 */
	if ((f54->query.touch_controller_family == 0) ||
			(f54->query.touch_controller_family == 1)) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_1 = kzalloc(sizeof(*(control->reg_1)),
				GFP_KERNEL);
		if (!control->reg_1)
			goto exit_no_mem;
		control->reg_1->address = reg_addr;
		reg_addr += sizeof(control->reg_1->data);
	}
	reg_num++;

	/* control 2 */
	attrs_ctrl_regs_exist[reg_num] = true;
	control->reg_2 = kzalloc(sizeof(*(control->reg_2)),
			GFP_KERNEL);
	if (!control->reg_2)
		goto exit_no_mem;
	control->reg_2->address = reg_addr;
	reg_addr += sizeof(control->reg_2->data);
	reg_num++;

	/* control 3 */
	if (f54->query.has_pixel_touch_threshold_adjustment == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_3 = kzalloc(sizeof(*(control->reg_3)),
				GFP_KERNEL);
		if (!control->reg_3)
			goto exit_no_mem;
		control->reg_3->address = reg_addr;
		reg_addr += sizeof(control->reg_3->data);
	}
	reg_num++;

	/* controls 4 5 6 */
	if ((f54->query.touch_controller_family == 0) ||
			(f54->query.touch_controller_family == 1)) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_4__6 = kzalloc(sizeof(*(control->reg_4__6)),
				GFP_KERNEL);
		if (!control->reg_4__6)
			goto exit_no_mem;
		control->reg_4__6->address = reg_addr;
		reg_addr += sizeof(control->reg_4__6->data);
	}
	reg_num++;

	/* control 7 */
	if (f54->query.touch_controller_family == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_7 = kzalloc(sizeof(*(control->reg_7)),
				GFP_KERNEL);
		if (!control->reg_7)
			goto exit_no_mem;
		control->reg_7->address = reg_addr;
		reg_addr += sizeof(control->reg_7->data);
	}
	reg_num++;

	/* controls 8 9 */
	if ((f54->query.touch_controller_family == 0) ||
			(f54->query.touch_controller_family == 1)) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_8__9 = kzalloc(sizeof(*(control->reg_8__9)),
				GFP_KERNEL);
		if (!control->reg_8__9)
			goto exit_no_mem;
		control->reg_8__9->address = reg_addr;
		reg_addr += sizeof(control->reg_8__9->data);
	}
	reg_num++;

	/* control 10 */
	if (f54->query.has_interference_metric == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_10 = kzalloc(sizeof(*(control->reg_10)),
				GFP_KERNEL);
		if (!control->reg_10)
			goto exit_no_mem;
		control->reg_10->address = reg_addr;
		reg_addr += sizeof(control->reg_10->data);
	}
	reg_num++;

	/* control 11 */
	if (f54->query.has_ctrl11 == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_11 = kzalloc(sizeof(*(control->reg_11)),
				GFP_KERNEL);
		if (!control->reg_11)
			goto exit_no_mem;
		control->reg_11->address = reg_addr;
		reg_addr += sizeof(control->reg_11->data);
	}
	reg_num++;

	/* controls 12 13 */
	if (f54->query.has_relaxation_control == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_12__13 = kzalloc(sizeof(*(control->reg_12__13)),
				GFP_KERNEL);
		if (!control->reg_12__13)
			goto exit_no_mem;
		control->reg_12__13->address = reg_addr;
		reg_addr += sizeof(control->reg_12__13->data);
	}
	reg_num++;

	/* controls 14 15 16 */
	if (f54->query.has_sensor_assignment == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;

		control->reg_14 = kzalloc(sizeof(*(control->reg_14)),
				GFP_KERNEL);
		if (!control->reg_14)
			goto exit_no_mem;
		control->reg_14->address = reg_addr;
		reg_addr += sizeof(control->reg_14->data);

		control->reg_15 = kzalloc(sizeof(*(control->reg_15)),
				GFP_KERNEL);
		if (!control->reg_15)
			goto exit_no_mem;
		control->reg_15->length = f54->query.num_of_rx_electrodes;
		control->reg_15->data = kzalloc(control->reg_15->length *
				sizeof(*(control->reg_15->data)), GFP_KERNEL);
		if (!control->reg_15->data)
			goto exit_no_mem;
		control->reg_15->address = reg_addr;
		reg_addr += control->reg_15->length;

		control->reg_16 = kzalloc(sizeof(*(control->reg_16)),
				GFP_KERNEL);
		if (!control->reg_16)
			goto exit_no_mem;
		control->reg_16->length = f54->query.num_of_tx_electrodes;
		control->reg_16->data = kzalloc(control->reg_16->length *
				sizeof(*(control->reg_16->data)), GFP_KERNEL);
		if (!control->reg_16->data)
			goto exit_no_mem;
		control->reg_16->address = reg_addr;
		reg_addr += control->reg_16->length;
	}
	reg_num++;

	/* controls 17 18 19 */
	if (f54->query.has_sense_frequency_control == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;

		length = num_of_sensing_freqs;

		control->reg_17 = kzalloc(sizeof(*(control->reg_17)),
				GFP_KERNEL);
		if (!control->reg_17)
			goto exit_no_mem;
		control->reg_17->length = length;
		control->reg_17->data = kzalloc(length *
				sizeof(*(control->reg_17->data)), GFP_KERNEL);
		if (!control->reg_17->data)
			goto exit_no_mem;
		control->reg_17->address = reg_addr;
		reg_addr += length;

		control->reg_18 = kzalloc(sizeof(*(control->reg_18)),
				GFP_KERNEL);
		if (!control->reg_18)
			goto exit_no_mem;
		control->reg_18->length = length;
		control->reg_18->data = kzalloc(length *
				sizeof(*(control->reg_18->data)), GFP_KERNEL);
		if (!control->reg_18->data)
			goto exit_no_mem;
		control->reg_18->address = reg_addr;
		reg_addr += length;

		control->reg_19 = kzalloc(sizeof(*(control->reg_19)),
				GFP_KERNEL);
		if (!control->reg_19)
			goto exit_no_mem;
		control->reg_19->length = length;
		control->reg_19->data = kzalloc(length *
				sizeof(*(control->reg_19->data)), GFP_KERNEL);
		if (!control->reg_19->data)
			goto exit_no_mem;
		control->reg_19->address = reg_addr;
		reg_addr += length;
	}
	reg_num++;

	/* control 20 */
	attrs_ctrl_regs_exist[reg_num] = true;
	control->reg_20 = kzalloc(sizeof(*(control->reg_20)),
			GFP_KERNEL);
	if (!control->reg_20)
		goto exit_no_mem;
	control->reg_20->address = reg_addr;
	reg_addr += sizeof(control->reg_20->data);
	reg_num++;

	/* control 21 */
	if (f54->query.has_sense_frequency_control == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_21 = kzalloc(sizeof(*(control->reg_21)),
				GFP_KERNEL);
		if (!control->reg_21)
			goto exit_no_mem;
		control->reg_21->address = reg_addr;
		reg_addr += sizeof(control->reg_21->data);
	}
	reg_num++;

	/* controls 22 23 24 25 26 */
	if (f54->query.has_firmware_noise_mitigation == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_22__26 = kzalloc(sizeof(*(control->reg_22__26)),
				GFP_KERNEL);
		if (!control->reg_22__26)
			goto exit_no_mem;
		control->reg_22__26->address = reg_addr;
		reg_addr += sizeof(control->reg_22__26->data);
	}
	reg_num++;

	/* control 27 */
	if (f54->query.has_iir_filter == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_27 = kzalloc(sizeof(*(control->reg_27)),
				GFP_KERNEL);
		if (!control->reg_27)
			goto exit_no_mem;
		control->reg_27->address = reg_addr;
		reg_addr += sizeof(control->reg_27->data);
	}
	reg_num++;

	/* control 28 */
	if (f54->query.has_firmware_noise_mitigation == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_28 = kzalloc(sizeof(*(control->reg_28)),
				GFP_KERNEL);
		if (!control->reg_28)
			goto exit_no_mem;
		control->reg_28->address = reg_addr;
		reg_addr += sizeof(control->reg_28->data);
	}
	reg_num++;

	/* control 29 */
	if (f54->query.has_cmn_removal == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_29 = kzalloc(sizeof(*(control->reg_29)),
				GFP_KERNEL);
		if (!control->reg_29)
			goto exit_no_mem;
		control->reg_29->address = reg_addr;
		reg_addr += sizeof(control->reg_29->data);
	}
	reg_num++;

	/* control 30 */
	if (f54->query.has_cmn_maximum == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_30 = kzalloc(sizeof(*(control->reg_30)),
				GFP_KERNEL);
		if (!control->reg_30)
			goto exit_no_mem;
		control->reg_30->address = reg_addr;
		reg_addr += sizeof(control->reg_30->data);
	}
	reg_num++;

	/* control 31 */
	if (f54->query.has_touch_hysteresis == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_31 = kzalloc(sizeof(*(control->reg_31)),
				GFP_KERNEL);
		if (!control->reg_31)
			goto exit_no_mem;
		control->reg_31->address = reg_addr;
		reg_addr += sizeof(control->reg_31->data);
	}
	reg_num++;

	/* controls 32 33 34 35 */
	if (f54->query.has_edge_compensation == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_32__35 = kzalloc(sizeof(*(control->reg_32__35)),
				GFP_KERNEL);
		if (!control->reg_32__35)
			goto exit_no_mem;
		control->reg_32__35->address = reg_addr;
		reg_addr += sizeof(control->reg_32__35->data);
	}
	reg_num++;

	/* control 36 */
	if ((f54->query.curve_compensation_mode == 1) ||
			(f54->query.curve_compensation_mode == 2)) {
		attrs_ctrl_regs_exist[reg_num] = true;

		if (f54->query.curve_compensation_mode == 1) {
			length = max(f54->query.num_of_rx_electrodes,
					f54->query.num_of_tx_electrodes);
		} else if (f54->query.curve_compensation_mode == 2) {
			length = f54->query.num_of_rx_electrodes;
		}

		control->reg_36 = kzalloc(sizeof(*(control->reg_36)),
				GFP_KERNEL);
		if (!control->reg_36)
			goto exit_no_mem;
		control->reg_36->length = length;
		control->reg_36->data = kzalloc(length *
				sizeof(*(control->reg_36->data)), GFP_KERNEL);
		if (!control->reg_36->data)
			goto exit_no_mem;
		control->reg_36->address = reg_addr;
		reg_addr += length;
	}
	reg_num++;

	/* control 37 */
	if (f54->query.curve_compensation_mode == 2) {
		attrs_ctrl_regs_exist[reg_num] = true;

		control->reg_37 = kzalloc(sizeof(*(control->reg_37)),
				GFP_KERNEL);
		if (!control->reg_37)
			goto exit_no_mem;
		control->reg_37->length = f54->query.num_of_tx_electrodes;
		control->reg_37->data = kzalloc(control->reg_37->length *
				sizeof(*(control->reg_37->data)), GFP_KERNEL);
		if (!control->reg_37->data)
			goto exit_no_mem;

		control->reg_37->address = reg_addr;
		reg_addr += control->reg_37->length;
	}
	reg_num++;

	/* controls 38 39 40 */
	if (f54->query.has_per_frequency_noise_control == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;

		control->reg_38 = kzalloc(sizeof(*(control->reg_38)),
				GFP_KERNEL);
		if (!control->reg_38)
			goto exit_no_mem;
		control->reg_38->length = num_of_sensing_freqs;
		control->reg_38->data = kzalloc(control->reg_38->length *
				sizeof(*(control->reg_38->data)), GFP_KERNEL);
		if (!control->reg_38->data)
			goto exit_no_mem;
		control->reg_38->address = reg_addr;
		reg_addr += control->reg_38->length;

		control->reg_39 = kzalloc(sizeof(*(control->reg_39)),
				GFP_KERNEL);
		if (!control->reg_39)
			goto exit_no_mem;
		control->reg_39->length = num_of_sensing_freqs;
		control->reg_39->data = kzalloc(control->reg_39->length *
				sizeof(*(control->reg_39->data)), GFP_KERNEL);
		if (!control->reg_39->data)
			goto exit_no_mem;
		control->reg_39->address = reg_addr;
		reg_addr += control->reg_39->length;

		control->reg_40 = kzalloc(sizeof(*(control->reg_40)),
				GFP_KERNEL);
		if (!control->reg_40)
			goto exit_no_mem;
		control->reg_40->length = num_of_sensing_freqs;
		control->reg_40->data = kzalloc(control->reg_40->length *
				sizeof(*(control->reg_40->data)), GFP_KERNEL);
		if (!control->reg_40->data)
			goto exit_no_mem;
		control->reg_40->address = reg_addr;
		reg_addr += control->reg_40->length;
	}
	reg_num++;

	/* control 41 */
	if (f54->query.has_signal_clarity == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_41 = kzalloc(sizeof(*(control->reg_41)),
				GFP_KERNEL);
		if (!control->reg_41)
			goto exit_no_mem;
		control->reg_41->address = reg_addr;
		reg_addr += sizeof(control->reg_41->data);
	}
	reg_num++;

	/* control 42 */
	if (f54->query.has_variance_metric == 1)
		reg_addr += CONTROL_42_SIZE;

	/* controls 43 44 45 46 47 48 49 50 51 52 53 54 */
	if (f54->query.has_multi_metric_state_machine == 1)
		reg_addr += CONTROL_43_54_SIZE;

	/* controls 55 56 */
	if (f54->query.has_0d_relaxation_control == 1)
		reg_addr += CONTROL_55_56_SIZE;

	/* control 57 */
	if (f54->query.has_0d_acquisition_control == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_57 = kzalloc(sizeof(*(control->reg_57)),
				GFP_KERNEL);
		if (!control->reg_57)
			goto exit_no_mem;
		control->reg_57->address = reg_addr;
		reg_addr += sizeof(control->reg_57->data);
	}
	reg_num++;

	/* control 58 */
	if (f54->query.has_0d_acquisition_control == 1)
		reg_addr += CONTROL_58_SIZE;

	/* control 59 */
	if (f54->query.has_h_blank == 1)
		reg_addr += CONTROL_59_SIZE;

	/* controls 60 61 62 */
	if ((f54->query.has_h_blank == 1) ||
			(f54->query.has_v_blank == 1) ||
			(f54->query.has_long_h_blank == 1))
		reg_addr += CONTROL_60_62_SIZE;

	/* control 63 */
	if ((f54->query.has_h_blank == 1) ||
			(f54->query.has_v_blank == 1) ||
			(f54->query.has_long_h_blank == 1) ||
			(f54->query.has_slew_metric == 1) ||
			(f54->query.has_slew_option == 1) ||
			(f54->query.has_noise_mitigation2 == 1))
		reg_addr += CONTROL_63_SIZE;

	/* controls 64 65 66 67 */
	if (f54->query.has_h_blank == 1)
		reg_addr += CONTROL_64_67_SIZE * 7;
	else if ((f54->query.has_v_blank == 1) ||
			(f54->query.has_long_h_blank == 1))
		reg_addr += CONTROL_64_67_SIZE;

	/* controls 68 69 70 71 72 73 */
	if ((f54->query.has_h_blank == 1) ||
			(f54->query.has_v_blank == 1) ||
			(f54->query.has_long_h_blank == 1))
		reg_addr += CONTROL_68_73_SIZE;

	/* control 74 */
	if (f54->query.has_slew_metric == 1)
		reg_addr += CONTROL_74_SIZE;

	/* control 75 */
	if (f54->query.has_enhanced_stretch == 1)
		reg_addr += num_of_sensing_freqs;

	/* control 76 */
	if (f54->query.has_startup_fast_relaxation == 1)
		reg_addr += CONTROL_76_SIZE;

	/* controls 77 78 */
	if (f54->query.has_esd_control == 1)
		reg_addr += CONTROL_77_78_SIZE;

	/* controls 79 80 81 82 83 */
	if (f54->query.has_noise_mitigation2 == 1)
		reg_addr += CONTROL_79_83_SIZE;

	/* controls 84 85 */
	if (f54->query.has_energy_ratio_relaxation == 1)
		reg_addr += CONTROL_84_85_SIZE;

	/* control 86 */
	if ((f54->query.has_query13 == 1) && (f54->query.has_ctrl86 == 1))
		reg_addr += CONTROL_86_SIZE;

	/* control 87 */
	if ((f54->query.has_query13 == 1) && (f54->query.has_ctrl87 == 1))
		reg_addr += CONTROL_87_SIZE;

	/* control 88 */
	if (f54->query.has_ctrl88 == 1) {
		control->reg_88 = kzalloc(sizeof(*(control->reg_88)),
				GFP_KERNEL);
		if (!control->reg_88)
			goto exit_no_mem;
		control->reg_88->address = reg_addr;
		reg_addr += sizeof(control->reg_88->data);
	}

	return 0;

exit_no_mem:
	dev_err(&rmi4_data->i2c_client->dev,
			"%s: Failed to alloc mem for control registers\n",
			__func__);
	return -ENOMEM;
}

/* < DTS2014012604495 sunlibin 20140126 begin */
#ifdef MMITEST
/*To get different product and module by product-iD*/
/* < DTS2014032805796 songrongyuan 20140328 begin */
/**
 * get_of_u32_val() - get the u32 type value fro the DTSI.
 * @np: device node
 * @name: The DTSI key name
 * @default_value: If the name is not matched, return a default value.
 */
static u32 get_of_u32_val(struct device_node *np,
	const char *name,u32 default_val)
{
	u32 tmp= 0;
	int err = 0;
	err = of_property_read_u32(np, name, &tmp);
	if (!err)
		return tmp;
	else {
		tp_log_err("%s:FAILED get the data from the dtsi default_val=%d!\n",__func__,default_val);
		return default_val;
	}
}
/*To get the tp module id by the product id */
/* < DTS2014090503817 yangyang 20140922 begin */
int get_product_module_name(unsigned char *product_id)
/* DTS2014090503817 yangyang 20140922 end> */
{
	int product_module_name = UNKNOW_PRODUCT_MODULE;
	int len = strlen(product_id);

	/*To get the last three characters of product_id , if the length of product id is longer than three */
	if (len > MODULE_STR_LEN) {
		product_id += (len - MODULE_STR_LEN);
	} else {
		product_module_name = UNKNOW_PRODUCT_MODULE;
		tp_log_debug("product module is:%d\n", product_module_name);
		return product_module_name;
	}

	/*To get the tp module id , 0 is ofilm , 1 is eely ,2 is truly , 5 is junda*/
	if (!strcmp(product_id, FW_OFILM_STR))
		product_module_name = FW_OFILM;
	else if (!strcmp(product_id, FW_EELY_STR))
		product_module_name = FW_EELY;
	else if (!strcmp(product_id, FW_TRULY_STR))
		product_module_name = FW_TRULY;
	else if (!strcmp(product_id, FW_JUNDA_STR))
		product_module_name = FW_JUNDA;
	/* < DTS2014062403301 wanglongfei 20140624 begin */ 	
	else if (!strcmp(product_id, FW_GIS_STR))
		product_module_name = FW_GIS;
	else if (!strcmp(product_id, FW_YASSY_STR))
		product_module_name = FW_YASSY;
	/* DTS2014062403301 wanglongfei 20140624 end> */
	/* < DTS2014070819376 sunlibin 20140709 begin */
	/*Modify G760L tp_cap threshold get from V3*/
	else if (!strcmp(product_id, FW_LENSONE_STR))
		product_module_name = FW_LENSONE;
	/* DTS2014070819376 sunlibin 20140709 end> */
	else
		product_module_name = UNKNOW_PRODUCT_MODULE;

	tp_log_debug("product module is:%d\n", product_module_name);
	return product_module_name;
}

/*To create the array according to the node size and get the property from the dtsi*/
static u16 *create_and_get_u16_array(struct device_node *dev_node,
			const char *name, int *size)
{
	const __be32 *values;
	u16 *val_array;
	int len;
	int sz;
	int rc;
	int i;

	/*To get the property by the property name in the node*/
	values = of_get_property(dev_node, name, &len);
	if (values == NULL)
		return NULL;

	sz = len / sizeof(u32);
	tp_log_debug("%s: %s size:%d\n", __func__, name, sz);

	val_array = kzalloc(sz * sizeof(u16), GFP_KERNEL);
	if (val_array == NULL) {
		rc = -ENOMEM;
		goto fail;
	}

	for (i = 0; i < sz; i++)
		val_array[i] = (u16)be32_to_cpup(values++);

	*size = sz;

	return val_array;

fail:
	return ERR_PTR(rc);
}

/*To release the capacitance threshold memory */
static void synaptics_rmi4_f54_release_cap_limit(void)
{
	if (!f54)
		return;

	if (f54->f54_full_raw_max_cap)
		kfree(f54->f54_full_raw_max_cap);
	if (f54->f54_full_raw_min_cap)
		kfree(f54->f54_full_raw_min_cap);
	if (f54->f54_highRes_max)
		kfree(f54->f54_highRes_max);
	if (f54->f54_highRes_min)
		kfree(f54->f54_highRes_min);

	f54->f54_full_raw_max_cap = NULL;
	f54->f54_full_raw_min_cap = NULL;
	/* < DTS2014070819376 sunlibin 20140709 begin */
	/*For Probability of tp cap test crush*/
	f54->f54_highRes_max = NULL;
	/* DTS2014070819376 sunlibin 20140709 end> */
	f54->f54_highRes_min = NULL;
	f54->f54_raw_max_cap = 0;
	f54->f54_raw_min_cap = 0;
	f54->f54_rx2rx_diagonal_max= 0;
	f54->f54_rx2rx_diagonal_min = 0;
	f54->f54_rx2rx_others = 0;
	f54->f54_tx2tx_limit = 0;
	tp_log_debug("%s:release the momery,LINE = %d\n", __func__, __LINE__);

	return;
}

/*To get the default capacitance threshold value if DTSI configuration is invalid*/
static void synaptics_rmi4_f54_default_cap_limit(void)
{
	int i = 0;
	if (!f54)
		return;

	f54->f54_full_raw_max_cap = 
		kmalloc(rx * tx * sizeof(*f54->f54_full_raw_max_cap), GFP_KERNEL);
	if (!f54->f54_full_raw_max_cap) {
		goto error;
	}
	
	/*The allocated memory is 0 by default , if we use the kzalloc*/
	f54->f54_full_raw_min_cap = 
		kzalloc(rx * tx * sizeof(*f54->f54_full_raw_min_cap), GFP_KERNEL);
	if (!f54->f54_full_raw_min_cap) {
		goto error;
	}

	f54->f54_highRes_max = 
		kmalloc(F54_HIGH_RESISTANCE_DATA_NUM * sizeof(*f54->f54_highRes_max),
			GFP_KERNEL);
	if (!f54->f54_highRes_max) {
		goto error;
	}

	f54->f54_highRes_min = 
		kmalloc(F54_HIGH_RESISTANCE_DATA_NUM * sizeof(*f54->f54_highRes_min),
			GFP_KERNEL);
	if (!f54->f54_highRes_min) {
		goto error;
	}

	for (i = 0; i < tx * rx; ++i) {
		f54->f54_full_raw_max_cap[i] = MAX_CAP_LIMIT;
	}

	for (i = 0; i < F54_HIGH_RESISTANCE_DATA_NUM; ++i) {
		f54->f54_highRes_max[i] = MAX_CAP_LIMIT;
		f54->f54_highRes_min[i] = MIN_CAP_LIMIT;
	}

	f54->f54_raw_max_cap = MAX_CAP_LIMIT;
	f54->f54_raw_min_cap = 0;
	f54->f54_rx2rx_diagonal_max= RX_DIAG_ULIMIT;
	f54->f54_rx2rx_diagonal_min = RX_DIAG_LLIMIT;
	f54->f54_rx2rx_others = RX_OTHER_ULIMIT;
	f54->f54_tx2tx_limit = 0;

	/* < DTS2014082605600 s00171075 20140830 begin */
	tp_log_info("Use default cap limit!\n");
	/* DTS2014082605600 s00171075 20140830 end > */
	return;
error:
	synaptics_rmi4_f54_release_cap_limit();
	return;
}

/*To get the capacitance threshold value*/
void get_f54_get_cap_limit(struct device *dev,char *product_id,u16 ic_type)
{
	int product_module_name = UNKNOW_PRODUCT_MODULE;
	struct device_node  *np = dev->of_node;
	struct device_node *dev_node = NULL;
	int size = 0;
	int i;

	if (NULL == product_id) {
		tp_log_err("%s:product_id is NULL!\n",__func__);
		return;
	}

	/*if the value is already */
	tp_log_debug("%s: f54_raw_max_cap=%d,LINE=%d\n", __func__, f54->f54_raw_max_cap,__LINE__);
	if (f54->f54_raw_max_cap) {
		tp_log_info("%s: skipping getting data!\n",__func__);
		return;
	}

	/*To get the tp module id by the product id */
	product_module_name = get_product_module_name(product_id);
	if (product_module_name == UNKNOW_PRODUCT_MODULE) {
		tp_log_err("%s: not able to get module name = %d\n",
					__func__,product_module_name);
		/* < DTS2014042407105 sunlibin 20140504 begin */
		/* Unknow module use default limit */
		goto error;
		/* DTS2014042407105 sunlibin 20140504 end> */
	}

	/*To find the capacitancecapacitance node by the node name if ic type is 3207*/
	if (IC_TYPE_3207 == ic_type) {
		switch(product_module_name) {
		case FW_JUNDA:
			dev_node= of_find_node_by_name(np, "huawei,junda");
			break;
		case FW_OFILM:
			dev_node= of_find_node_by_name(np, "huawei,ofilm");
			break;
		case FW_TRULY:
			dev_node= of_find_node_by_name(np, "huawei,truly");
			break;
		case FW_EELY:
			dev_node= of_find_node_by_name(np, "huawei,eely");
			break;
		/* < DTS2014062403301 wanglongfei 20140624 begin */
		case FW_GIS:
			dev_node= of_find_node_by_name(np, "huawei,gis");
			break;
		case FW_YASSY:
			dev_node= of_find_node_by_name(np, "huawei,yassy");
			break;
		/* DTS2014062403301 wanglongfei 20140624 end> */
		/* < DTS2014070819376 sunlibin 20140709 begin */
		/*Modify G760L tp_cap threshold get from V3*/
		case FW_LENSONE:
			dev_node= of_find_node_by_name(np, "huawei,lensone");
			break;
		/* DTS2014070819376 sunlibin 20140709 end> */
		default:
			tp_log_err("%s: got failed,use default!\n",__func__);
			goto error;
		}
	}

	tp_log_debug("%s: product_module_name=%d,line=%d \n", __func__,product_module_name,__LINE__);
	/*To create the array and get the fullraw_upperlimit*/
	f54->f54_full_raw_max_cap =
		create_and_get_u16_array(dev_node,"huawei,fullraw_upperlimit", &size);
	if (!f54->f54_full_raw_max_cap || size != rx * tx) {
		/* BEGIN PN: DTS2014101407779,Modified by yuanbo, 2010/10/14*/
		tp_log_err("unable to read huawei,fullraw_upperlimit size=%d,rx*tx=%d\n", size, rx * tx);
		/* END   PN: DTS2014101407779,Modified by yuanbo, 2010/10/14*/
		goto error;
	}

	/*To create the array and get the fullraw_lowerlimit*/
	f54->f54_full_raw_min_cap =
		create_and_get_u16_array(dev_node,"huawei,fullraw_lowerlimit", &size);
	if (!f54->f54_full_raw_min_cap || size != rx*tx) {
		/* BEGIN PN: DTS2014101407779,Modified by yuanbo, 2010/10/14*/
		tp_log_err("unable to read huawei,fullraw_lowerlimit size=%d,rx*tx=%d\n", size, rx * tx);
		/* END PN: DTS2014101407779,Modified by yuanbo, 2010/10/14*/
		goto error;
	}

	/*To create the array and get the highres_upperlimit*/
	f54->f54_highRes_max =
		create_and_get_u16_array(dev_node,"huawei,highres_upperlimit", &size);
	if (!f54->f54_highRes_max || size != F54_HIGH_RESISTANCE_DATA_NUM) {
		tp_log_err("unable to read huawei,highres_upperlimit size=%d\n", size);
		goto error;
	}

	/*To create the array and get the highres_lowerlimit*/
	f54->f54_highRes_min =
		create_and_get_u16_array(dev_node,"huawei,highres_lowerlimit", &size);
	if (!f54->f54_highRes_min || size != F54_HIGH_RESISTANCE_DATA_NUM) {
		tp_log_err("unable to read huawei,highres_upperlimit size=%d\n", size);
		goto error;
	}

	for (i = 0; i < size; ++i){
		tp_log_debug("f54_highRes_min[%d]=%d \n",i,f54->f54_highRes_min[i]);
		tp_log_debug("f54_highRes_max[%d]=%d \n",i,f54->f54_highRes_max[i]);
	}

	/*To get the raw_max_cap from the dtsi*/
	f54->f54_raw_max_cap = get_of_u32_val(dev_node, "huawei,rawcap_upperlimit", MAX_CAP_LIMIT);
	tp_log_debug("%s: rawcap_upperlimit = %d\n",__func__,f54->f54_raw_max_cap);


	/*To get the raw_min_cap from the dtsi*/
	f54->f54_raw_min_cap = get_of_u32_val(dev_node, "huawei,rawcap_lowerlimit", 0);
	tp_log_debug("%s: rawcap_lowerlimit= %d\n",__func__,f54->f54_raw_min_cap);

	/*To get the rx2rx_diagonal_max from the dtsi*/
	f54->f54_rx2rx_diagonal_max = get_of_u32_val(dev_node, "huawei,rxdiagonal_upperlimit", RX_DIAG_ULIMIT);
	tp_log_debug("%s: rxdiagonal_upperlimit = %d\n",__func__,f54->f54_rx2rx_diagonal_max);

	/*To get the rx2rx_diagonal_min from the dtsi*/
	f54->f54_rx2rx_diagonal_min = get_of_u32_val(dev_node, "huawei,rxdiagonal_lowerlimit", RX_DIAG_LLIMIT);
	tp_log_debug("%s: rxdiagonal_lowerlimit = %d\n",__func__,f54->f54_rx2rx_diagonal_min);

	/*To get the rxothers_upperlimit from the dtsi*/
	f54->f54_rx2rx_others = get_of_u32_val(dev_node, "huawei,rxothers_upperlimit", RX_OTHER_ULIMIT);
	tp_log_debug("%s: rxothers_upperlimit = %d\n",__func__,f54->f54_rx2rx_others);

	/*To get the txtxreport_limit from the dtsi*/
	f54->f54_tx2tx_limit = get_of_u32_val(dev_node, "huawei,txtxreport_limit", 0);
	tp_log_debug("%s: txtxreport_limit= %d\n",__func__,f54->f54_tx2tx_limit);

	return;
error:
	/*To use the default capacitance limit if getting the data from the dtsi*/
	tp_log_err("DTSI configuration is invalid\n");
	synaptics_rmi4_f54_release_cap_limit();
	synaptics_rmi4_f54_default_cap_limit();
	return;
}
/* DTS2014032805796 songrongyuan 20140328 end > */
#endif/*MMITEST*/
/* DTS2014012604495 sunlibin 20140126 end> */

static void synaptics_rmi4_f54_status_work(struct work_struct *work)
{
	int retval;
	/* < DTS2014042407105 sunlibin 20140504 begin */
	int i = 0;
	unsigned int report_times_max = 0;
	unsigned int report_size_temp = MAX_I2C_MSG_LENS;
	unsigned char *report_data_temp = NULL;
	/* DTS2014042407105 sunlibin 20140504 end> */
	unsigned char report_index[2];
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
/* < DTS2014012604495 sunlibin 20140126 begin */
#ifdef MMITEST
	u16 ic_type = IC_TYPE_3207;
#endif/*MMITEST*/
/* DTS2014012604495 sunlibin 20140126 end> */

	tp_log_debug("%s:in!\n", __func__);
	if (f54->status != STATUS_BUSY)
		return;

	set_report_size();
	if (f54->report_size == 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Report data size = 0\n",
				__func__);
		retval = -EINVAL;
		goto error_exit;
	}

	if (f54->data_buffer_size < f54->report_size) {
		mutex_lock(&f54->data_mutex);
		if (f54->data_buffer_size)
			kfree(f54->report_data);
		f54->report_data = kzalloc(f54->report_size, GFP_KERNEL);
		if (!f54->report_data) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to alloc mem for data buffer\n",
					__func__);
			f54->data_buffer_size = 0;
			mutex_unlock(&f54->data_mutex);
			retval = -ENOMEM;
			goto error_exit;
		}
		f54->data_buffer_size = f54->report_size;
		mutex_unlock(&f54->data_mutex);
	}

	/* < DTS2014042407105 sunlibin 20140504 begin */
	/* Msm8916 i2c transfer max block len is 256bytes, */
	/* so we need to transfer every 256 bytes when meaasge len>256 */
	report_times_max = f54->report_size/MAX_I2C_MSG_LENS;
	if(f54->report_size%MAX_I2C_MSG_LENS != 0)
	{
		report_times_max += 1;
	}

	report_index[0] = 0; //ReportIndexLSB
	report_index[1] = 0; //ReportIndexMSB

	/* Read the same register,internal FIFO will send data by byte. */
	retval = f54->fn_ptr->write(rmi4_data,
			f54->data_base_addr + DATA_REPORT_INDEX_OFFSET,
			report_index,
			sizeof(report_index));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to write report data index\n",
				__func__);
		retval = -EINVAL;
		goto error_exit;
	}
	/* < DTS2014080804645 caowei 201400808 begin */
	tp_log_info("%s %d:report_times_max = %d, report_size_temp = %d\n", 
				__func__, __LINE__, report_times_max, report_size_temp);
	mutex_lock(&f54->data_mutex);
	/* Point to the block data about to transfer */
	report_data_temp = f54->report_data;
	for(i = 0;i < report_times_max;i ++)
	{

		if(i == (report_times_max - 1))
		{
			/* The last time transfer the rest of the block data */
			report_size_temp = f54->report_size%MAX_I2C_MSG_LENS;
			
			tp_log_info("%s %d:report_size_temp = %d\n", __func__, __LINE__, 
				report_size_temp);
		}

		tp_log_info("%s %d:loop i = %d\n", __func__, __LINE__, i);
		retval = f54->fn_ptr->read(rmi4_data,
				f54->data_base_addr + DATA_REPORT_DATA_OFFSET,
				report_data_temp,
				report_size_temp);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to read report data\n",
					__func__);
			retval = -EINVAL;
			mutex_unlock(&f54->data_mutex);
			goto error_exit;
		}
		/* Point to the next 256bytes data */
		report_data_temp += MAX_I2C_MSG_LENS;
	}
	mutex_unlock(&f54->data_mutex);
	/* DTS2014042407105 sunlibin 20140504 end> */
	/* DTS2014080804645 caowei 201400808 end >*/
/* < DTS2014012604495 sunlibin 20140126 begin */
#ifdef MMITEST
/* < DTS2014032805796 songrongyuan 20140328 begin */
	tp_log_debug("%s: start mmi and get capacitance threshold value line=%d \n",__func__,__LINE__);
	get_f54_get_cap_limit(&rmi4_data->i2c_client->dev,rmi4_data->product_id,ic_type);
/* DTS2014032805796 songrongyuan 20140328 end > */
#endif/*MMITEST*/
/* DTS2014012604495 sunlibin 20140126 end> */
	retval = STATUS_IDLE;

#ifdef RAW_HEX
	/* < DTS2014042407105 sunlibin 20140504 begin */
	/* Open debug log */
	if( synaptics_dsx_debug_mask >= TP_DBG )
	{
		print_raw_hex_report();
	}
	/* DTS2014042407105 sunlibin 20140504 end> */
#endif

#ifdef HUMAN_READABLE
	print_image_report();
#endif

error_exit:
	mutex_lock(&f54->status_mutex);
	f54->status = retval;
	mutex_unlock(&f54->status_mutex);

	return;
}

static void synaptics_rmi4_f54_set_regs(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count,
		unsigned char page)
{
	unsigned char ii;
	unsigned char intr_offset;

	f54->query_base_addr = fd->query_base_addr | (page << 8);
	f54->control_base_addr = fd->ctrl_base_addr | (page << 8);
	f54->data_base_addr = fd->data_base_addr | (page << 8);
	f54->command_base_addr = fd->cmd_base_addr | (page << 8);

	f54->intr_reg_num = (intr_count + 7) / 8;
	if (f54->intr_reg_num != 0)
		f54->intr_reg_num -= 1;

	f54->intr_mask = 0;
	intr_offset = intr_count % 8;
	for (ii = intr_offset;
			ii < ((fd->intr_src_count & MASK_3BIT) +
			intr_offset);
			ii++) {
		f54->intr_mask |= 1 << ii;
	}

	return;
}

static void synaptics_rmi5_f55_init(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ii;
	unsigned char rx_electrodes = f54->query.num_of_rx_electrodes;
	unsigned char tx_electrodes = f54->query.num_of_tx_electrodes;

	retval = f54->fn_ptr->read(rmi4_data,
			f55->query_base_addr,
			f55->query.data,
			sizeof(f55->query.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read f55 query registers\n",
				__func__);
		return;
	}

	if (!f55->query.has_sensor_assignment)
		return;

	f55->rx_assignment = kzalloc(rx_electrodes, GFP_KERNEL);
	f55->tx_assignment = kzalloc(tx_electrodes, GFP_KERNEL);

	retval = f54->fn_ptr->read(rmi4_data,
			f55->control_base_addr + SENSOR_RX_MAPPING_OFFSET,
			f55->rx_assignment,
			rx_electrodes);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read f55 rx assignment\n",
				__func__);
		return;
	}

	retval = f54->fn_ptr->read(rmi4_data,
			f55->control_base_addr + SENSOR_TX_MAPPING_OFFSET,
			f55->tx_assignment,
			tx_electrodes);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read f55 tx assignment\n",
				__func__);
		return;
	}

	f54->rx_assigned = 0;
	for (ii = 0; ii < rx_electrodes; ii++) {
		if (f55->rx_assignment[ii] != 0xff)
			f54->rx_assigned++;
	}

	f54->tx_assigned = 0;
	for (ii = 0; ii < tx_electrodes; ii++) {
		if (f55->tx_assignment[ii] != 0xff)
			f54->tx_assigned++;
	}
	/* BEGIN PN: DTS2014101407779,Added by yuanbo, 2010/10/14*/
	tp_log_info("%s: line(%d) f54->rx_assigned =%d.f54->tx_assigned=%d\n",__func__,__LINE__,f54->rx_assigned,f54->tx_assigned);
	/* END   PN: DTS2014101407779,Added by yuanbo, 2010/10/14*/

	return;
}

static void synaptics_rmi4_f55_set_regs(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned char page)
{
	f55 = kzalloc(sizeof(*f55), GFP_KERNEL);
	if (!f55) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for f55\n",
				__func__);
		return;
	}

	f55->query_base_addr = fd->query_base_addr | (page << 8);
	f55->control_base_addr = fd->ctrl_base_addr | (page << 8);
	f55->data_base_addr = fd->data_base_addr | (page << 8);
	f55->command_base_addr = fd->cmd_base_addr | (page << 8);

	return;
}

static void synaptics_rmi4_f54_attn(struct synaptics_rmi4_data *rmi4_data,
		unsigned char intr_mask)
{
	if (!f54)
		return;

	if (f54->intr_mask & intr_mask) {
		queue_delayed_work(f54->status_workqueue,
				&f54->status_work,
				msecs_to_jiffies(STATUS_WORK_INTERVAL));
	}

	return;
}

/* < DTS2014072402403 yangyang 20140805 begin */
static int synaptics_rmi4_f54_init(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned short ii;
	unsigned char page;
	unsigned char intr_count = 0;
	bool f54found = false;
	bool f55found = false;
	struct synaptics_rmi4_fn_desc rmi_fd;
	int pdt_retry=3; /* used to retry read register to get the right value of f54/f55 */
	/* < DTS2014053000677 sunlibin 20140530 begin */
	/*Sync DTS2014051400740 from 8926,in case mmi fail */
	retval = add_synaptics_mmi_test_interfaces();
	if (retval < 0) {
		tp_log_err( "%s: Error, synaptics_mmi_test init sysfs fail! \n",
			__func__);
		goto exit;
	}
	/* DTS2014053000677 sunlibin 20140530 end> */

	/* < DTS2014042407105 sunlibin 20140504 begin */
	/* In flash program mode,no need to do init for it must be failed. */
	if (rmi4_data->flash_prog_mode) {
		retval = -EINVAL;
		tp_log_err("%s: line(%d)Jump init in prog mode,ret=%d \n", __func__,__LINE__,retval);
		goto exit;
	}
	/* DTS2014042407105 sunlibin 20140504 end> */

	f54 = kzalloc(sizeof(*f54), GFP_KERNEL);
	if (!f54) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for f54\n",
				__func__);
		retval = -ENOMEM;
		goto exit;
	}

	f54->fn_ptr = kzalloc(sizeof(*(f54->fn_ptr)), GFP_KERNEL);
	if (!f54->fn_ptr) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for fn_ptr\n",
				__func__);
		retval = -ENOMEM;
		goto exit_free_f54;
	}

	f54->rmi4_data = rmi4_data;
	f54->fn_ptr->read = rmi4_data->i2c_read;
	f54->fn_ptr->write = rmi4_data->i2c_write;
	f54->fn_ptr->enable = rmi4_data->irq_enable;
init_f54_f55:
	for (page = 0; page < PAGES_TO_SERVICE; page++) {
		for (ii = PDT_START; ii > PDT_END; ii -= PDT_ENTRY_SIZE) {
			ii |= (page << 8);

			retval = f54->fn_ptr->read(rmi4_data,
					ii,
					(unsigned char *)&rmi_fd,
					sizeof(rmi_fd));
			if (retval < 0) {
				/* < DTS2014082605600 s00171075 20140830 begin */
				tp_log_err("%s: Read addr(0x%04x) error!\n",
					__func__,ii);
				/* DTS2014082605600 s00171075 20140830 end > */
				goto exit_free_mem;
			}

			/* < DTS2014081200136 caowei 201400812 begin */
			if (!rmi_fd.fn_number) {
				/* < DTS2014082605600 s00171075 20140830 begin */
				/*Base addr for F54=0x01E9/F55=0x03E9,both from 0xE9 */
 				if ((F54_QUERY_BASE == ii) || (F55_QUERY_BASE == ii)) 
				{
					tp_log_info( "%s: Read addr(0x%04x) fn_number is zero!\n",
						__func__,ii);
/* < DTS2015012105205  zhoumin wx222300 20150121 begin */
#ifdef CONFIG_HUAWEI_DSM
					synp_tp_report_dsm_err(DSM_TP_I2C_RW_ERROR_NO, retval);
#endif
/* DTS2015012105205  zhoumin wx222300 20150121 end > */
				}
				/* DTS2014082605600 s00171075 20140830 end > */
				break;
			}
			/* DTS2014081200136 caowei 201400812 end >*/

			switch (rmi_fd.fn_number) {
				case SYNAPTICS_RMI4_F54:
					synaptics_rmi4_f54_set_regs(rmi4_data,
							&rmi_fd, intr_count, page);
					f54found = true;
					break;
				case SYNAPTICS_RMI4_F55:
					synaptics_rmi4_f55_set_regs(rmi4_data,
							&rmi_fd, page);
					f55found = true;
					break;
				default:
					break;
			}

			/* < DTS2014082605600 s00171075 20140830 begin */
			/* if f54 and f55 is abnormal, end loop to trace error */
			if (((F54_QUERY_BASE == ii) && !f54found) || 
				((F55_QUERY_BASE == ii) && !f55found)) 
			{
				goto pdt_done;
			}
			/* DTS2014082605600 s00171075 20140830 end > */

			if (f54found && f55found) {
				tp_log_debug("%s#%d f54&f55 found.\n",__func__, __LINE__);
				goto pdt_done;
			}

			intr_count += (rmi_fd.intr_src_count & MASK_3BIT);
		}
	}
pdt_done:
	pdt_retry--;
	/* if f54found or f55found is not set, print the register data, and retry to read the register */
	if (!f54found || !f55found) {
		/* < DTS2014082605600 s00171075 20140830 begin */
		g_rmi_fd = &rmi_fd;

		tp_log_err("%s#line %d#\n",__func__,__LINE__);
		tp_log_err("%s page:0x%02x\n",__func__,page);
		tp_log_err("%s addr:0x%02x\n",__func__,ii);
		tp_log_err("%s sizeof(rmi_fd):%d\n",__func__,sizeof(rmi_fd));
		tp_log_err("%s struct synaptics_rmi4_fn_desc{\n",__func__);
		tp_log_err("%s query_base_addr       :0x%02x\n",__func__,rmi_fd.query_base_addr);
		tp_log_err("%s cmd_base_addr         :0x%02x\n",__func__,rmi_fd.cmd_base_addr);
		tp_log_err("%s ctrl_base_addr        :0x%02x\n",__func__,rmi_fd.ctrl_base_addr);
		tp_log_err("%s data_base_addr        :0x%02x\n",__func__,rmi_fd.data_base_addr);
		tp_log_err("%s intr_src_count(3)     :0x%02x\n",__func__,rmi_fd.intr_src_count);
		tp_log_err("%s fn_number             :0x%02x\n",__func__,rmi_fd.fn_number);
		tp_log_err("%s }\n",__func__);
		/* DTS2014082605600 s00171075 20140830 end > */

		/* < DTS2014042407105 sunlibin 20140504 begin */
		/* <DTS2014071505823 wwx203500 20142407 begin */
		tp_log_err("%s: line(%d) PDT scan Error f54found:%d f55found:%d\n", __func__,__LINE__,f54found,f55found);
		/* retry to read the register to get the right value */
		if (pdt_retry) {
			tp_log_err("%s: retry(%d) to scan pdt \n",__func__,pdt_retry);
			goto init_f54_f55;
		}
		/* DTS2014071505823 wwx203500 20142407 end> */
		/* DTS2014042407105 sunlibin 20140504 end> */
/* < DTS2015012105205  zhoumin wx222300 20150121 begin */
#ifdef CONFIG_HUAWEI_DSM
		/* < DTS2014082605600 s00171075 20140830 begin */
		/* report to device_monitor */
		synp_tp_report_dsm_err(DSM_TP_F54_PDT_ERROR_NO, retval);
		/* DTS2014082605600 s00171075 20140830 end > */
#endif
/* DTS2015012105205  zhoumin wx222300 20150121 end > */
		tp_log_err("%s: line(%d) f54/f55 NOT found. Error.\n",__func__,__LINE__);
		retval = -ENODEV;
		goto exit_free_mem;
	} else {
		tp_log_info("%s: line(%d) f54/f55 found.\n",__func__,__LINE__);
	}
	retval = f54->fn_ptr->read(rmi4_data,
			f54->query_base_addr,
			f54->query.data,
			sizeof(f54->query.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read f54 query registers\n",
				__func__);
		goto exit_free_mem;
	}
	/* BEGIN   PN: DTS2014101407779,Added by yuanbo, 2010/10/14*/
	tp_log_info("%s: line(%d) num_of_rx_electrodes=%d.num_of_tx_electrodes=%d\n",__func__,__LINE__,f54->query.num_of_rx_electrodes, f54->query.num_of_tx_electrodes);
	/* END   PN: DTS2014101407779,Added by yuanbo, 2010/10/14*/

	f54->rx_assigned = f54->query.num_of_rx_electrodes;
	f54->tx_assigned = f54->query.num_of_tx_electrodes;

	retval = synaptics_rmi4_f54_set_ctrl();
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to set up f54 control registers\n",
				__func__);
		goto exit_free_control;
	}

	if (f55found)
		synaptics_rmi5_f55_init(rmi4_data);

	mutex_init(&f54->status_mutex);
	mutex_init(&f54->data_mutex);
	mutex_init(&f54->control_mutex);
/* < DTS2013121708848 sunlibin 20131217 begin */
/*Add synaptics capacitor test function */
#if 1
	/* < DTS2014042407105 sunlibin 20140504 begin */
	tp_log_info("%s: sysfs_is_busy = %d\n", __func__,sysfs_is_busy);
	/* DTS2014042407105 sunlibin 20140504 end> */
	if (!sysfs_is_busy) {
		retval = synaptics_rmi4_f54_set_sysfs();
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to create sysfs entries\n",
					__func__);
			goto exit_sysfs;
		}
		sysfs_is_busy = false;
	}
#endif
/* DTS2013121708848 sunlibin 20131217 end >*/
	f54->status_workqueue =
			create_singlethread_workqueue("f54_status_workqueue");
	/* < DTS2014120402835 songrongyuan 20141210 begin */
	if(NULL == f54->status_workqueue){
		tp_log_err("%s: failed to create workqueue for f54 status\n", __func__);
		retval = -ENOMEM;
		goto exit_sysfs;
	}
	/* DTS2014120402835 songrongyuan 20141210 end > */
	INIT_DELAYED_WORK(&f54->status_work,
			synaptics_rmi4_f54_status_work);

#ifdef WATCHDOG_HRTIMER
	/* Watchdog timer to catch unanswered get report commands */
	hrtimer_init(&f54->watchdog, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	f54->watchdog.function = get_report_timeout;

	/* Work function to do actual cleaning up */
	INIT_WORK(&f54->timeout_work, timeout_set_status);
#endif

	f54->status = STATUS_IDLE;

	return 0;

exit_sysfs:
	if (NULL != f55) {
		kfree(f55->rx_assignment);
		kfree(f55->tx_assignment);
	}

exit_free_control:
	free_control_mem();

exit_free_mem:
	kfree(f55);
	kfree(f54->fn_ptr);

exit_free_f54:
	kfree(f54);
	f54 = NULL;

exit:
	return retval;
}
/* DTS2014072402403 yangyang 20140805 end > */

/* < DTS2014072402403 yangyang 20140805 begin */
static void synaptics_rmi4_f54_remove(struct synaptics_rmi4_data *rmi4_data)
{
	tp_log_debug("%s#%d\n",__func__, __LINE__);
	if (!f54) {
		tp_log_err("%s#%d checked f54 = NULL\n",__func__, __LINE__);
		goto exit;
	}

#ifdef WATCHDOG_HRTIMER
	hrtimer_cancel(&f54->watchdog);
#endif

	cancel_delayed_work_sync(&f54->status_work);
	flush_workqueue(f54->status_workqueue);
	destroy_workqueue(f54->status_workqueue);

/* < DTS2013121708848 sunlibin 20131217 begin */
/*Add synaptics capacitor test function */
	if (!sysfs_is_busy) {
		tp_log_warning("%s#%d sysfs_is_busy:%d\n",__func__, __LINE__,sysfs_is_busy);
		remove_sysfs();
	}
/* DTS2013121708848 sunlibin 20131217 end >*/

	if (NULL != f55) {
		tp_log_warning("%s#%d checked f55 != NULL\n",__func__, __LINE__);
		kfree(f55->rx_assignment);
		kfree(f55->tx_assignment);
	}

	free_control_mem();

	kfree(f55);
	f55 = NULL;

	if (f54->data_buffer_size)
		kfree(f54->report_data);

	kfree(f54->fn_ptr);
/* < DTS2014032805796 songrongyuan 20140328 begin */
#ifdef MMITEST
	tp_log_debug("%s:release the capacitance momery,LINE = %d\n", __func__, __LINE__);
	synaptics_rmi4_f54_release_cap_limit();
#endif
/* DTS2014032805796 songrongyuan 20140328 end > */
	kfree(f54);
	f54 = NULL;
	
/* < DTS2013123006678 zhangmin 20131230 begin */
/*move free action to module_exit,because the cap test
 is complexity,I change this to other place,but panic always
 happened,so move to module exit for safety*/
/* DTS2013123006678 zhangmin 20131230 end > */
exit:
	complete(&f54_remove_complete);

	return;
}
/* DTS2014072402403 yangyang 20140805 end > */

static void synaptics_rmi4_f54_reset(struct synaptics_rmi4_data *rmi4_data)
{
	/* < DTS2014042407105 sunlibin 20140504 begin */
	/* In flash program mode,f54 will always be NULL and cause f54 reset fail. */
	/* <DTS2014071505823 wwx203500 20142407 begin */
	tp_log_warning("%s: line(%d)\n", __func__,__LINE__);
	/* DTS2014071505823 wwx203500 20142407 end> */
	/* DTS2014042407105 sunlibin 20140504 end> */

	synaptics_rmi4_f54_remove(rmi4_data);
	synaptics_rmi4_f54_init(rmi4_data);

	return;
}

static struct synaptics_rmi4_exp_fn f54_module = {
	.fn_type = RMI_F54,
	.init = synaptics_rmi4_f54_init,
	.remove = synaptics_rmi4_f54_remove,
	.reset = synaptics_rmi4_f54_reset,
	.reinit = NULL,
	.early_suspend = NULL,
	.suspend = NULL,
	.resume = NULL,
	.late_resume = NULL,
	.attn = synaptics_rmi4_f54_attn,
};

static int __init rmi4_f54_module_init(void)
{
	synaptics_rmi4_new_func(&f54_module, true);

	return 0;
}

static void __exit rmi4_f54_module_exit(void)
{
	synaptics_rmi4_new_func(&f54_module, false);
	/* < DTS2013123006678 zhangmin 20131230 begin */
	if (g_mmi_buf_f54test_result)
		kfree(g_mmi_buf_f54test_result);
	if (g_mmi_buf_f54raw_data)
		kfree(g_mmi_buf_f54raw_data);
	if (g_mmi_highresistance_report)
		kfree(g_mmi_highresistance_report);
	if (g_mmi_maxmincapacitance_report)
		kfree(g_mmi_maxmincapacitance_report);
	if (g_mmi_RxtoRxshort_report)
		kfree(g_mmi_RxtoRxshort_report);
	/* DTS2013123006678 zhangmin 20131230 end > */
/* < DTS2014032805796 songrongyuan 20140328 begin */
#ifdef MMITEST
	synaptics_rmi4_f54_release_cap_limit();
#endif
/* DTS2014032805796 songrongyuan 20140328 end > */
	wait_for_completion(&f54_remove_complete);

	return;
}

module_init(rmi4_f54_module_init);
module_exit(rmi4_f54_module_exit);

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("Synaptics DSX Test Reporting Module");
MODULE_LICENSE("GPL v2");
/* DTS2014042402686 sunlibin 20140424 end> */

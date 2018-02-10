/* Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#define pr_fmt(fmt)	"CHG: %s: " fmt, __func__

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/spmi.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <linux/power_supply.h>
#include <linux/qpnp/qpnp-adc.h>
#include <linux/alarmtimer.h>
#include <linux/bitops.h>
/* < DTS2014092405372 gwx199358 20140924 begin */
#ifdef CONFIG_HUAWEI_KERNEL
#include <linux/huawei_boardid.h>
#include <soc/qcom/socinfo.h>
#endif
/* DTS2014092405372 gwx199358 20140924 end > */
/* <DTS2014042804721 chenyuanquan 20140428 begin */
#ifdef CONFIG_HUAWEI_KERNEL
#include <linux/of_batterydata.h>
/* <DTS2014070819388 zhaoxiaoli 20140711 begin */
#include<linux/wakelock.h>
/* DTS2014070819388 zhaoxiaoli 20140711 end> */
#endif
/* DTS2014042804721 chenyuanquan 20140428 end> */
/* DTS2014072806213 by l00220156 for factory interfaces 20140728 begin */
#ifdef CONFIG_HUAWEI_KERNEL
    #define MAX_SHOW_STRING_LENTH (40)
    #include <linux/equip.h>
#endif
/* DTS2014072806213 by l00220156 for factory interfaces 20140728 end */
/*<DTS2014053001058 jiangfei 20140530 begin */
#ifdef CONFIG_HUAWEI_DSM
#include <linux/dsm_pub.h>
#endif
/* DTS2014053001058 jiangfei 20140530 end> */
/* < DTS2014052901670 zhaoxiaoli 20140529 begin */
#ifdef CONFIG_HUAWEI_KERNEL
static struct qpnp_lbc_chip *global_chip;
#endif
/* DTS2014052901670 zhaoxiaoli 20140529 end> */
/*<DTS2014070404260 liyuping 20140704 begin */
#ifdef CONFIG_HUAWEI_KERNEL
#include <linux/power/bq24152_charger.h>
#endif
/* DTS2014070404260 liyuping 20140704 end> */
/* <DTS2014071001904 jiangfei 20140710 begin */
#ifdef CONFIG_HW_FEATURE_STORAGE_DIAGNOSE_LOG
#include <linux/store_log.h>
#endif
/* DTS2014071001904 jiangfei 20140710 end> */
#define DSM_CHARGER_NOT_CHARGE_ERROR_NO  			(10200)
#define DSM_CHARGER_OTG_OCP_ERROR_NO  				(10201)
#define DSM_CHARGER_CHG_OVP_ERROR_NO  				(10202)
#define DSM_CHARGER_BATT_PRES_ERROR_NO  			(10203)
#define DSM_CHARGER_SPMI_ABNORMAL_ERROR_NO  		(10204)
#define CREATE_MASK(NUM_BITS, POS) \
	((unsigned char) (((1 << (NUM_BITS)) - 1) << (POS)))
#define LBC_MASK(MSB_BIT, LSB_BIT) \
	CREATE_MASK(MSB_BIT - LSB_BIT + 1, LSB_BIT)

/* Interrupt offsets */
#define INT_RT_STS_REG				0x10
#define FAST_CHG_ON_IRQ                         BIT(5)
#define OVERTEMP_ON_IRQ				BIT(4)
#define BAT_TEMP_OK_IRQ                         BIT(1)
#define BATT_PRES_IRQ                           BIT(0)

/* USB CHARGER PATH peripheral register offsets */
#define USB_PTH_STS_REG				0x09
#define USB_IN_VALID_MASK			LBC_MASK(7, 6)
#define USB_SUSP_REG				0x47
#define USB_SUSPEND_BIT				BIT(0)

/* CHARGER peripheral register offset */
#define CHG_OPTION_REG				0x08
#define CHG_OPTION_MASK				BIT(7)
#define CHG_STATUS_REG				0x09
#define CHG_VDD_LOOP_BIT			BIT(1)
#define CHG_VDD_MAX_REG				0x40
#define CHG_VDD_SAFE_REG			0x41
#define CHG_IBAT_MAX_REG			0x44
#define CHG_IBAT_SAFE_REG			0x45
#define CHG_VIN_MIN_REG				0x47
#define CHG_CTRL_REG				0x49
#define CHG_ENABLE				BIT(7)
#define CHG_FORCE_BATT_ON			BIT(0)
#define CHG_EN_MASK				(BIT(7) | BIT(0))
#define CHG_FAILED_REG				0x4A
#define CHG_FAILED_BIT				BIT(7)
#define CHG_VBAT_WEAK_REG			0x52
#define CHG_IBATTERM_EN_REG			0x5B
#define CHG_USB_ENUM_T_STOP_REG			0x4E
#define CHG_TCHG_MAX_EN_REG			0x60
#define CHG_TCHG_MAX_EN_BIT			BIT(7)
#define CHG_TCHG_MAX_MASK			LBC_MASK(6, 0)
#define CHG_TCHG_MAX_REG			0x61
#define CHG_WDOG_EN_REG				0x65
#define CHG_PERPH_RESET_CTRL3_REG		0xDA
#define CHG_COMP_OVR1				0xEE
#define CHG_VBAT_DET_OVR_MASK			LBC_MASK(1, 0)
#define OVERRIDE_0				0x2
#define OVERRIDE_NONE				0x0

/* BATTIF peripheral register offset */
#define BAT_IF_PRES_STATUS_REG			0x08
#define BATT_PRES_MASK				BIT(7)
#define BAT_IF_TEMP_STATUS_REG			0x09
#define BATT_TEMP_HOT_MASK			BIT(6)
#define BATT_TEMP_COLD_MASK			LBC_MASK(7, 6)
#define BATT_TEMP_OK_MASK			BIT(7)
#define BAT_IF_VREF_BAT_THM_CTRL_REG		0x4A
#define VREF_BATT_THERM_FORCE_ON		LBC_MASK(7, 6)
#define VREF_BAT_THM_ENABLED_FSM		BIT(7)
#define BAT_IF_BPD_CTRL_REG			0x48
#define BATT_BPD_CTRL_SEL_MASK			LBC_MASK(1, 0)
#define BATT_BPD_OFFMODE_EN			BIT(3)
#define BATT_THM_EN				BIT(1)
#define BATT_ID_EN				BIT(0)
#define BAT_IF_BTC_CTRL				0x49
#define BTC_COMP_EN_MASK			BIT(7)
#define BTC_COLD_MASK				BIT(1)
#define BTC_HOT_MASK				BIT(0)

/* MISC peripheral register offset */
#define MISC_REV2_REG				0x01
#define MISC_BOOT_DONE_REG			0x42
#define MISC_BOOT_DONE				BIT(7)
#define MISC_TRIM3_REG				0xF3
#define MISC_TRIM3_VDD_MASK			LBC_MASK(5, 4)
#define MISC_TRIM4_REG				0xF4
#define MISC_TRIM4_VDD_MASK			BIT(4)

#define PERP_SUBTYPE_REG			0x05
#define SEC_ACCESS                              0xD0

/* Linear peripheral subtype values */
#define LBC_CHGR_SUBTYPE			0x15
#define LBC_BAT_IF_SUBTYPE			0x16
#define LBC_USB_PTH_SUBTYPE			0x17
#define LBC_MISC_SUBTYPE			0x18

#define QPNP_CHG_I_MAX_MIN_90                   90

/* Feature flags */
#define VDD_TRIM_SUPPORTED			BIT(0)

/* <DTS2014051601896 jiangfei 20140516 begin */
/* <DTS2014061002202 jiangfei 20140610 begin */
/* <DTS2014071001904 jiangfei 20140710 begin */
/*<DTS2014072501338 jiangfei 20140725 begin */
#ifdef CONFIG_HUAWEI_KERNEL
/*Running test result*/
/*And charge abnormal info*/
#define CHARGE_STATUS_FAIL 	(0<<0) //Indicate running test charging status fail
#define CHARGE_STATUS_PASS 	(1<<0) //Indicate running test charging status pass
#define BATTERY_FULL 			(1<<1)
#define USB_NOT_PRESENT 		(1<<2)
#define REGULATOR_BOOST		(1<<3)
#define CHARGE_LIMIT			(1<<4)
#define BATTERY_HEALTH 		(1<<5)
#define CHARGER_OVP			(1<<6)
#define OCP_ABNORML			(1<<7)
#define BATTERY_VOL_ABNORML	(1<<8)
#define BATTERY_TEMP_ABNORML	(1<<9)
#define BATTERY_ABSENT			(1<<10)
#define BQ24152_FATAL_FAULT		(1<<11)

#define CHARGE_OCP_THR	-2500000 //charge current abnormal threshold
#define BATTERY_OCP_THR 	5000000 //discharge current abnormal threshold
#define BATTERY_VOL_THR_HI	4500000 //battery voltage abnormal high threshold
#define BATTERY_VOL_THR_LO	2500000 //battery voltage abnormal low threshold
#define BATTERY_TEMP_HI	780 //battery high temp threshold
#define BATTERY_TEMP_LO	-100 //battery low temp threshold
#define WARM_VOL_BUFFER	100 //cfg_warm_bat_mv need have a 100mV buffer
/* <DTS2014070701815 jiangfei 20140707 begin */
/* <DTS2014080801320 jiangfei 20140808 begin */
#define WARM_TEMP_THR		390 //battery warm temp threshold for running test
/* DTS2014080801320 jiangfei 20140808 end> */
/* DTS2014070701815 jiangfei 20140707 end> */
#define HOT_TEMP_THR		600 //battery hot temp threshold for running test
#define BATT_FULL			100 //battery full capactiy
#define USB_VALID_MASK		0xC0
#define USB_VALID_OVP_VALUE    0x40
#define ERROR_VBUS_OVP		1
#define ERROR_BATT_OVP		4
#define ERROR_THERMAL_SHUTDOWN		5
#define PASS_MASK			0x1E    //Running test pass mask

/*<DTS2014121703947  xiongxi xwx234328 20141217 begin*/
#define ERROR_CHARGE_FAULT	16
#define ERROR_BAT_FAULT		8
/*DTS2014121703947  xiongxi xwx234328 20141217 end>*/

#define INT_RT_STS(base)			(base + 0x10)
#define LOG_BUF_LENGTH				128
#define NOT_CHARGE_COUNT		3
/* <DTS2014073007208 jiangfei 20140801 begin */
#define USBIN_VALID_IRQ			BIT(1)
#define CHG_VIN_MIN_LOOP_BIT			BIT(3)
/*<DTS2014080807172 jiangfei 20140813 begin */
/*<DTS2014090201517 jiangfei 20140902 begin */
#define WARM_TEMP_BUFFER		30
#define COLD_HOT_TEMP_BUFFER		30
#define BQ2415X_REG_STATUS		0x00
#define BQ2415X_REG_CURRENT		0x04
/* DTS2014090201517 jiangfei 20140902 end> */
/* DTS2014080807172 jiangfei 20140813 end> */
#endif
/* DTS2014073007208 jiangfei 20140801 end> */
/* DTS2014072501338 jiangfei 20140725 end> */
/* DTS2014071001904 jiangfei 20140710 end> */
/* DTS2014061002202 jiangfei 20140610 end> */
/* DTS2014051601896 jiangfei 20140516 end> */

/*<DTS2014053001058 jiangfei 20140530 begin */
/* <DTS2014070701815 jiangfei 20140707 begin */
#ifdef CONFIG_HUAWEI_DSM
#define HOT_TEMP_BUFFER	10
#define CHECKING_TIME	30000
#define DELAY_TIME		5000
#endif
/* DTS2014070701815 jiangfei 20140707 end> */
/* DTS2014053001058 jiangfei 20140530 end> */
/* <DTS2014070819388 zhaoxiaoli 20140711 begin */
#ifdef CONFIG_HUAWEI_KERNEL
#define LED_CHECK_PERIOD_MS		3000
#endif
/* DTS2014070819388 zhaoxiaoli 20140711 end> */

/* < DTS2014092502339 zhaoxiaoli 20140929 begin */
#ifdef CONFIG_HUAWEI_KERNEL
#define BATTERY_VOL_ADJ_THR_UV    4200000
#define HW_VINMIN_ADJ_THR_MV      4283
#define HW_VINMIN_DEFAULT_MV      4389
#define VINMIN_ADJUST_MAX_COUNT   10
#endif
/* DTS2014092502339 zhaoxiaoli 20140929 end> */
#define QPNP_CHARGER_DEV_NAME	"qcom,qpnp-linear-charger"

/* usb_interrupts */

struct qpnp_lbc_irq {
	int		irq;
	unsigned long	disabled;
	bool            is_wake;
};

enum {
	USBIN_VALID = 0,
	USB_OVER_TEMP,
	USB_CHG_GONE,
	BATT_PRES,
	BATT_TEMPOK,
	CHG_DONE,
	CHG_FAILED,
	CHG_FAST_CHG,
	CHG_VBAT_DET_LO,
	MAX_IRQS,
};

enum {
	USER	= BIT(0),
	THERMAL = BIT(1),
	CURRENT = BIT(2),
	SOC	= BIT(3),
};

enum bpd_type {
	BPD_TYPE_BAT_ID,
	BPD_TYPE_BAT_THM,
	BPD_TYPE_BAT_THM_BAT_ID,
};

static const char * const bpd_label[] = {
	[BPD_TYPE_BAT_ID] = "bpd_id",
	[BPD_TYPE_BAT_THM] = "bpd_thm",
	[BPD_TYPE_BAT_THM_BAT_ID] = "bpd_thm_id",
};

enum btc_type {
	HOT_THD_25_PCT = 25,
	HOT_THD_35_PCT = 35,
	COLD_THD_70_PCT = 70,
	COLD_THD_80_PCT = 80,
};

static u8 btc_value[] = {
	[HOT_THD_25_PCT] = 0x0,
	[HOT_THD_35_PCT] = BIT(0),
	[COLD_THD_70_PCT] = 0x0,
	[COLD_THD_80_PCT] = BIT(1),
};
/* DTS2014072806213 by l00220156 for factory interfaces 20140728 begin */
extern int bq2419x_factory_set_charging_enabled(int enable);
extern int bq2419x_equip_at_batt_charge_enable(int enable);
/* DTS2014072806213 by l00220156 for factory interfaces 20140728 end */
/* <Begin DTS2014080603839 modified by l00220156 for interface of vm and charger 20140806 > */
extern int bq2419x_get_charge_type(void);
extern int bq2419x_get_charge_status(void);
/* <End DTS2014080603839 modified by l00220156 for interface of vm and charger 20140806 > */
/* <DTS2014093003947 (report SOC 100%) caiwei 20141009 begin */
extern int bq2419x_check_charge_finished(void);
/* DTS2014093003947 (report SOC 100%) caiwei 20141009 end> */
/* <DTS2014111002912 caiwei 20141112 begin */
extern int bq2419x_otg_status_check(void);
/* DTS2014111002912 caiwei 20141112 end> */
static inline int get_bpd(const char *name)
{
	int i = 0;
	for (i = 0; i < ARRAY_SIZE(bpd_label); i++) {
		if (strcmp(bpd_label[i], name) == 0)
			return i;
	}
	return -EINVAL;
}

static enum power_supply_property msm_batt_power_props[] = {
	POWER_SUPPLY_PROP_CHARGING_ENABLED,
	/* <DTS2014052400953 chenyuanquan 20140524 begin */
	/* <DTS2014061002202 jiangfei 20140610 begin */
	/*<DTS2014080807172 jiangfei 20140813 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	POWER_SUPPLY_PROP_FACTORY_DIAG,
	POWER_SUPPLY_PROP_INPUT_CURRENT_MAX,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_HOT_IBAT_LIMIT,
	POWER_SUPPLY_PROP_CHARGE_LOG,
	POWER_SUPPLY_PROP_TECHNOLOGY,
#endif
	/* DTS2014080807172 jiangfei 20140813 end> */
	/* DTS2014061002202 jiangfei 20140610 end> */
	/* DTS2014052400953 chenyuanquan 20140524 end> */
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CURRENT_NOW,
    /* DTS2014072806213 by l00220156 for factory interfaces 20140728 begin */
    POWER_SUPPLY_PROP_CURRENT_REALTIME,
    /* DTS2014072806213 by l00220156 for factory interfaces 20140728 end */
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_COOL_TEMP,
	POWER_SUPPLY_PROP_WARM_TEMP,
/* <DTS2014051601896 jiangfei 20140516 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	POWER_SUPPLY_PROP_RUNNING_TEST_STATUS,
#endif
/* DTS2014051601896 jiangfei 20140516 end> */
	POWER_SUPPLY_PROP_SYSTEM_TEMP_LEVEL,
      /* <DTS2014071002612  mapengfei 20140710 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	POWER_SUPPLY_PROP_RESUME_CHARGING,
#endif
       /* DTS2014071002612  mapengfei 20140710 end> */
};

static char *pm_batt_supplied_to[] = {
	"bms",
	/* <DTS2014052906550 liyuping 20140530 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	"ti-charger"
#endif
	/* DTS2014052906550 liyuping 20140530 end> */
};

struct vddtrim_map {
	int			trim_uv;
	int			trim_val;
};

/*
 * VDDTRIM is a 3 bit value which is split across two
 * register TRIM3(bit 5:4)	-> VDDTRIM bit(2:1)
 * register TRIM4(bit 4)	-> VDDTRIM bit(0)
 */
#define TRIM_CENTER			4
#define MAX_VDD_EA_TRIM_CFG		8
#define VDD_TRIM3_MASK			LBC_MASK(2, 1)
#define VDD_TRIM3_SHIFT			3
#define VDD_TRIM4_MASK			BIT(0)
#define VDD_TRIM4_SHIFT			4
#define AVG(VAL1, VAL2)			((VAL1 + VAL2) / 2)

/*
 * VDDTRIM table containing map of trim voltage and
 * corresponding trim value.
 */
struct vddtrim_map vddtrim_map[] = {
	{36700,		0x00},
	{28000,		0x01},
	{19800,		0x02},
	{10760,		0x03},
	{0,		0x04},
	{-8500,		0x05},
	{-16800,	0x06},
	{-25440,	0x07},
};

/*
 * struct qpnp_lbc_chip - device information
 * @dev:			device pointer to access the parent
 * @spmi:			spmi pointer to access spmi information
 * @chgr_base:			charger peripheral base address
 * @bat_if_base:		battery interface  peripheral base address
 * @usb_chgpth_base:		USB charge path peripheral base address
 * @misc_base:			misc peripheral base address
 * @bat_is_cool:		indicates that battery is cool
 * @bat_is_warm:		indicates that battery is warm
 * @chg_done:			indicates that charging is completed
 * @usb_present:		present status of USB
 * @batt_present:		present status of battery
 * @cfg_charging_disabled:	disable drawing current from USB.
 * @cfg_use_fake_battery:	flag to report default battery properties
 * @fastchg_on:			indicate charger in fast charge mode
 * @cfg_btc_disabled:		flag to disable btc (disables hot and cold
 *				irqs)
 * @cfg_max_voltage_mv:		the max volts the batt should be charged up to
 * @cfg_min_voltage_mv:		VIN_MIN configuration
 * @cfg_batt_weak_voltage_uv:	weak battery voltage threshold
 * @cfg_warm_bat_chg_ma:	warm battery maximum charge current in mA
 * @cfg_cool_bat_chg_ma:	cool battery maximum charge current in mA
 * @cfg_safe_voltage_mv:	safe voltage to which battery can charge
 * @cfg_warm_bat_mv:		warm temperature battery target voltage
 * @cfg_warm_bat_mv:		warm temperature battery target voltage
 * @cfg_cool_bat_mv:		cool temperature battery target voltage
 * @cfg_soc_resume_charging:	indicator that battery resumes charging
 * @cfg_float_charge:		enable float charging
 * @charger_disabled:		maintain USB path state.
 * @cfg_charger_detect_eoc:	charger can detect end of charging
 * @cfg_disable_vbatdet_based_recharge:	keep VBATDET comparator overriden to
 *				low and VBATDET irq disabled.
 * @cfg_safe_current:		battery safety current setting
 * @cfg_hot_batt_p:		hot battery threshold setting
 * @cfg_cold_batt_p:		eold battery threshold setting
 * @cfg_warm_bat_decidegc:	warm battery temperature in degree Celsius
 * @cfg_cool_bat_decidegc:	cool battery temperature in degree Celsius
 * @fake_battery_soc:		SOC value to be reported to userspace
 * @cfg_tchg_mins:		maximum allowed software initiated charge time
 * @chg_failed_count:		counter to maintained number of times charging
 *				failed
 * @cfg_disable_follow_on_reset	charger ignore PMIC reset signal
 * @cfg_bpd_detection:		battery present detection mechanism selection
 * @cfg_thermal_levels:		amount of thermal mitigation levels
 * @cfg_thermal_mitigation:	thermal mitigation level values
 * @therm_lvl_sel:		thermal mitigation level selection
 * @jeita_configure_lock:	lock to serialize jeita configuration request
 * @hw_access_lock:		lock to serialize access to charger registers
 * @ibat_change_lock:		lock to serialize ibat change requests from
 *				USB and thermal.
 * @irq_lock			lock to serialize enabling/disabling of irq
 * @supported_feature_flag	bitmask for all supported features
 * @vddtrim_alarm		alarm to schedule trim work at regular
 *				interval
 * @vddtrim_work		work to perform actual vddmax trimming
 * @init_trim_uv		initial trim voltage at bootup
 * @delta_vddmax_uv		current vddmax trim voltage
 * @chg_enable_lock:		lock to serialize charging enable/disable for
 *				SOC based resume charging
 * @usb_psy:			power supply to export information to
 *				userspace
 * @bms_psy:			power supply to export information to
 *				userspace
 * @batt_psy:			power supply to export information to
 *				userspace
 */
/* <DTS2014042804721 chenyuanquan 20140428 begin */
struct qpnp_lbc_chip {
	struct device			*dev;
	struct spmi_device		*spmi;
	u16				chgr_base;
	u16				bat_if_base;
	u16				usb_chgpth_base;
	u16				misc_base;
	bool				bat_is_cool;
	bool				bat_is_warm;
	bool				chg_done;
	bool				usb_present;
	bool				batt_present;
	bool				cfg_charging_disabled;
	bool				cfg_btc_disabled;
	bool				cfg_use_fake_battery;
	bool				fastchg_on;
	bool				cfg_use_external_charger;
	/* <DTS2014052906550 liyuping 20140530 begin */
	/* <DTS2014061002202 jiangfei 20140610 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	bool				use_other_charger;
	/*DTS2014091301574,begin:rm linear charger, 2014.9.30*/
	bool				use_other_charger_pad;
	/*DTS2014091301574,end:rm linear charger, 2014.9.30*/
	bool				resuming_charging;
	char				log_buf[LOG_BUF_LENGTH];
#endif
	/* DTS2014061002202 jiangfei 20140610 end> */
	/* DTS2014052906550 liyuping 20140530 end> */
	unsigned int			cfg_warm_bat_chg_ma;
	unsigned int			cfg_cool_bat_chg_ma;
	unsigned int			cfg_safe_voltage_mv;
	unsigned int			cfg_max_voltage_mv;
	unsigned int			cfg_min_voltage_mv;
	unsigned int			cfg_charger_detect_eoc;
	unsigned int			cfg_disable_vbatdet_based_recharge;
	unsigned int			cfg_batt_weak_voltage_uv;
	unsigned int			cfg_warm_bat_mv;
	unsigned int			cfg_cool_bat_mv;
	unsigned int			cfg_hot_batt_p;
	unsigned int			cfg_cold_batt_p;
	unsigned int			cfg_thermal_levels;
	unsigned int			therm_lvl_sel;
	unsigned int			*thermal_mitigation;
	unsigned int			cfg_safe_current;
	unsigned int			cfg_tchg_mins;
	unsigned int			chg_failed_count;
	unsigned int			cfg_disable_follow_on_reset;
	unsigned int			supported_feature_flag;
	int				cfg_bpd_detection;
	int				cfg_warm_bat_decidegc;
	int				cfg_cool_bat_decidegc;
	int				fake_battery_soc;
	/* <DTS2014071002612  mapengfei 20140710 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	bool				cfg_soc_resume_charging;
#endif
   /* DTS2014071002612  mapengfei 20140710 end> */
	int				cfg_float_charge;
	int				charger_disabled;
	int				prev_max_ma;
/* <DTS2014051601896 jiangfei 20140516 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	int				running_test_settled_status;
#endif
/* DTS2014051601896 jiangfei 20140516 end> */
	int				usb_psy_ma;
	int				delta_vddmax_uv;
	int				init_trim_uv;
	struct alarm			vddtrim_alarm;
	struct work_struct		vddtrim_work;
	struct qpnp_lbc_irq		irqs[MAX_IRQS];
	struct mutex			jeita_configure_lock;
	struct mutex			chg_enable_lock;
	spinlock_t			ibat_change_lock;
	spinlock_t			hw_access_lock;
	spinlock_t			irq_lock;
	struct power_supply		*usb_psy;
	struct power_supply		*bms_psy;
	/* <DTS2014052906550 liyuping 20140530 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	struct power_supply		*ti_charger;
#endif
	/* DTS2014052906550 liyuping 20140530 end> */
	struct power_supply		batt_psy;
	struct qpnp_adc_tm_btm_param	adc_param;
	struct qpnp_vadc_chip		*vadc_dev;
	struct qpnp_adc_tm_chip		*adc_tm_dev;
/*<DTS2014053001058 jiangfei 20140530 begin */
#ifdef CONFIG_HUAWEI_DSM
	struct delayed_work		check_charging_status_work;
	struct work_struct		dump_work;
	int				error_type;
#endif
/* DTS2014053001058 jiangfei 20140530 end> */
#ifdef CONFIG_HUAWEI_KERNEL
	int				cfg_term_current;
	int				cfg_cold_bat_decidegc;
	int				cfg_hot_bat_decidegc;
#endif
    /* <DTS2014070819388 zhaoxiaoli 20140711 begin */
	/* <DTS2014081410316 jiangfei 20140819 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	struct wake_lock		led_wake_lock;
	struct wake_lock		chg_wake_lock;
#endif
	/* DTS2014081410316 jiangfei 20140819 end> */
    /* DTS2014070819388 zhaoxiaoli 20140711 end> */
};

/* <DTS2014060603035 jiangfei 20140708 begin */
#ifdef CONFIG_HUAWEI_KERNEL
enum hw_high_low_temp_configure_type {
	COLD_COLD_ZONE,
	COLD_COOL_ZONE,
	COOL_WARM_ZONE,
	WARM_HOT_ZONE,
	HOT_HOT_ZONE,
	UNKNOW_ZONE,
};
#define HOT_TEMP_DEFAULT		520
#define COLD_TEMP_DEFAULT		0
#define BATT_FULL_LEVEL			100
#define BATT_LEVEL_99			99
static int bad_temp_flag = false;
#endif
/* DTS2014060603035 jiangfei 20140708 end> */
/* DTS2014042804721 chenyuanquan 20140428 end> */

/*<DTS2014053001058 jiangfei 20140530 begin */
/* <DTS2014061002202 jiangfei 20140610 begin */
/* <DTS2014062100152 jiangfei 20140623 begin */
#ifdef CONFIG_HUAWEI_DSM
/*charger dsm client definition */
struct dsm_dev dsm_charger = {
	.name = "dsm_charger", // dsm client name
	.fops = NULL,
	.buff_size = 4096, // buffer size
};
struct dsm_client *charger_dclient = NULL;
extern u16 bms_base;
/*8916 pmic LBC_CHGR registers*/
static u8 LBC_CHGR[] = {
	0x08,
	0x09,
	0x0A,
	0x0B,
	0x10,
	0x11,
	0x12,
	0x13,
	0x14,
	0x15,
	0x16,
	0x18,
	0x19,
	0x1A,
	0x1B,
	0x40,
	0x41,
	0x43,
	0x44,
	0x45,
	0x47,
	0x49,
	0x4A,
	0x4C,
	0x4D,
	0x50,
	0x52,
	0x55,
	0x5B,
	0x5E,
	0x5F,
	0x60,
	0x61,
	0x62,
	0x63,
	0x64,
	0x65,
	0x66,
	0x69,
	0x6A
};
/*8916 pmic LBC_BAT_IF registers*/
static u8 LBC_BAT_IF[] = {
	0x08,
	0x09,
	0x0A,
	0x10,
	0x11,
	0x12,
	0x13,
	0x14,
	0x15,
	0x16,
	0x18,
	0x19,
	0x1A,
	0x1B,
	0x48,
	0x49,
	0x4A,
	0x4F,
	0xD0
};
/*8916 pmic LBC_USB registers*/
static u8 LBC_USB[] = {
	0x08,
	0x09,
	0x10,
	0x11,
	0x12,
	0x13,
	0x14,
	0x15,
	0x16,
	0x18,
	0x19,
	0x1A,
	0x1B,
	0x42,
	0x47,
	0x4E,
	0x4F,
	0xD0
};
/*8916 pmic LBC_MISC registers*/
static u8 LBC_MISC[] = {
	0x40,
	0x41,
	0x42,
	0x43,
	0x49,
	0xCD,
	0xCE,
	0xD0
};

/*8916 pmic VM_BMS registers*/
static u8 VM_BMS[] = {
	0x08,
	0x09,
	0x10,
	0x11,
	0x12,
	0x13,
	0x14,
	0x15,
	0x16,
	0x18,
	0x19,
	0x1A,
	0x1B,
	0x40,
	0x42,
	0x43,
	0x44,
	0x46,
	0x47,
	0x50,
	0x51,
	0x53,
	0x55,
	0x56,
	0x57,
	0x58,
	0x5A,
	0x5B,
	0x5C,
	0x5D,
	0x5E,
	0x5F,
	0x60,
	0x61,
	0x62,
	0x63,
	0x64,
	0x65,
	0x66,
	0x67,
	0x6A,
	0x6B,
	0xB0,
	0xB1,
	0xB2,
	0xB3,
	0xB4,
	0xB5,
	0xB6,
	0xB7,
	0xB8,
	0xB9,
	0xC0,
	0xC1,
	0xC2,
	0xC3,
	0xC4,
	0xC5,
	0xC6,
	0xC7,
	0xC8,
	0xC9,
	0xCA,
	0xCB,
	0xCC,
	0xCD,
	0xCE,
	0xCF,
	0xD0,
	0xD8,
	0xD9,
	0xDA,
	0xDB
};

int dump_registers_and_adc(struct dsm_client *dclient, struct qpnp_lbc_chip *chip, int type );
struct qpnp_lbc_chip *g_lbc_chip = NULL;
bool use_other_charger = false;
extern void bq2415x_dump_regs(struct dsm_client *dclient);
extern void bq27510_dump_regs(struct dsm_client *dclient);
/* <DTS2014070701815 jiangfei 20140707 begin */
/*<DTS2014072501338 jiangfei 20140725 begin */
extern int is_bq24152_in_boost_mode(void);
extern int get_bq2415x_fault_status(void);
/*<DTS2014121703947  xiongxi xwx234328 20141217 begin*/
extern int bq2419x_ovp_status_check(void);
/*DTS2014121703947  xiongxi xwx234328 20141217 end>*/
#endif
/* DTS2014072501338 jiangfei 20140725 end> */
/* DTS2014070701815 jiangfei 20140707 end> */
/* DTS2014062100152 jiangfei 20140623 end> */
/* DTS2014061002202 jiangfei 20140610 end> */
/* DTS2014053001058 jiangfei 20140530 end> */

/* < DTS2014042818708 zhaoxiaoli 20140429 begin */
/*<DTS2014090201517 jiangfei 20140902 begin */
#ifdef CONFIG_HUAWEI_KERNEL
static int get_prop_batt_temp(struct qpnp_lbc_chip *chip);
//extern int get_bq2415x_reg_values(u8 reg);
#endif
/* DTS2014090201517 jiangfei 20140902 end> */
/* DTS2014042818708 zhaoxiaoli 20140429 end >*/
/* < DTS2014080705190 yuanzhen 20140807 begin */
#ifdef CONFIG_HUAWEI_KERNEL
#define HOT_DESIGN_MAX_CURRENT 1440
/* <DTS2014052400953 chenyuanquan 20140524 begin */
/*<DTS2014070404260 liyuping 20140704 begin */
int hot_design_current = HOT_DESIGN_MAX_CURRENT;
/* DTS2014070404260 liyuping 20140704 end> */
static int factory_diag_flag = 0;
static int factory_diag_last_current_ma = 0;
static int input_current_max_ma = 0;
static int input_current_max_flag = 0;
/* DTS2014052400953 chenyuanquan 20140524 end> */
#endif
/* DTS2014080705190 yuanzhen 20140807 end > */
/* < DTS2014052901670 zhaoxiaoli 20140529 begin */
#ifdef CONFIG_HUAWEI_KERNEL
static int fake_capacity = -1;
module_param(fake_capacity, int, 0644);
/* < DTS2014061009305 zhaoxiaoli 20140611 begin */
static int charge_no_limit = 0;
/* <DTS2014062508292 sunwenyong 20140625 begin */

enum hw_charge_configure_type{
	NORMAL_TYPE,
	DISABLE_SOFTWARE_HARDWARE_TEMP_CONTROL,
	DISABLE_SOFTWARE_TEMP_CONTROL,
	DISABLE_HARDWARE_TEMP_CONTROL,
	MAX_TYPE,
};

static int qpnp_lbc_bat_if_configure_btc(struct qpnp_lbc_chip *chip);
static int qpnp_lbc_is_batt_present(struct qpnp_lbc_chip *chip);

/*==========================================
FUNCTION: hw_set_charge_temp_type

DESCRIPTION:	1. set battery charge temp limit type

INPUT:	hw_charge_configure_type
OUTPUT: NULL
RETURN: NULL

============================================*/
static void hw_set_charge_temp_type(enum hw_charge_configure_type type)
{
	int batt_present,rc;
	if(!global_chip)
	{
		return ;
	}
	
	batt_present = qpnp_lbc_is_batt_present(global_chip);
	switch(type){
	case NORMAL_TYPE:
		if(batt_present)
		{
			/*<DTS2014072301355 jiangfei 20140723 begin */
			rc = qpnp_adc_tm_channel_measure(global_chip->adc_tm_dev,&global_chip->adc_param);
			if (rc) {
				pr_err("request ADC error rc=%d\n", rc);
			}
			/* DTS2014072301355 jiangfei 20140723 end> */
		}		
		global_chip->cfg_hot_batt_p = HOT_THD_35_PCT;
		global_chip->cfg_cold_batt_p = COLD_THD_70_PCT;
		rc = qpnp_lbc_bat_if_configure_btc(global_chip);
		if(rc)
		{
			pr_err("Failed to configure btc rc=%d\n", rc);
		}
		break;
	case DISABLE_SOFTWARE_HARDWARE_TEMP_CONTROL:
		qpnp_adc_tm_disable_chan_meas(global_chip->adc_tm_dev,&global_chip->adc_param);
		global_chip->cfg_hot_batt_p = HOT_THD_25_PCT;
		global_chip->cfg_cold_batt_p = COLD_THD_80_PCT;
		rc = qpnp_lbc_bat_if_configure_btc(global_chip);
		if(rc)
		{
			pr_err("Failed to configure btc rc=%d\n", rc);
		}
		else
		{
			pr_info("hardware battery threshold is changed to 25 to 80,and software temp control is canceled \n");
		}
		break;
	case DISABLE_SOFTWARE_TEMP_CONTROL:
		qpnp_adc_tm_disable_chan_meas(global_chip->adc_tm_dev,&global_chip->adc_param);		
		pr_info("software battery temperature control is canceled \n");
		break;
	case DISABLE_HARDWARE_TEMP_CONTROL:
		global_chip->cfg_hot_batt_p = HOT_THD_25_PCT;
		global_chip->cfg_cold_batt_p = COLD_THD_80_PCT;
		rc = qpnp_lbc_bat_if_configure_btc(global_chip);
		if(rc)
		{
			pr_err("Failed to configure btc rc=%d\n", rc);
		}
		else
		{
			pr_info("hardware battery temperature threshold is changed to 25 to 80 \n");
		}
		break;
	default:
		break;
	}

	return ;
}

static int set_charge_limit_temp_type(const char *val, struct kernel_param *kp)
{
	int ret = param_set_int(val, kp);

	if(!global_chip)
	{
		return ret;
	}
	else
	{
		hw_set_charge_temp_type((enum hw_charge_configure_type)charge_no_limit);
		return ret;
	}
}

module_param_call(charge_no_limit,&set_charge_limit_temp_type,&param_get_int,&charge_no_limit,0664);
/* DTS2014062508292 sunwenyong 20140625 end> */

/* DTS2014061009305 zhaoxiaoli 20140611 end> */
#endif
/* DTS2014052901670 zhaoxiaoli 20140529 end> */

static void qpnp_lbc_enable_irq(struct qpnp_lbc_chip *chip,
					struct qpnp_lbc_irq *irq)
{
	unsigned long flags;

	spin_lock_irqsave(&chip->irq_lock, flags);
	if (__test_and_clear_bit(0, &irq->disabled)) {
		pr_debug("number = %d\n", irq->irq);
		enable_irq(irq->irq);
		if (irq->is_wake)
			enable_irq_wake(irq->irq);
	}
	spin_unlock_irqrestore(&chip->irq_lock, flags);
}

static void qpnp_lbc_disable_irq(struct qpnp_lbc_chip *chip,
					struct qpnp_lbc_irq *irq)
{
	unsigned long flags;

	spin_lock_irqsave(&chip->irq_lock, flags);
	if (!__test_and_set_bit(0, &irq->disabled)) {
		pr_debug("number = %d\n", irq->irq);
		disable_irq_nosync(irq->irq);
		if (irq->is_wake)
			disable_irq_wake(irq->irq);
	}
	spin_unlock_irqrestore(&chip->irq_lock, flags);
}

static int __qpnp_lbc_read(struct spmi_device *spmi, u16 base,
			u8 *val, int count)
{
	int rc = 0;

	rc = spmi_ext_register_readl(spmi->ctrl, spmi->sid, base, val, count);
	if (rc)
		pr_err("SPMI read failed base=0x%02x sid=0x%02x rc=%d\n",
				base, spmi->sid, rc);

	return rc;
}

static int __qpnp_lbc_write(struct spmi_device *spmi, u16 base,
			u8 *val, int count)
{
	int rc;

	rc = spmi_ext_register_writel(spmi->ctrl, spmi->sid, base, val,
					count);
	if (rc)
		pr_err("SPMI write failed base=0x%02x sid=0x%02x rc=%d\n",
				base, spmi->sid, rc);

	return rc;
}

static int __qpnp_lbc_secure_write(struct spmi_device *spmi, u16 base,
				u16 offset, u8 *val, int count)
{
	int rc;
	u8 reg_val;

	reg_val = 0xA5;
	rc = __qpnp_lbc_write(spmi, base + SEC_ACCESS, &reg_val, 1);
	if (rc) {
		pr_err("SPMI read failed base=0x%02x sid=0x%02x rc=%d\n",
				base + SEC_ACCESS, spmi->sid, rc);
		return rc;
	}

	rc = __qpnp_lbc_write(spmi, base + offset, val, 1);
	if (rc)
		pr_err("SPMI write failed base=0x%02x sid=0x%02x rc=%d\n",
				base + SEC_ACCESS, spmi->sid, rc);

	return rc;
}

static int qpnp_lbc_read(struct qpnp_lbc_chip *chip, u16 base,
			u8 *val, int count)
{
	int rc = 0;
	struct spmi_device *spmi = chip->spmi;
	unsigned long flags;

	if (base == 0) {
		pr_err("base cannot be zero base=0x%02x sid=0x%02x rc=%d\n",
				base, spmi->sid, rc);
		return -EINVAL;
	}

	spin_lock_irqsave(&chip->hw_access_lock, flags);
	rc = __qpnp_lbc_read(spmi, base, val, count);
	spin_unlock_irqrestore(&chip->hw_access_lock, flags);

	return rc;
}

static int qpnp_lbc_write(struct qpnp_lbc_chip *chip, u16 base,
			u8 *val, int count)
{
	int rc = 0;
	struct spmi_device *spmi = chip->spmi;
	unsigned long flags;

	if (base == 0) {
		pr_err("base cannot be zero base=0x%02x sid=0x%02x rc=%d\n",
				base, spmi->sid, rc);
		return -EINVAL;
	}

	spin_lock_irqsave(&chip->hw_access_lock, flags);
	rc = __qpnp_lbc_write(spmi, base, val, count);
	spin_unlock_irqrestore(&chip->hw_access_lock, flags);

	return rc;
}

static int qpnp_lbc_masked_write(struct qpnp_lbc_chip *chip, u16 base,
				u8 mask, u8 val)
{
	int rc;
	u8 reg_val;
	struct spmi_device *spmi = chip->spmi;
	unsigned long flags;

	spin_lock_irqsave(&chip->hw_access_lock, flags);
	rc = __qpnp_lbc_read(spmi, base, &reg_val, 1);
	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n", base, rc);
		goto out;
	}
	pr_debug("addr = 0x%x read 0x%x\n", base, reg_val);

	reg_val &= ~mask;
	reg_val |= val & mask;

	pr_debug("writing to base=%x val=%x\n", base, reg_val);

	rc = __qpnp_lbc_write(spmi, base, &reg_val, 1);
	if (rc)
		pr_err("spmi write failed: addr=%03X, rc=%d\n", base, rc);

out:
	spin_unlock_irqrestore(&chip->hw_access_lock, flags);
	return rc;
}

static int __qpnp_lbc_secure_masked_write(struct spmi_device *spmi, u16 base,
				u16 offset, u8 mask, u8 val)
{
	int rc;
	u8 reg_val, reg_val1;

	rc = __qpnp_lbc_read(spmi, base + offset, &reg_val, 1);
	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n", base, rc);
		return rc;
	}
	pr_debug("addr = 0x%x read 0x%x\n", base, reg_val);

	reg_val &= ~mask;
	reg_val |= val & mask;
	pr_debug("writing to base=%x val=%x\n", base, reg_val);

	reg_val1 = 0xA5;
	rc = __qpnp_lbc_write(spmi, base + SEC_ACCESS, &reg_val1, 1);
	if (rc) {
		pr_err("SPMI read failed base=0x%02x sid=0x%02x rc=%d\n",
				base + SEC_ACCESS, spmi->sid, rc);
		return rc;
	}

	rc = __qpnp_lbc_write(spmi, base + offset, &reg_val, 1);
	if (rc) {
		pr_err("SPMI write failed base=0x%02x sid=0x%02x rc=%d\n",
				base + offset, spmi->sid, rc);
		return rc;
	}

	return rc;
}

static int qpnp_lbc_get_trim_voltage(u8 trim_reg)
{
	int i;

	for (i = 0; i < MAX_VDD_EA_TRIM_CFG; i++)
		if (trim_reg == vddtrim_map[i].trim_val)
			return vddtrim_map[i].trim_uv;

	pr_err("Invalid trim reg reg_val=%x\n", trim_reg);
	return -EINVAL;
}

static u8 qpnp_lbc_get_trim_val(struct qpnp_lbc_chip *chip)
{
	int i, sign;
	int delta_uv;

	sign = (chip->delta_vddmax_uv >= 0) ? -1 : 1;

	switch (sign) {
	case -1:
		for (i = TRIM_CENTER; i >= 0; i--) {
			if (vddtrim_map[i].trim_uv > chip->delta_vddmax_uv) {
				delta_uv = AVG(vddtrim_map[i].trim_uv,
						vddtrim_map[i + 1].trim_uv);
				if (chip->delta_vddmax_uv >= delta_uv)
					return vddtrim_map[i].trim_val;
				else
					return vddtrim_map[i + 1].trim_val;
			}
		}
		break;
	case 1:
		for (i = TRIM_CENTER; i <= 7; i++) {
			if (vddtrim_map[i].trim_uv < chip->delta_vddmax_uv) {
				delta_uv = AVG(vddtrim_map[i].trim_uv,
						vddtrim_map[i - 1].trim_uv);
				if (chip->delta_vddmax_uv >= delta_uv)
					return vddtrim_map[i - 1].trim_val;
				else
					return vddtrim_map[i].trim_val;
			}
		}
		break;
	}

	return vddtrim_map[i].trim_val;
}

/* <DTS2014073007208 jiangfei 201407801 begin */
static int qpnp_lbc_is_usb_chg_plugged_in(struct qpnp_lbc_chip *chip)
{
#ifdef CONFIG_HUAWEI_KERNEL
/*< DTS2014081101424  lishiliang 20140811 begin */
/* < DTS2014092405372 gwx199358 20140924 begin */
	uint32_t boardid = 0;
/* DTS2014092405372 gwx199358 20140924 end > */
	u8 usb_chgpth_rt_sts = 0;
#else
	u8 usbin_valid_rt_sts = 0;
#endif
/* DTS2014081101424  lishiliang 20140811 end> */
	int rc;

/* < DTS2014092405372 gwx199358 20140924 begin */
#ifdef CONFIG_HUAWEI_KERNEL
        boardid = socinfo_get_platform_type();
        if (BOARDID_T1_10 != boardid) {
                rc = qpnp_lbc_read(chip, CBLPWR_USBIN_ADDR,
                        &usb_chgpth_rt_sts, 1);
                if (rc) {
                        pr_err("spmi read failed: addr=%03X, rc=%d\n",
                                CBLPWR_USBIN_ADDR, rc);
                        return rc;
                }

                pr_debug("usb_chgpth_rt_sts 0x%x\n", usb_chgpth_rt_sts);

                return (usb_chgpth_rt_sts & CBLPWR_USBIN_VALID_IRQ) ? 1 : 0;
	}
	else {
                rc = qpnp_lbc_read(chip, INT_RT_STS(chip->usb_chgpth_base),
                        &usb_chgpth_rt_sts, 1);
                if (rc) {
                        pr_err("spmi read failed: addr=%03X, rc=%d\n",
                                INT_RT_STS(chip->usb_chgpth_base), rc);
                                return rc;
                }

                pr_debug("usb_chgpth_rt_sts 0x%x\n", usb_chgpth_rt_sts);

                return (usb_chgpth_rt_sts & USBIN_VALID_IRQ) ? 1 : 0;
	}
/* DTS2014092405372 gwx199358 20140924 end > */
#else
	rc = qpnp_lbc_read(chip, chip->usb_chgpth_base + USB_PTH_STS_REG,
				&usbin_valid_rt_sts, 1);
	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n",
				chip->usb_chgpth_base + USB_PTH_STS_REG, rc);
		return rc;
	}

	pr_debug("usb_path_sts 0x%x\n", usbin_valid_rt_sts);

	return (usbin_valid_rt_sts & USB_IN_VALID_MASK) ? 1 : 0;
#endif
}
/* DTS2014073007208 jiangfei 20140801 end> */

static int qpnp_lbc_charger_enable(struct qpnp_lbc_chip *chip, int reason,
					int enable)
{
	int disabled = chip->charger_disabled;
	u8 reg_val;
	int rc = 0;

	/* < DTS2014080200746 yuanzhen 20140802 begin */
#ifdef CONFIG_HUAWEI_KERNEL
    /*DTS2014091301574,begin:rm linear charger, 2014.9.30*/
	if (chip->use_other_charger || chip->use_other_charger_pad) {
	/*DTS2014091301574,end:rm linear charger, 2014.9.30*/
		/* always disble pmic charger if use external charger */
		reg_val = CHG_FORCE_BATT_ON;
		rc = qpnp_lbc_masked_write(chip, chip->chgr_base + CHG_CTRL_REG,
							CHG_EN_MASK, reg_val);
		if (rc) {
			pr_err("Failed to disable pmic charger rc=%d\n", rc);
			return rc;
		}
		return rc;
	}
#endif
	/* DTS2014080200746 yuanzhen 20140802 end > */

	pr_debug("reason=%d requested_enable=%d disabled_status=%d\n",
					reason, enable, disabled);
	if (enable)
		disabled &= ~reason;
	else
		disabled |= reason;
	/* <DTS2014052103449 chenyuanquan 20140521 begin */
	/* avoid goto skip when enable charger but chip->charger_diabled is 0 */
	/* <DTS2014052400953 chenyuanquan 20140524 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	if ((!!chip->charger_disabled == !!disabled) && chip->charger_disabled)
#else
	if (!!chip->charger_disabled == !!disabled)
#endif
		goto skip;
	/* DTS2014052400953 chenyuanquan 20140524 end> */
	/* DTS2014052103449 chenyuanquan 20140521 end> */

	/* <DTS2014042804721 chenyuanquan 20140428 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	if ((bad_temp_flag) && enable)
	{
		pr_info("bad_temp_flag %d, not enable charge\n", bad_temp_flag);
		goto skip;
	}
#endif
	/* DTS2014042804721 chenyuanquan 20140428 end> */

	reg_val = !!disabled ? CHG_FORCE_BATT_ON : CHG_ENABLE;
	rc = qpnp_lbc_masked_write(chip, chip->chgr_base + CHG_CTRL_REG,
				CHG_EN_MASK, reg_val);
	if (rc) {
		pr_err("Failed to %s charger rc=%d\n",
				reg_val ? "enable" : "disable", rc);
		return rc;
	}
skip:
	chip->charger_disabled = disabled;
	return rc;
}

/* < DTS2014052901670 zhaoxiaoli 20140529 begin */
#ifdef CONFIG_HUAWEI_KERNEL
int is_usb_chg_exist(void)
{
	if (!global_chip) 
	{
		pr_err("called before init\n");
		return -EINVAL;
	}
	return qpnp_lbc_is_usb_chg_plugged_in(global_chip);
}
EXPORT_SYMBOL(is_usb_chg_exist);
#endif
/* DTS2014052901670 zhaoxiaoli 20140529 end> */

static int qpnp_lbc_is_batt_present(struct qpnp_lbc_chip *chip)
{
	u8 batt_pres_rt_sts;
	int rc;

	rc = qpnp_lbc_read(chip, chip->bat_if_base + INT_RT_STS_REG,
				&batt_pres_rt_sts, 1);
	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n",
				chip->bat_if_base + INT_RT_STS_REG, rc);
		return rc;
	}

	return (batt_pres_rt_sts & BATT_PRES_IRQ) ? 1 : 0;
}

static int qpnp_lbc_bat_if_configure_btc(struct qpnp_lbc_chip *chip)
{
	u8 btc_cfg = 0, mask = 0, rc;

	/* Do nothing if battery peripheral not present */
	if (!chip->bat_if_base)
		return 0;

	if ((chip->cfg_hot_batt_p == HOT_THD_25_PCT)
			|| (chip->cfg_hot_batt_p == HOT_THD_35_PCT)) {
		btc_cfg |= btc_value[chip->cfg_hot_batt_p];
		mask |= BTC_HOT_MASK;
	}

	if ((chip->cfg_cold_batt_p == COLD_THD_70_PCT) ||
			(chip->cfg_cold_batt_p == COLD_THD_80_PCT)) {
		btc_cfg |= btc_value[chip->cfg_cold_batt_p];
		mask |= BTC_COLD_MASK;
	}

	if (!chip->cfg_btc_disabled) {
		mask |= BTC_COMP_EN_MASK;
		btc_cfg |= BTC_COMP_EN_MASK;
	}

	pr_debug("BTC configuration mask=%x\n", btc_cfg);

	rc = qpnp_lbc_masked_write(chip,
			chip->bat_if_base + BAT_IF_BTC_CTRL,
			mask, btc_cfg);
	if (rc)
		pr_err("Failed to configure BTC rc=%d\n", rc);

	return rc;
}

#define QPNP_LBC_VBATWEAK_MIN_UV        3000000
#define QPNP_LBC_VBATWEAK_MAX_UV        3581250
#define QPNP_LBC_VBATWEAK_STEP_UV       18750
static int qpnp_lbc_vbatweak_set(struct qpnp_lbc_chip *chip, int voltage)
{
	u8 reg_val;
	int rc;

	if (voltage < QPNP_LBC_VBATWEAK_MIN_UV ||
			voltage > QPNP_LBC_VBATWEAK_MAX_UV) {
		rc = -EINVAL;
	} else {
		reg_val = (voltage - QPNP_LBC_VBATWEAK_MIN_UV) /
					QPNP_LBC_VBATWEAK_STEP_UV;
		pr_debug("VBAT_WEAK=%d setting %02x\n",
				chip->cfg_batt_weak_voltage_uv, reg_val);
		rc = qpnp_lbc_write(chip, chip->chgr_base + CHG_VBAT_WEAK_REG,
					&reg_val, 1);
		if (rc)
			pr_err("Failed to set VBAT_WEAK rc=%d\n", rc);
	}

	return rc;
}

#define QPNP_LBC_VBAT_MIN_MV		4000
#define QPNP_LBC_VBAT_MAX_MV		4775
#define QPNP_LBC_VBAT_STEP_MV		25
static int qpnp_lbc_vddsafe_set(struct qpnp_lbc_chip *chip, int voltage)
{
	u8 reg_val;
	int rc;

	if (voltage < QPNP_LBC_VBAT_MIN_MV
			|| voltage > QPNP_LBC_VBAT_MAX_MV) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}
	reg_val = (voltage - QPNP_LBC_VBAT_MIN_MV) / QPNP_LBC_VBAT_STEP_MV;
	pr_debug("voltage=%d setting %02x\n", voltage, reg_val);
	rc = qpnp_lbc_write(chip, chip->chgr_base + CHG_VDD_SAFE_REG,
				&reg_val, 1);
	if (rc)
		pr_err("Failed to set VDD_SAFE rc=%d\n", rc);

	return rc;
}

static int qpnp_lbc_vddmax_set(struct qpnp_lbc_chip *chip, int voltage)
{
	u8 reg_val;
	int rc, trim_val;
	unsigned long flags;

	if (voltage < QPNP_LBC_VBAT_MIN_MV
			|| voltage > QPNP_LBC_VBAT_MAX_MV) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}

	spin_lock_irqsave(&chip->hw_access_lock, flags);
	reg_val = (voltage - QPNP_LBC_VBAT_MIN_MV) / QPNP_LBC_VBAT_STEP_MV;
	pr_debug("voltage=%d setting %02x\n", voltage, reg_val);
	rc = __qpnp_lbc_write(chip->spmi, chip->chgr_base + CHG_VDD_MAX_REG,
				&reg_val, 1);
	if (rc) {
		pr_err("Failed to set VDD_MAX rc=%d\n", rc);
		goto out;
	}

	/* Update trim value */
	if (chip->supported_feature_flag & VDD_TRIM_SUPPORTED) {
		trim_val = qpnp_lbc_get_trim_val(chip);
		reg_val = (trim_val & VDD_TRIM3_MASK) << VDD_TRIM3_SHIFT;
		rc = __qpnp_lbc_secure_masked_write(chip->spmi,
				chip->misc_base, MISC_TRIM3_REG,
				MISC_TRIM3_VDD_MASK, reg_val);
		if (rc) {
			pr_err("Failed to set MISC_TRIM3_REG rc=%d\n", rc);
			goto out;
		}

		reg_val = (trim_val & VDD_TRIM4_MASK) << VDD_TRIM4_SHIFT;
		rc = __qpnp_lbc_secure_masked_write(chip->spmi,
				chip->misc_base, MISC_TRIM4_REG,
				MISC_TRIM4_VDD_MASK, reg_val);
		if (rc) {
			pr_err("Failed to set MISC_TRIM4_REG rc=%d\n", rc);
			goto out;
		}

		chip->delta_vddmax_uv = qpnp_lbc_get_trim_voltage(trim_val);
		if (chip->delta_vddmax_uv == -EINVAL) {
			pr_err("Invalid trim voltage=%d\n",
					chip->delta_vddmax_uv);
			rc = -EINVAL;
			goto out;
		}

		pr_debug("VDD_MAX delta=%d trim value=%x\n",
				chip->delta_vddmax_uv, trim_val);
	}

out:
	spin_unlock_irqrestore(&chip->hw_access_lock, flags);
	return rc;
}

static int qpnp_lbc_set_appropriate_vddmax(struct qpnp_lbc_chip *chip)
{
	int rc;

	if (chip->bat_is_cool)
		rc = qpnp_lbc_vddmax_set(chip, chip->cfg_cool_bat_mv);
	else if (chip->bat_is_warm)
		rc = qpnp_lbc_vddmax_set(chip, chip->cfg_warm_bat_mv);
	else
		rc = qpnp_lbc_vddmax_set(chip, chip->cfg_max_voltage_mv);
	if (rc)
		pr_err("Failed to set appropriate vddmax rc=%d\n", rc);

	return rc;
}

#define QPNP_LBC_MIN_DELTA_UV			13000
static void qpnp_lbc_adjust_vddmax(struct qpnp_lbc_chip *chip, int vbat_uv)
{
	int delta_uv, prev_delta_uv, rc;

	prev_delta_uv =  chip->delta_vddmax_uv;
	delta_uv = (int)(chip->cfg_max_voltage_mv * 1000) - vbat_uv;

	/*
	 * If delta_uv is positive, apply trim if delta_uv > 13mv
	 * If delta_uv is negative always apply trim.
	 */
	if (delta_uv > 0 && delta_uv < QPNP_LBC_MIN_DELTA_UV) {
		pr_debug("vbat is not low enough to increase vdd\n");
		return;
	}

	pr_debug("vbat=%d current delta_uv=%d prev delta_vddmax_uv=%d\n",
			vbat_uv, delta_uv, chip->delta_vddmax_uv);
	chip->delta_vddmax_uv = delta_uv + chip->delta_vddmax_uv;
	pr_debug("new delta_vddmax_uv  %d\n", chip->delta_vddmax_uv);
	rc = qpnp_lbc_set_appropriate_vddmax(chip);
	if (rc) {
		pr_err("Failed to set appropriate vddmax rc=%d\n", rc);
		chip->delta_vddmax_uv = prev_delta_uv;
	}
}

#define QPNP_LBC_VINMIN_MIN_MV		4200
#define QPNP_LBC_VINMIN_MAX_MV		5037
#define QPNP_LBC_VINMIN_STEP_MV		27
static int qpnp_lbc_vinmin_set(struct qpnp_lbc_chip *chip, int voltage)
{
	u8 reg_val;
	int rc;

	if ((voltage < QPNP_LBC_VINMIN_MIN_MV)
			|| (voltage > QPNP_LBC_VINMIN_MAX_MV)) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}

	reg_val = (voltage - QPNP_LBC_VINMIN_MIN_MV) / QPNP_LBC_VINMIN_STEP_MV;
	pr_debug("VIN_MIN=%d setting %02x\n", voltage, reg_val);
	rc = qpnp_lbc_write(chip, chip->chgr_base + CHG_VIN_MIN_REG,
				&reg_val, 1);
	if (rc)
		pr_err("Failed to set VIN_MIN rc=%d\n", rc);

	return rc;
}

#define QPNP_LBC_IBATSAFE_MIN_MA	90
#define QPNP_LBC_IBATSAFE_MAX_MA	1440
#define QPNP_LBC_I_STEP_MA		90
static int qpnp_lbc_ibatsafe_set(struct qpnp_lbc_chip *chip, int safe_current)
{
	u8 reg_val;
	int rc;

	if (safe_current < QPNP_LBC_IBATSAFE_MIN_MA
			|| safe_current > QPNP_LBC_IBATSAFE_MAX_MA) {
		pr_err("bad mA=%d asked to set\n", safe_current);
		return -EINVAL;
	}

	reg_val = (safe_current - QPNP_LBC_IBATSAFE_MIN_MA)
			/ QPNP_LBC_I_STEP_MA;
	pr_debug("Ibate_safe=%d setting %02x\n", safe_current, reg_val);

	rc = qpnp_lbc_write(chip, chip->chgr_base + CHG_IBAT_SAFE_REG,
				&reg_val, 1);
	if (rc)
		pr_err("Failed to set IBAT_SAFE rc=%d\n", rc);

	return rc;
}

#define QPNP_LBC_IBATMAX_MIN	90
/* <DTS2014061007778 chenyuanquan 20140616 begin */
/* limit the max charging current to 1100 mA */
#ifdef CONFIG_HUAWEI_KERNEL
#define QPNP_LBC_IBATMAX_MAX	1100
#else
#define QPNP_LBC_IBATMAX_MAX	1440
#endif
/* DTS2014061007778 chenyuanquan 20140616 end> */
/*
 * Set maximum current limit from charger
 * ibat =  System current + charging current
 */


/* <DTS2014071502649 tanyanying 20140714 begin */
#ifdef CONFIG_HUAWEI_THERMAL
extern int check_battery_id(void);
#endif
/* DTS2014071502649 tanyanying 20140714 end> */

static int qpnp_lbc_ibatmax_set(struct qpnp_lbc_chip *chip, int chg_current)
{
	u8 reg_val;
	int rc;

	if (chg_current > QPNP_LBC_IBATMAX_MAX)
		pr_debug("bad mA=%d clamping current\n", chg_current);

	chg_current = clamp(chg_current, QPNP_LBC_IBATMAX_MIN,
						QPNP_LBC_IBATMAX_MAX);
	/* <DTS2014052400953 chenyuanquan 20140524 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	chg_current = min(chg_current, QPNP_LBC_IBATMAX_MAX);
#endif
	/* DTS2014052400953 chenyuanquan 20140524 end> */
	reg_val = (chg_current - QPNP_LBC_IBATMAX_MIN) / QPNP_LBC_I_STEP_MA;

/* <DTS2014071502649 tanyanying 20140714 begin */
#ifdef CONFIG_HUAWEI_THERMAL
	if(check_battery_id())
	{
		reg_val=0;
	}
#endif
/* DTS2014071502649 tanyanying 20140714 end> */
	rc = qpnp_lbc_write(chip, chip->chgr_base + CHG_IBAT_MAX_REG,
				&reg_val, 1);
	if (rc)
		pr_err("Failed to set IBAT_MAX rc=%d\n", rc);
	else
		chip->prev_max_ma = chg_current;

	return rc;
}

#define QPNP_LBC_TCHG_MIN	4
#define QPNP_LBC_TCHG_MAX	512
#define QPNP_LBC_TCHG_STEP	4
static int qpnp_lbc_tchg_max_set(struct qpnp_lbc_chip *chip, int minutes)
{
	u8 reg_val = 0;
	int rc;

	minutes = clamp(minutes, QPNP_LBC_TCHG_MIN, QPNP_LBC_TCHG_MAX);

	/* Disable timer */
	rc = qpnp_lbc_masked_write(chip, chip->chgr_base + CHG_TCHG_MAX_EN_REG,
						CHG_TCHG_MAX_EN_BIT, 0);
	if (rc) {
		pr_err("Failed to write tchg_max_en rc=%d\n", rc);
		return rc;
	}

	reg_val = (minutes / QPNP_LBC_TCHG_STEP) - 1;

	pr_debug("TCHG_MAX=%d mins setting %x\n", minutes, reg_val);
	rc = qpnp_lbc_masked_write(chip, chip->chgr_base + CHG_TCHG_MAX_REG,
						CHG_TCHG_MAX_MASK, reg_val);
	if (rc) {
		pr_err("Failed to write tchg_max_reg rc=%d\n", rc);
		return rc;
	}

	/* Enable timer */
	rc = qpnp_lbc_masked_write(chip, chip->chgr_base + CHG_TCHG_MAX_EN_REG,
				CHG_TCHG_MAX_EN_BIT, CHG_TCHG_MAX_EN_BIT);
	if (rc) {
		pr_err("Failed to write tchg_max_en rc=%d\n", rc);
		return rc;
	}

	return rc;
}

static int qpnp_lbc_vbatdet_override(struct qpnp_lbc_chip *chip, int ovr_val)
{
	int rc;
	u8 reg_val;
	struct spmi_device *spmi = chip->spmi;
	unsigned long flags;

	spin_lock_irqsave(&chip->hw_access_lock, flags);

	rc = __qpnp_lbc_read(spmi, chip->chgr_base + CHG_COMP_OVR1,
				&reg_val, 1);
	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n",
						chip->chgr_base, rc);
		goto out;
	}
	pr_debug("addr = 0x%x read 0x%x\n", chip->chgr_base, reg_val);

	reg_val &= ~CHG_VBAT_DET_OVR_MASK;
	reg_val |= ovr_val & CHG_VBAT_DET_OVR_MASK;

	pr_debug("writing to base=%x val=%x\n", chip->chgr_base, reg_val);

	rc = __qpnp_lbc_secure_write(spmi, chip->chgr_base, CHG_COMP_OVR1,
					&reg_val, 1);
	if (rc)
		pr_err("spmi write failed: addr=%03X, rc=%d\n",
						chip->chgr_base, rc);

out:
	spin_unlock_irqrestore(&chip->hw_access_lock, flags);
	return rc;
}

static int get_prop_battery_voltage_now(struct qpnp_lbc_chip *chip)
{
	int rc = 0;
	struct qpnp_vadc_result results;

	rc = qpnp_vadc_read(chip->vadc_dev, VBAT_SNS, &results);
	if (rc) {
		pr_err("Unable to read vbat rc=%d\n", rc);
		return 0;
	}

	return results.physical;
}

/*<DTS2014053001058 jiangfei 20140530 begin */
#ifdef CONFIG_HUAWEI_DSM
static int get_prop_battery_id(struct qpnp_lbc_chip *chip)
{
	int rc = 0;
	struct qpnp_vadc_result results;

	rc = qpnp_vadc_read(chip->vadc_dev, LR_MUX2_BAT_ID, &results);
	if (rc) {
		pr_err("Unable to read battery_id rc=%d\n", rc);
		return 0;
	}

	return results.physical;
}

static int get_prop_vbus_uv(struct qpnp_lbc_chip *chip)
{
	int rc = 0;
	struct qpnp_vadc_result results;

	rc = qpnp_vadc_read(chip->vadc_dev, USBIN, &results);
	if (rc) {
		pr_err("Unable to read vbus rc=%d\n", rc);
		return 0;
	}

	return results.physical;
}

static int get_prop_vchg_uv(struct qpnp_lbc_chip *chip)
{
	int rc = 0;
	struct qpnp_vadc_result results;

	rc = qpnp_vadc_read(chip->vadc_dev, VCHG_SNS, &results);
	if (rc) {
		pr_err("Unable to read vchg rc=%d\n", rc);
		return 0;
	}

	return results.physical;
}

static int get_prop_vcoin_uv(struct qpnp_lbc_chip *chip)
{
	int rc = 0;
	struct qpnp_vadc_result results;

	rc = qpnp_vadc_read(chip->vadc_dev, VCOIN, &results);
	if (rc) {
		pr_err("Unable to read vcoin rc=%d\n", rc);
		return 0;
	}

	return results.physical;
}

 /* <DTS2014062601697 jiangfei 20140626 begin */
static int is_usb_ovp(struct qpnp_lbc_chip *chip)
{
	int rc = 0;
	u8 usbin_valid_rt_sts = 0;
	rc = qpnp_lbc_read(chip, chip->usb_chgpth_base + USB_PTH_STS_REG,
				&usbin_valid_rt_sts, 1);
	if (rc) {
			pr_err("spmi read failed: addr=%03X, rc=%d\n",
				chip->usb_chgpth_base + USB_PTH_STS_REG, rc);
				return rc;
	}

	return ((usbin_valid_rt_sts & USB_VALID_MASK)== USB_VALID_OVP_VALUE) ? 1 : 0;

}
#endif
/* DTS2014062601697 jiangfei 20140626 end> */
/* DTS2014053001058 jiangfei 20140530 end> */
/* <DTS2014110409521 caiwei 20141104 begin */
static int equip_hot_temperate_test_get(struct qpnp_lbc_chip *chip)
{
    union power_supply_propval ret = {0,};

    if (chip->bms_psy)
    {
        chip->bms_psy->get_property(chip->bms_psy,
                                    POWER_SUPPLY_PROP_HOT_TEMP_TEST_STATUS, &ret);
        return ret.intval;
    }
    else
    {
        pr_debug("No BMS supply registered return 0\n");
    }

    return 0;
}

static int equip_hot_temperate_test_set(struct qpnp_lbc_chip *chip, int enable)
{
    union power_supply_propval ret = {enable,};
    if (chip->bms_psy)
    {
        chip->bms_psy->set_property(chip->bms_psy,
                                    POWER_SUPPLY_PROP_HOT_TEMP_TEST_STATUS, &ret);
        return ret.intval;
    }
    else
    {
        pr_debug("No BMS supply registered return 0\n");
    }

    return 0;

}
/* DTS2014110409521 caiwei 20141104 end> */
static int get_prop_batt_present(struct qpnp_lbc_chip *chip)
{
	u8 reg_val;
	int rc;

	rc = qpnp_lbc_read(chip, chip->bat_if_base + BAT_IF_PRES_STATUS_REG,
				&reg_val, 1);
	if (rc) {
		pr_err("Failed to read battery status read failed rc=%d\n",
				rc);
		return 0;
	}

	return (reg_val & BATT_PRES_MASK) ? 1 : 0;
}

/* < DTS2014042818708 zhaoxiaoli 20140429 begin */
#ifdef CONFIG_HUAWEI_KERNEL
#define BATT_TEMP_MAX    600
#endif
/* DTS2014042818708 zhaoxiaoli 20140429 end >*/

static int get_prop_batt_health(struct qpnp_lbc_chip *chip)
{
	u8 reg_val;
	int rc;
/* < DTS2014042818708 zhaoxiaoli 20140429 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	int temp = 0;

    rc = get_prop_batt_present(chip);
    if(!rc)
    {
        return POWER_SUPPLY_HEALTH_DEAD;
    }
#endif
/* DTS2014042818708 zhaoxiaoli 20140429 end >*/

	rc = qpnp_lbc_read(chip, chip->bat_if_base + BAT_IF_TEMP_STATUS_REG,
				&reg_val, 1);
	if (rc) {
		pr_err("Failed to read battery health rc=%d\n", rc);
		return POWER_SUPPLY_HEALTH_UNKNOWN;
	}

/* < DTS2014042818708 zhaoxiaoli 20140429 begin */
#ifdef CONFIG_HUAWEI_KERNEL
    temp = get_prop_batt_temp(chip);
    if(temp >= BATT_TEMP_MAX)
    {
        return POWER_SUPPLY_HEALTH_OVERHEAT;
    }
    else
    {
        /* <DTS2014073105492 modified by l00220156 20140818 for bq24296 begin*/
        if (chip->bat_is_cool)
            return POWER_SUPPLY_HEALTH_GOOD;
        if (chip->bat_is_warm)
            return POWER_SUPPLY_HEALTH_GOOD;
        /* <DTS2014073105492 modified by l00220156 20140818 for bq24296 end*/ 
        
        return POWER_SUPPLY_HEALTH_GOOD;
    }
#else
	if (BATT_TEMP_HOT_MASK & reg_val)
		return POWER_SUPPLY_HEALTH_OVERHEAT;
	if (!(BATT_TEMP_COLD_MASK & reg_val))
		return POWER_SUPPLY_HEALTH_COLD;
	if (chip->bat_is_cool)
		return POWER_SUPPLY_HEALTH_COOL;
	if (chip->bat_is_warm)
		return POWER_SUPPLY_HEALTH_WARM;

	return POWER_SUPPLY_HEALTH_GOOD;
#endif
/* DTS2014042818708 zhaoxiaoli 20140429 end >*/
}

/* <Begin DTS2014080603839 modified by l00220156 for interface of vm and charger 20140806 > */
static int get_prop_charge_type(struct qpnp_lbc_chip *chip)
{
#if 0
	int rc;
	u8 reg_val;
#endif

	if (!get_prop_batt_present(chip))
		return POWER_SUPPLY_CHARGE_TYPE_NONE;

    return bq2419x_get_charge_type();

#if 0
	rc = qpnp_lbc_read(chip, chip->chgr_base + INT_RT_STS_REG,
				&reg_val, 1);
	if (rc) {
		pr_err("Failed to read interrupt sts %d\n", rc);
		return POWER_SUPPLY_CHARGE_TYPE_NONE;
	}

	if (reg_val & FAST_CHG_ON_IRQ)
		return POWER_SUPPLY_CHARGE_TYPE_FAST;

	return POWER_SUPPLY_CHARGE_TYPE_NONE;
#endif
}
/* <Begin DTS2014080603839 modified by l00220156 for interface of vm and charger 20140806 > */
static int get_prop_batt_status(struct qpnp_lbc_chip *chip)
{
	int rc;
	u8 reg_val;
      /* <DTS2014111002912 caiwei 20141112 begin */
	if (!bq2419x_otg_status_check() && qpnp_lbc_is_usb_chg_plugged_in(chip) && chip->chg_done)
      /* DTS2014111002912 caiwei 20141112 end> */
		return POWER_SUPPLY_STATUS_FULL;

	rc = qpnp_lbc_read(chip, chip->chgr_base + INT_RT_STS_REG,
				&reg_val, 1);
	if (rc) {
		pr_err("Failed to read interrupt sts rc= %d\n", rc);
		return POWER_SUPPLY_CHARGE_TYPE_NONE;
	}

	/* <DTS2014042804721 chenyuanquan 20140428 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	if (bad_temp_flag == 1)
	{
		return POWER_SUPPLY_STATUS_NOT_CHARGING;
	}
#endif
	/* DTS2014042804721 chenyuanquan 20140428 end> */
    
    /* <Begin DTS2014080603839 modified by l00220156 for interface of vm and charger 20140806 > */
    return bq2419x_get_charge_status();
#if 0    
	if (reg_val & FAST_CHG_ON_IRQ)
		return POWER_SUPPLY_STATUS_CHARGING;

	return POWER_SUPPLY_STATUS_DISCHARGING;
#endif
    /* <End DTS2014080603839 modified by l00220156 for interface of vm and charger 20140806 > */    
}

/* <DTS2014060603035 jiangfei 20140708 begin */
#ifdef CONFIG_HUAWEI_KERNEL
int hw_get_prop_batt_status(void)
{
	if(!global_chip)
	{
		pr_err("global_chip is not init ready \n");
		return POWER_SUPPLY_CHARGE_TYPE_NONE;
	}
	else
	{
		return get_prop_batt_status(global_chip);
	}
}
EXPORT_SYMBOL(hw_get_prop_batt_status);
#endif
/* DTS2014060603035 jiangfei 20140708 end> */

static int get_prop_current_now(struct qpnp_lbc_chip *chip)
{
	union power_supply_propval ret = {0,};

	if (chip->bms_psy) {
		chip->bms_psy->get_property(chip->bms_psy,
			  POWER_SUPPLY_PROP_CURRENT_NOW, &ret);
		return ret.intval;
	} else {
		pr_debug("No BMS supply registered return 0\n");
	}

	return 0;
}

/* <DTS2014082700221 caiwei 20140828 begin*/
#define MAX_UP_POINT 1
#define MAX_DOWN_POINT 1
static int last_battery_soc = -1;
static int last_charging_state = 0;
static int charging_state = 0;
#define SMOOTH_ADJUST 1
#define MAX_SMOOTH_STEP 2
static int smooth_step = 0;

#define CAPACITY_SOC4  4
#define CAPACITY_SOC10  10
#define CAPACITY_SOC20  20
#define CAPACITY_SOC90  90
#define MIN_FULL_CAHRGE_CURRENT (-5)

static int pad_smooth_soc(int new_soc)
{
    int difference = 0;
    int result = 0;

    if ((CAPACITY_SOC10 > new_soc) && (CAPACITY_SOC4 <= new_soc)){
        new_soc = (new_soc + 4) / 2;
    }

    if (charging_state == 0){
        if ((0 == smooth_step) && (9 == new_soc % 10) && (CAPACITY_SOC90 > new_soc)){
            smooth_step = 1;
        }
    } else {
        if ((0 == smooth_step) && (0 == new_soc % 10) && (91 > new_soc)){
            smooth_step = 1;
        }
    }

    if ((CAPACITY_SOC20 > new_soc) && (CAPACITY_SOC10 <= new_soc)){
        new_soc = (new_soc + new_soc / 2) / 2 - 1;
    }

    if ((CAPACITY_SOC90 > new_soc) && (CAPACITY_SOC20 <= new_soc)){
        new_soc = new_soc - (9 - new_soc / 10);
    }

    if (-1 == last_battery_soc){
        last_battery_soc = new_soc;
    }

    if (charging_state == 0){
        //NO_charging
        difference = last_battery_soc - new_soc;
        if (0 > difference){
            return last_battery_soc;
        }

        if (0 != smooth_step){
            if (0 != difference){
                result = last_battery_soc - SMOOTH_ADJUST;
                last_battery_soc = result;
            }

            return last_battery_soc;
        }

        if (MAX_DOWN_POINT <= difference){
            result = last_battery_soc - MAX_DOWN_POINT;
            last_battery_soc = result;
            return result;
        }
    } else {
        //charging
        difference = new_soc - last_battery_soc;
        if (0 > difference){
            return last_battery_soc;
        }

        if (0 != smooth_step){
            if (0 != difference){
                result = last_battery_soc + SMOOTH_ADJUST;
                last_battery_soc = result;
            }

            return last_battery_soc;
        }

        if (MAX_UP_POINT <= difference){
            result = last_battery_soc + MAX_DOWN_POINT;
            last_battery_soc = result;
            return result;
        }
    }

    return new_soc;
}
/* DTS2014082700221 caiwei 20140828 end> */

#define DEFAULT_CAPACITY	50
/* <DTS2014093003947 (report SOC 100%) caiwei 20141009 begin */
struct timespec g_last_ts;
#define TIME_INTERVAL_SECONDS (10)
/* DTS2014093003947 (report SOC 100%) caiwei 20141009 end> */
static int get_prop_capacity(struct qpnp_lbc_chip *chip)
{
	union power_supply_propval ret = {0,};
/* <DTS2014082700221 caiwei 20140828 begin */
	int soc, battery_status, charger_in, charging_current;

	int tmp_soc = 0;
    struct timespec ts;
    struct rtc_time tm;
/* <DTS2014093003947 (report SOC 100%) caiwei 20141009 begin */
    long int time_interval = 0;
/* DTS2014093003947 (report SOC 100%) caiwei 20141009 end> */
/* DTS2014082700221 caiwei 20140828 end> */
	if (chip->fake_battery_soc >= 0)
		return chip->fake_battery_soc;
	/* <DTS2014080100998  liyu 20140807 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	if (chip->cfg_use_fake_battery || (!get_prop_batt_present(chip)&&qpnp_lbc_is_usb_chg_plugged_in(chip)))
#else
	if (chip->cfg_use_fake_battery || !get_prop_batt_present(chip))
#endif
	/* DTS2014080100998  liyu 20140807 end> */
		return DEFAULT_CAPACITY;

	if (chip->bms_psy) {
		chip->bms_psy->get_property(chip->bms_psy,
				POWER_SUPPLY_PROP_CAPACITY, &ret);
		mutex_lock(&chip->chg_enable_lock);
		if (chip->chg_done)
			chip->bms_psy->get_property(chip->bms_psy,
					POWER_SUPPLY_PROP_CAPACITY, &ret);
		battery_status = get_prop_batt_status(chip);
		charger_in = qpnp_lbc_is_usb_chg_plugged_in(chip);
          /* <DTS2014071002612  mapengfei 20140710 begin */
#ifdef CONFIG_HUAWEI_KERNEL
		/* reset chg_done flag if capacity not 100% or cfg_soc_resume_charging*/
		if ((ret.intval < 100 || chip->cfg_soc_resume_charging)
							&& chip->chg_done) {
#endif
         /* DTS2014071002612  mapengfei 20140710 end> */
			chip->chg_done = false;
			power_supply_changed(&chip->batt_psy);
		}
		/* <DTS2014061002202 jiangfei 20140610 begin */
		if (battery_status != POWER_SUPPLY_STATUS_CHARGING
				&& charger_in
				&& !chip->cfg_charging_disabled
#ifdef CONFIG_HUAWEI_KERNEL
				&& !chip->resuming_charging
#endif
          /* <DTS2014071002612  mapengfei 20140710 begin */
#ifdef CONFIG_HUAWEI_KERNEL
            && chip->cfg_soc_resume_charging) {
#endif
         /* DTS2014071002612  mapengfei 20140710 end> */
			/* < DTS2014051304851 liyuping 20140513 begin */
			pr_info("resuming charging at %d%% soc\n",
					ret.intval);
			/* DTS2014051304851 liyuping 20140513 end > */
#ifdef CONFIG_HUAWEI_KERNEL
			chip->resuming_charging = true;
#endif
			/* DTS2014061002202 jiangfei 20140610 end> */
			if (!chip->cfg_disable_vbatdet_based_recharge)
				qpnp_lbc_vbatdet_override(chip, OVERRIDE_0);
			qpnp_lbc_charger_enable(chip, SOC, 1);
		}
		mutex_unlock(&chip->chg_enable_lock);

		soc = ret.intval;
		if (soc == 0) {
			if (!qpnp_lbc_is_usb_chg_plugged_in(chip))
				pr_warn_ratelimited("Batt 0, CHG absent\n");
		}
/* < DTS2014052901670 zhaoxiaoli 20140529 begin */
/* <DTS2014060603035 jiangfei 20140708 begin */
/* <DTS2014071002612  mapengfei 20140710 begin */
#ifdef CONFIG_HUAWEI_KERNEL
		if(fake_capacity != -1)
		{
			soc = fake_capacity;
		}
#endif
/* DTS2014071002612  mapengfei 20140710 end> */
/* DTS2014060603035 jiangfei 20140708 end> */
/* DTS2014052901670 zhaoxiaoli 20140529 end> */

/* DTS2014082700221 caiwei 20140828 begin */
    last_charging_state = charging_state;
    if (0 > smooth_step){
        smooth_step = 0;
    }
/* <DTS2014093003947 (report SOC 100%) caiwei 20141009 begin */
    if (bq2419x_check_charge_finished()){
        pr_debug("bq2419x_charge_finished bms cap is %d;force to 100", soc);
        soc = 100;
    }

    getnstimeofday(&ts); /*�õ�time_t���͵�UTCʱ��*/
    time_interval = (long int )ts.tv_sec - (long int )g_last_ts.tv_sec;
    pr_debug("time_interval:%ld \n", time_interval);
    if (abs(time_interval) <= TIME_INTERVAL_SECONDS){
        if (-1 == last_battery_soc){ //��δ��ʼ����
            pr_debug("last_battery_soc not initialed soc:%d \n", soc);
            return soc;
        }else{
            pr_debug("last_battery_soc is initialed soc:%d \n", last_battery_soc);
            return last_battery_soc;
        }
    }
/* DTS2014093003947 (report SOC 100%) caiwei 20141009 end> */
    charging_current = get_prop_current_now(chip);
    if(charging_current < 0) {
        charging_state = 1;
    } else {
		if((soc == 100) && (charging_current < 0)) {
            charging_state = 1;
        } else {
            charging_state = 0;
                }
    }

    if (0 == smooth_step){
        if (last_charging_state != charging_state){
            smooth_step = MAX_SMOOTH_STEP;
        }
    }

    if (soc >= 0){
        tmp_soc = soc;

        last_battery_soc = pad_smooth_soc(soc);
        if (0 != smooth_step){
            smooth_step--;
        }
        //getnstimeofday(&ts);
        rtc_time_to_tm(ts.tv_sec, &tm);
        /* < DTS2014082906164 gwx199358 20140829 begin */
        pr_debug("smooth_capacity ver 2.2 charging_state is %d  new_cap is %d, modified_cap is %d "
           "(%d-%02d-%02d %02d:%02d:%02d.%09lu UTC) \n",
        /* < DTS2014082906164 gwx199358 20140829 begin */
           charging_state, tmp_soc, last_battery_soc,
           tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
           tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);
    }
/* <DTS2014093003947 (report SOC 100%) caiwei 20141009 begin */
        getnstimeofday(&g_last_ts);
/* DTS2014093003947 (report SOC 100%) caiwei 20141009 end> */
        return last_battery_soc;
/* DTS2014082700221 caiwei 20140828 end> */
	} else {
		pr_debug("No BMS supply registered return %d\n",
							DEFAULT_CAPACITY);
	}

	/*
	 * Return default capacity to avoid userspace
	 * from shutting down unecessarily
	 */
	return DEFAULT_CAPACITY;
}

/*<DTS2014080707237 liyuping 20140807 begin */
#ifdef CONFIG_HUAWEI_KERNEL
#define DEFAULT_TEMP		90
#else
#define DEFAULT_TEMP		250
#endif
/* DTS2014080707237 liyuping 20140807 end> */
static int get_prop_batt_temp(struct qpnp_lbc_chip *chip)
{
	int rc = 0;
	struct qpnp_vadc_result results;
   /* <DTS2014110409521 caiwei 20141104 begin */
	union power_supply_propval ret = {0,};
   /* DTS2014110409521 caiwei 20141104 end> */
	if (chip->cfg_use_fake_battery || !get_prop_batt_present(chip))
		return DEFAULT_TEMP;

	rc = qpnp_vadc_read(chip->vadc_dev, LR_MUX1_BATT_THERM, &results);
	if (rc) {
		pr_debug("Unable to read batt temperature rc=%d\n", rc);
		return DEFAULT_TEMP;
	}
	pr_debug("get_bat_temp %d, %lld\n", results.adc_code,
							results.physical);
   /* <DTS2014110409521 caiwei 20141104 begin */
	if (chip->bms_psy)
		chip->bms_psy->get_property(chip->bms_psy,
			  POWER_SUPPLY_PROP_HOT_TEMP_TEST_STATUS, &ret);

	if (ret.intval)
		return DEFAULT_TEMP;
	else
	   return (int)results.physical;
   /* DTS2014110409521 caiwei 20141104 end> */
}

/* <DTS2014051601896 jiangfei 20140516 begin */
/* <DTS2014061002202 jiangfei 20140610 begin */
/* <DTS2014071001904 jiangfei 20140710 begin */
/*<DTS2014072301355 jiangfei 20140723 begin */
/*<DTS2014072501338 jiangfei 20140725 begin */
#ifdef CONFIG_HUAWEI_KERNEL
/*===========================================
FUNCTION: get_running_test_result
DESCRIPTION: For running test apk to get the running test result and status
IPNUT:	qpnp_lbc_chip *chip
RETURN:	a int value, we use bit0 to bit10 to tell running test apk the test 
result and status, if bit0 is 0, the result is fail, bit5 to bit11 is the failed reason
if bit0 is 1, the result is pass.
=============================================*/
static int get_running_test_result(struct qpnp_lbc_chip *chip)
{
	int result = 0;
	int cur_status = 0;
	int is_temp_vol_current_ok = 1;
	/*< DTS2014081101424  lishiliang 20140811 begin */
	int vol = 0, temp = 0, health = 0, current_ma =0, capacity;
	u8 usbin_valid_rt_sts = 0;
	/* DTS2014081101424  lishiliang 20140811 end> */
	int rc;
	int mode = 0;
	int error = 0;
	union power_supply_propval val = {0};

	/*<DTS2014121703947  xiongxi xwx234328 20141217 begin*/
	if(chip->use_other_charger_pad || !chip->use_other_charger){
	/*<DTS2014121703947  xiongxi xwx234328 20141217 end>*/
		/*Get battery props, for 8916 except G760 */
		cur_status = get_prop_batt_status(chip);
		current_ma = get_prop_current_now(chip);
		vol = get_prop_battery_voltage_now(chip);
		temp = get_prop_batt_temp(chip);
		capacity = get_prop_capacity(chip);
		health = get_prop_batt_health(chip);
	}else{
		/* Get battery props, for G760 */
		if(chip->ti_charger && chip->ti_charger->get_property){
			chip->ti_charger->get_property(chip->ti_charger,POWER_SUPPLY_PROP_STATUS,&val);
			cur_status = val.intval;
			chip->ti_charger->get_property(chip->ti_charger,POWER_SUPPLY_PROP_CURRENT_NOW,&val);
			current_ma = val.intval;
			chip->ti_charger->get_property(chip->ti_charger,POWER_SUPPLY_PROP_VOLTAGE_NOW,&val);
			vol = val.intval;
			chip->ti_charger->get_property(chip->ti_charger,POWER_SUPPLY_PROP_TEMP,&val);
			temp = val.intval;
			chip->ti_charger->get_property(chip->ti_charger,POWER_SUPPLY_PROP_CAPACITY,&val);
			capacity = val.intval;
			chip->ti_charger->get_property(chip->ti_charger,POWER_SUPPLY_PROP_HEALTH,&val);
			health = val.intval;
		}
		mode = is_bq24152_in_boost_mode();
	}
	pr_debug("get_running_test_result info: usb=%d batt_pres=%d batt_volt=%d batt_temp=%d"
				" cur_status=%d current_ma=%d setting status=%d\n",
				qpnp_lbc_is_usb_chg_plugged_in(chip),
				get_prop_batt_present(chip),
				vol,
				temp,
				cur_status,
				current_ma,
				chip->running_test_settled_status
				);

	if((CHARGE_OCP_THR > current_ma) || (BATTERY_OCP_THR < current_ma)){
		result |= OCP_ABNORML;
		is_temp_vol_current_ok = 0;
		pr_info("Find OCP! current_ma is %d\n", current_ma);
	}

	if((BATTERY_VOL_THR_HI < vol) || (BATTERY_VOL_THR_LO > vol)){
		result |= BATTERY_VOL_ABNORML;
		is_temp_vol_current_ok = 0;
		pr_info("Battery voltage is abnormal! voltage is %d\n", vol);
	}

	if((BATTERY_TEMP_HI < temp) || (BATTERY_TEMP_LO > temp)){
		result |= BATTERY_TEMP_ABNORML;
		is_temp_vol_current_ok = 0;
		pr_info("Battery temperature is abnormal! temp is %d\n", temp);
	}

	if(!is_temp_vol_current_ok){
		result |= CHARGE_STATUS_FAIL;
		pr_info("running test find abnormal battery status, the result is 0x%x\n", result);
		return result;
	}

	if(cur_status == chip->running_test_settled_status){
		result |= CHARGE_STATUS_PASS;
		return result;

	}else if((POWER_SUPPLY_STATUS_CHARGING == cur_status)
			&&(POWER_SUPPLY_STATUS_DISCHARGING == chip->running_test_settled_status)){
		/* <DTS2015020306355 xiongxi xwx234328 20150203 begin*/
		result |= CHARGE_STATUS_PASS;
		pr_info("The charger has been plugged in after running test writing 0 to charging_enabled\n");
		return result;
		/* DTS2015020306355 xiongxi xwx234328 20150203 end>*/

	}else if(POWER_SUPPLY_STATUS_CHARGING == chip->running_test_settled_status){
		if((POWER_SUPPLY_STATUS_DISCHARGING == cur_status)
			&& (BATT_FULL == capacity) && (get_prop_batt_present(chip))
				&& qpnp_lbc_is_usb_chg_plugged_in(chip)){
			cur_status = POWER_SUPPLY_STATUS_FULL;
		}

		if(POWER_SUPPLY_STATUS_FULL == cur_status){
			result |= BATTERY_FULL;
		}

		if(!qpnp_lbc_is_usb_chg_plugged_in(chip)){
			result |= USB_NOT_PRESENT;
		}

		if(chip->use_other_charger && (1 == mode)){
			result |= REGULATOR_BOOST;
		}

		if((vol >= ((chip->cfg_warm_bat_mv - WARM_VOL_BUFFER)*1000))
			&& (WARM_TEMP_THR <= temp)){
			result |= CHARGE_LIMIT;
		}

		if((POWER_SUPPLY_STATUS_NOT_CHARGING == cur_status)
			&& (HOT_TEMP_THR > temp)){
			result |= CHARGE_LIMIT;
			pr_info("settled_status = %d cur_status = %d temp = %d\n",
					chip->running_test_settled_status, cur_status, temp);
		}

		/*<DTS2014090201517 jiangfei 20140902 begin */
		/* G760 return discharging when stop charging in hot or cold environment */
		/* we use battery temperature to determine whether it pass or not */
		if((POWER_SUPPLY_STATUS_DISCHARGING == cur_status)
			&& chip->use_other_charger){
			if(((temp >= (chip->cfg_hot_bat_decidegc - COLD_HOT_TEMP_BUFFER))&&(HOT_TEMP_THR > temp))
				||(temp <= (chip->cfg_cold_bat_decidegc + COLD_HOT_TEMP_BUFFER))){
				result |= CHARGE_LIMIT;
				pr_info("settled_status %d cur_status=%d temp=%d "
						"chip->cfg_hot_bat_decidegc=%d "
						"chip->cfg_cold_bat_decidegc=%d\n",
						chip->running_test_settled_status, cur_status, temp,
						chip->cfg_hot_bat_decidegc,
						chip->cfg_cold_bat_decidegc);
			}
		}
		/* DTS2014090201517 jiangfei 20140902 end> */

		if((POWER_SUPPLY_HEALTH_OVERHEAT == health)
			|| (POWER_SUPPLY_HEALTH_COLD ==health)){
			result |= BATTERY_HEALTH;
		}

		/*<DTS2014121703947  xiongxi xwx234328 20141217 begin*/
		if(!chip->use_other_charger_pad){
			rc = qpnp_lbc_read(chip, chip->usb_chgpth_base + USB_PTH_STS_REG,
					&usbin_valid_rt_sts, 1);
			if (rc) {
				pr_err("spmi read failed: addr=%03X, rc=%d\n",
				chip->usb_chgpth_base + USB_PTH_STS_REG, rc);
			}else{
				if ((usbin_valid_rt_sts & USB_VALID_MASK)== USB_VALID_OVP_VALUE) {
					result |= CHARGER_OVP;
				}
			}
		}
		/*DTS2014121703947  xiongxi xwx234328 20141217 end>*/

		if(!get_prop_batt_present(chip)){
			result |= BATTERY_ABSENT;
		}

		if((POWER_SUPPLY_STATUS_UNKNOWN == cur_status)
			&& chip->use_other_charger){
			error = get_bq2415x_fault_status();
			if((ERROR_VBUS_OVP == error) ||(ERROR_BATT_OVP == error)
				||(ERROR_THERMAL_SHUTDOWN == error)){
				result |= BQ24152_FATAL_FAULT;
				pr_info("find bq24152 fatal fault! error = %d\n", error);
			}
		}
		
		/*<DTS2014121703947  xiongxi xwx234328 20141217 begin*/
		if((POWER_SUPPLY_STATUS_UNKNOWN == cur_status)
			&& chip->use_other_charger_pad){
			error = bq2419x_ovp_status_check();
			if((ERROR_BAT_FAULT == error) ||(ERROR_CHARGE_FAULT == error)){
				result |= CHARGER_OVP;
				pr_info("find bq2419x fatal fault! error = %d\n", error);
			}
		}
		/*DTS2014121703947  xiongxi xwx234328 20141217 end>*/

		if(result & PASS_MASK){
			result |= CHARGE_STATUS_PASS;
		}else{
			result |= CHARGE_STATUS_FAIL;
			pr_info("get_running_test_result: usb=%d batt_pres=%d batt_volt=%d batt_temp=%d result=0x%x\n",
					qpnp_lbc_is_usb_chg_plugged_in(chip),
					get_prop_batt_present(chip),
					vol,
					temp,
					result);
		}

		return result;
	}else{
		pr_info("other else status!");
		/* <DTS2014080801320 jiangfei 20140808 begin */
		/* if the setting status is discharging, meanwhile */
		/* if(cur_status != POWER_SUPPLY_STATUS_CHARGING*/
		/* && cur_status != POWER_SUPPLY_STATUS_DISCHARGING) */
		/* We return 1(PASS) directly, as when set discharging*/
		/* it do not need to care high temperature, battery full or unknow*/
		pr_info("get_running_test_result info: usb=%d batt_pres=%d batt_volt=%d batt_temp=%d"
				" cur_status=%d current_ma=%d setting status=%d\n",
				qpnp_lbc_is_usb_chg_plugged_in(chip),
				get_prop_batt_present(chip),
				vol,
				temp,
				cur_status,
				current_ma,
				chip->running_test_settled_status
				);
		return 1;
		/* DTS2014080801320 jiangfei 20140808 end> */
	}
}
/* DTS2014072501338 jiangfei 20140725 end> */
/* DTS2014072301355 jiangfei 20140723 end> */
/* DTS2014071001904 jiangfei 20140710 end> */
 /* <DTS2014062601697 jiangfei 20140626 begin */
/*===========================================
FUNCTION: get_prop_charge_log
DESCRIPTION: to get some pmic charging regs for chargelog.sh
IPNUT:	qpnp_lbc_chip *chip
RETURN:	a int value
chip->log_buf is used to save the regs values
=============================================*/
static int get_prop_charge_log(struct qpnp_lbc_chip *chip)
{
	int rc = 0;
	u8 bat_sts = 0, chg_sts = 0, usb_sts = 0, chg_ctrl = 0, usb_susp = 0;

	rc = qpnp_lbc_read(chip, INT_RT_STS(chip->bat_if_base), &bat_sts, 1);
	if (rc)
		pr_err("failed to read batt_sts rc=%d\n", rc);

	rc = qpnp_lbc_read(chip, INT_RT_STS(chip->chgr_base), &chg_sts, 1);
	if (rc)
		pr_err("failed to read chgr_sts rc=%d\n", rc);

	rc = qpnp_lbc_read(chip, INT_RT_STS(chip->usb_chgpth_base), &usb_sts, 1);
	if (rc)
		pr_err("failed to read usb_sts rc=%d\n", rc);

	rc = qpnp_lbc_read(chip, chip->chgr_base + CHG_CTRL_REG, &chg_ctrl, 1);
	if (rc)
		pr_err("failed to read chg_ctrl sts %d\n", rc);

	rc = qpnp_lbc_read(chip, chip->usb_chgpth_base + USB_SUSP_REG, &usb_susp, 1);
	if (rc)
		pr_err("failed to read usb_sts rc=%d\n", rc);

	usb_susp = usb_susp&0x1;

	snprintf(chip->log_buf, sizeof(chip->log_buf), "%d %u %u %u %u %u",
		chip->resuming_charging, bat_sts, chg_sts,
		usb_sts, chg_ctrl, usb_susp);

	return 0;
}

/* <DTS2014073007208 jiangfei 20140801 begin */
/*===========================================
FUNCTION: qpnp_lbc_is_in_vin_min_loop
DESCRIPTION: to get pmic8916 vin_min loop value
IPNUT:	qpnp_lbc_chip *chip
RETURN:	a int value, 1 means in vin_min loop; 0 means not in vin_min loop
=============================================*/
int qpnp_lbc_is_in_vin_min_loop(struct qpnp_lbc_chip *chip)
{
	/*< DTS2014081101424  lishiliang 20140811 begin */
	u8 reg_val = 0;
	/* DTS2014081101424  lishiliang 20140811 end> */
	int rc;

	rc = qpnp_lbc_read(chip, chip->chgr_base + CHG_STATUS_REG,
				&reg_val, 1);
	if (rc) {
		pr_err("Failed to read chg status rc=%d\n", rc);
		return rc;
	}

	pr_debug("CHG_STATUS_REG %x\n", reg_val);
	return (reg_val & CHG_VIN_MIN_LOOP_BIT) ? 1 : 0;
}
EXPORT_SYMBOL(qpnp_lbc_is_in_vin_min_loop);
/* DTS2014073007208 jiangfei 20140801 end> */
#endif
/* DTS2014062601697 jiangfei 20140626 end> */
/* DTS2014061002202 jiangfei 20140610 end> */
/* DTS2014051601896 jiangfei 20140516 end> */

static void qpnp_lbc_set_appropriate_current(struct qpnp_lbc_chip *chip)
{
	unsigned int chg_current = chip->usb_psy_ma;

	if (chip->bat_is_cool && chip->cfg_cool_bat_chg_ma)
		chg_current = min(chg_current, chip->cfg_cool_bat_chg_ma);
	if (chip->bat_is_warm && chip->cfg_warm_bat_chg_ma)
		chg_current = min(chg_current, chip->cfg_warm_bat_chg_ma);
	if (chip->therm_lvl_sel != 0 && chip->thermal_mitigation)
		chg_current = min(chg_current,
			chip->thermal_mitigation[chip->therm_lvl_sel]);
	/* <DTS2014080100998  liyu 20140807 begin */
	pr_info("setting charger current %d mA\n", chg_current);
	/* DTS2014080100998  liyu 20140807 end> */
	qpnp_lbc_ibatmax_set(chip, chg_current);
}

static void qpnp_batt_external_power_changed(struct power_supply *psy)
{
	struct qpnp_lbc_chip *chip = container_of(psy, struct qpnp_lbc_chip,
								batt_psy);
	union power_supply_propval ret = {0,};
	int current_ma;
	unsigned long flags;
	/*<DTS2014072301355 jiangfei 20140723 begin */
	int rc = 0;
	/* DTS2014072301355 jiangfei 20140723 end> */
	spin_lock_irqsave(&chip->ibat_change_lock, flags);
	if (!chip->bms_psy)
		chip->bms_psy = power_supply_get_by_name("bms");

	/* <DTS2014052906550 liyuping 20140530 begin */
	/*<DTS2014061605512 liyuping 20140617 begin */
	/*<DTS2014072301355 jiangfei 20140723 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	if (!chip->ti_charger && chip->use_other_charger)
		chip->ti_charger = power_supply_get_by_name("ti-charger");
	/*DTS2014091301574,begin:rm linear charger, 2014.9.30*/
	if (chip->ti_charger || chip->use_other_charger_pad){
	/*DTS2014091301574,end:rm linear charger, 2014.9.30*/
		rc = qpnp_lbc_charger_enable(chip, USER, 0);
		if (rc){
			pr_err("Failed to disable charging rc=%d\n",rc);
		}
	}
#endif
	/* DTS2014072301355 jiangfei 20140723 end> */
	/* DTS2014061605512 liyuping 20140617 end> */
	/* DTS2014052906550 liyuping 20140530 end> */
	if (qpnp_lbc_is_usb_chg_plugged_in(chip)) {
		chip->usb_psy->get_property(chip->usb_psy,
				POWER_SUPPLY_PROP_CURRENT_MAX, &ret);
		current_ma = ret.intval / 1000;

		/* <DTS2014052400953 chenyuanquan 20140524 begin */
#ifdef CONFIG_HUAWEI_KERNEL
		if (!factory_diag_flag && input_current_max_flag) {
			current_ma = max(current_ma, input_current_max_ma);
		}
		current_ma = min(current_ma, hot_design_current);
#endif
		/* DTS2014052400953 chenyuanquan 20140524 end> */

		if (current_ma == chip->prev_max_ma)
			goto skip_current_config;

		/* Disable charger in case of reset or suspend event */
		if (current_ma <= 2 && !chip->cfg_use_fake_battery
				&& get_prop_batt_present(chip)) {
			qpnp_lbc_charger_enable(chip, CURRENT, 0);
			chip->usb_psy_ma = QPNP_CHG_I_MAX_MIN_90;
			qpnp_lbc_set_appropriate_current(chip);
			/* <DTS2014052906550 liyuping 20140530 begin */
#ifdef CONFIG_HUAWEI_KERNEL
			if (chip->ti_charger)
				power_supply_set_current_limit(chip->ti_charger,0);
#endif
			/* DTS2014052906550 liyuping 20140530 end> */
		} else {
			chip->usb_psy_ma = current_ma;
			qpnp_lbc_set_appropriate_current(chip);
			qpnp_lbc_charger_enable(chip, CURRENT, 1);
			/* <DTS2014052906550 liyuping 20140530 begin */
#ifdef CONFIG_HUAWEI_KERNEL
			if (chip->ti_charger)
				power_supply_set_current_limit(chip->ti_charger,current_ma);
#endif
			/* DTS2014052906550 liyuping 20140530 end> */
		}
	}

skip_current_config:
	spin_unlock_irqrestore(&chip->ibat_change_lock, flags);
	pr_debug("power supply changed batt_psy\n");
	power_supply_changed(&chip->batt_psy);
}

/* <DTS2014052400953 chenyuanquan 20140524 begin */
/* <DTS2014062601697 jiangfei 20140626 begin */
#ifdef CONFIG_HUAWEI_KERNEL
/* get maximum current limit which is set to IBAT_MAX register */
static int qpnp_lbc_ibatmax_get(struct qpnp_lbc_chip *chip)
{
	int rc, iusbmax_ma;
	u8 iusbmax = 0;

	rc = qpnp_lbc_read(chip, chip->chgr_base + CHG_IBAT_MAX_REG, &iusbmax, 1);
	if (rc) {
		pr_err("failed to read IUSB_MAX rc=%d\n", rc);
		return 0;
	}

	iusbmax_ma = QPNP_LBC_IBATMAX_MIN + iusbmax * QPNP_LBC_I_STEP_MA;
	pr_debug(" = 0x%02x, iusbmax_ma = %d\n", iusbmax, iusbmax_ma);

	return iusbmax_ma;
}
/* DTS2014062601697 jiangfei 20140626 end> */
static int get_prop_full_design(struct qpnp_lbc_chip *chip)
{
	union power_supply_propval ret = {0,};

	if (chip->bms_psy) {
		chip->bms_psy->get_property(chip->bms_psy,
			POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN, &ret);
		return ret.intval;
	} else {
		pr_debug("No BMS supply registered return 0\n");
	}

	return 0;
}

#endif
/* DTS2014052400953 chenyuanquan 20140524 end> */

static int qpnp_lbc_system_temp_level_set(struct qpnp_lbc_chip *chip,
								int lvl_sel)
{
	int rc = 0;
	int prev_therm_lvl;
	unsigned long flags;

	if (!chip->thermal_mitigation) {
		pr_err("Thermal mitigation not supported\n");
		return -EINVAL;
	}

	if (lvl_sel < 0) {
		pr_err("Unsupported level selected %d\n", lvl_sel);
		return -EINVAL;
	}

	if (lvl_sel >= chip->cfg_thermal_levels) {
		pr_err("Unsupported level selected %d forcing %d\n", lvl_sel,
				chip->cfg_thermal_levels - 1);
		lvl_sel = chip->cfg_thermal_levels - 1;
	}

	if (lvl_sel == chip->therm_lvl_sel)
		return 0;

	spin_lock_irqsave(&chip->ibat_change_lock, flags);
	prev_therm_lvl = chip->therm_lvl_sel;
	chip->therm_lvl_sel = lvl_sel;
	if (chip->therm_lvl_sel == (chip->cfg_thermal_levels - 1)) {
		/* Disable charging if highest value selected by */
		rc = qpnp_lbc_charger_enable(chip, THERMAL, 0);
		if (rc < 0)
			dev_err(chip->dev,
				"Failed to set disable charging rc %d\n", rc);
		goto out;
	}

	qpnp_lbc_set_appropriate_current(chip);

	if (prev_therm_lvl == chip->cfg_thermal_levels - 1) {
		/*
		 * If previously highest value was selected charging must have
		 * been disabed. Enable charging.
		 */
		rc = qpnp_lbc_charger_enable(chip, THERMAL, 1);
		if (rc < 0) {
			dev_err(chip->dev,
				"Failed to enable charging rc %d\n", rc);
		}
	}
out:
	spin_unlock_irqrestore(&chip->ibat_change_lock, flags);
	return rc;
}

#define MIN_COOL_TEMP		-300
#define MAX_WARM_TEMP		1000
#define HYSTERISIS_DECIDEGC	20

static int qpnp_lbc_configure_jeita(struct qpnp_lbc_chip *chip,
			enum power_supply_property psp, int temp_degc)
{
	int rc = 0;

	if ((temp_degc < MIN_COOL_TEMP) || (temp_degc > MAX_WARM_TEMP)) {
		pr_err("Bad temperature request %d\n", temp_degc);
		return -EINVAL;
	}

	mutex_lock(&chip->jeita_configure_lock);
	switch (psp) {
	case POWER_SUPPLY_PROP_COOL_TEMP:
		if (temp_degc >=
			(chip->cfg_warm_bat_decidegc - HYSTERISIS_DECIDEGC)) {
			pr_err("Can't set cool %d higher than warm %d - hysterisis %d\n",
					temp_degc,
					chip->cfg_warm_bat_decidegc,
					HYSTERISIS_DECIDEGC);
			rc = -EINVAL;
			goto mutex_unlock;
		}
		if (chip->bat_is_cool)
			chip->adc_param.high_temp =
				temp_degc + HYSTERISIS_DECIDEGC;
		else if (!chip->bat_is_warm)
			chip->adc_param.low_temp = temp_degc;

		chip->cfg_cool_bat_decidegc = temp_degc;
		break;
	case POWER_SUPPLY_PROP_WARM_TEMP:
		if (temp_degc <=
		(chip->cfg_cool_bat_decidegc + HYSTERISIS_DECIDEGC)) {
			pr_err("Can't set warm %d higher than cool %d + hysterisis %d\n",
					temp_degc,
					chip->cfg_warm_bat_decidegc,
					HYSTERISIS_DECIDEGC);
			rc = -EINVAL;
			goto mutex_unlock;
		}
		if (chip->bat_is_warm)
			chip->adc_param.low_temp =
				temp_degc - HYSTERISIS_DECIDEGC;
		else if (!chip->bat_is_cool)
			chip->adc_param.high_temp = temp_degc;

		chip->cfg_warm_bat_decidegc = temp_degc;
		break;
	default:
		rc = -EINVAL;
		goto mutex_unlock;
	}

	if (qpnp_adc_tm_channel_measure(chip->adc_tm_dev, &chip->adc_param))
		pr_err("request ADC error\n");

mutex_unlock:
	mutex_unlock(&chip->jeita_configure_lock);
	return rc;
}

static int qpnp_batt_property_is_writeable(struct power_supply *psy,
					enum power_supply_property psp)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_CAPACITY:
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
	/* <DTS2014052400953 chenyuanquan 20140524 begin */
      /* <DTS2014071002612  mapengfei 20140710 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	case POWER_SUPPLY_PROP_FACTORY_DIAG:
       case POWER_SUPPLY_PROP_RESUME_CHARGING:
	case POWER_SUPPLY_PROP_INPUT_CURRENT_MAX:
	case POWER_SUPPLY_PROP_HOT_IBAT_LIMIT:
#endif
       /* DTS2014071002612  mapengfei 20140710 end> */
	/* DTS2014052400953 chenyuanquan 20140524 end> */
	case POWER_SUPPLY_PROP_COOL_TEMP:
	case POWER_SUPPLY_PROP_VOLTAGE_MIN:
	case POWER_SUPPLY_PROP_WARM_TEMP:
	case POWER_SUPPLY_PROP_SYSTEM_TEMP_LEVEL:
		return 1;
	default:
		break;
	}

	return 0;
}

/*
 * End of charge happens only when BMS reports the battery status as full. For
 * charging to end the s/w must put the usb path in suspend. Note that there
 * is no battery fet and usb path suspend is the only control to prevent any
 * current going in to the battery (and the system)
 * Charging can begin only when VBATDET comparator outputs 0. This indicates
 * that the battery is a at a lower voltage than 4% of the vddmax value.
 * S/W can override this comparator to output a favourable value - this is
 * used while resuming charging when the battery hasnt fallen below 4% but
 * the SOC has fallen below the resume threshold.
 *
 * In short, when SOC resume happens:
 * a. overide the comparator to output 0
 * b. enable charging
 *
 * When vbatdet based resume happens:
 * a. enable charging
 *
 * When end of charge happens:
 * a. disable the overrides in the comparator
 *    (may be from a previous soc resume)
 * b. disable charging
 */
static int qpnp_batt_power_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct qpnp_lbc_chip *chip = container_of(psy, struct qpnp_lbc_chip,
								batt_psy);
	int rc = 0;

	/* <DTS2014052400953 chenyuanquan 20140524 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	union power_supply_propval val_factory_diag = {0,};
#endif
	/* DTS2014052400953 chenyuanquan 20140524 end> */

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (val->intval == POWER_SUPPLY_STATUS_FULL &&
				!chip->cfg_float_charge) {
			mutex_lock(&chip->chg_enable_lock);

			/* Disable charging */
			rc = qpnp_lbc_charger_enable(chip, SOC, 0);
			if (rc)
				pr_err("Failed to disable charging rc=%d\n",
						rc);
			else
				chip->chg_done = true;

			/*
			 * Enable VBAT_DET based charging:
			 * To enable charging when VBAT falls below VBAT_DET
			 * and device stays suspended after EOC.
			 */
			if (!chip->cfg_disable_vbatdet_based_recharge) {
				/* No override for VBAT_DET_LO comp */
				rc = qpnp_lbc_vbatdet_override(chip,
							OVERRIDE_NONE);
				if (rc)
					pr_err("Failed to override VBAT_DET rc=%d\n",
							rc);
				else
					qpnp_lbc_enable_irq(chip,
						&chip->irqs[CHG_VBAT_DET_LO]);
			}

			mutex_unlock(&chip->chg_enable_lock);
		}
		break;
	case POWER_SUPPLY_PROP_COOL_TEMP:
		rc = qpnp_lbc_configure_jeita(chip, psp, val->intval);
		break;
	case POWER_SUPPLY_PROP_WARM_TEMP:
		rc = qpnp_lbc_configure_jeita(chip, psp, val->intval);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		chip->fake_battery_soc = val->intval;
		pr_debug("power supply changed batt_psy\n");
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		chip->cfg_charging_disabled = !(val->intval);
/* <DTS2014051601896 jiangfei 20140516 begin */
#ifdef CONFIG_HUAWEI_KERNEL
		pr_info("set charging_enabled value is %d\n",val->intval);
		if(chip->cfg_charging_disabled){
			chip->running_test_settled_status = POWER_SUPPLY_STATUS_DISCHARGING;
		}else{
			chip->running_test_settled_status = POWER_SUPPLY_STATUS_CHARGING;
		}
#endif
/* DTS2014051601896 jiangfei 20140516 end> */
		/*<DTS2014070404260 liyuping 20140704 begin */
#ifdef CONFIG_HUAWEI_KERNEL
        /*DTS2014091301574,begin:rm linear charger, 2014.9.30*/
        if(chip->use_other_charger_pad)
		{
		    /* DTS2014072806213 by l00220156 for factory interfaces 20140728 begin */
            rc = bq2419x_factory_set_charging_enabled(val->intval);
            pr_err("charging_enabled intval=%d\n", val->intval);
            if (rc)
                pr_err("Failed to disable charging rc=%d\n", rc);
            /* DTS2014072806213 by l00220156 for factory interfaces 20140728 end */
		}
		else if(!chip->use_other_charger)
		/*DTS2014091301574,end:rm linear charger, 2014.9.30*/
		{
			rc = qpnp_lbc_charger_enable(chip, USER,
							!chip->cfg_charging_disabled);
			
			if (rc)
				pr_err("Failed to disable charging rc=%d\n", rc);
		}
		else
		{
			val_factory_diag.intval = !chip->cfg_charging_disabled;
			rc = chip->ti_charger->set_property(chip->ti_charger,
				POWER_SUPPLY_PROP_CHARGING_ENABLED,&val_factory_diag);
			
			if (rc < 0)
				pr_err("Failed to disable charging rc=%d\n", rc);
		}
#endif
		/* DTS2014061605512 liyuping 20140617 end> */
					
		break;
	/* <DTS2014052400953 chenyuanquan 20140524 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	case POWER_SUPPLY_PROP_FACTORY_DIAG:
		factory_diag_flag = !(val->intval);
		/*<DTS2014061605512 liyuping 20140617 begin */
		/*DTS2014091301574,begin:rm linear charger, 2014.9.30*/
		if(!chip->use_other_charger || chip->use_other_charger_pad)
		/*DTS2014091301574,end:rm linear charger, 2014.9.30*/
		{
			if (factory_diag_flag) {
				factory_diag_last_current_ma = qpnp_lbc_ibatmax_get(chip);
				val_factory_diag.intval = QPNP_CHG_I_MAX_MIN_90 * 1000;
				chip->usb_psy->set_property(chip->usb_psy,
						POWER_SUPPLY_PROP_CURRENT_MAX, &val_factory_diag);
			} else {
				if (factory_diag_last_current_ma) {
					val_factory_diag.intval = factory_diag_last_current_ma * 1000;
					chip->usb_psy->set_property(chip->usb_psy,
						POWER_SUPPLY_PROP_CURRENT_MAX, &val_factory_diag);

				}
				factory_diag_last_current_ma = 0;
				}
		}
		else
		{
			/*<DTS2014072301355 jiangfei 20140723 begin */
			if(chip->ti_charger && chip->ti_charger->set_property){
				/*<DTS2014062506205 liyuping 20140625 begin */		
				val_factory_diag.intval = QPNP_CHG_I_MAX_MIN_90;
				chip->ti_charger->set_property(chip->ti_charger,POWER_SUPPLY_PROP_CURRENT_MAX,&val_factory_diag);
			}
			/* DTS2014072301355 jiangfei 20140723 end> */
				/* DTS2014062506205 liyuping 20140625 end> */
		}
		/* DTS2014061605512 liyuping 20140617 end> */
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_MAX:
		if (val->intval) {
			input_current_max_ma = val->intval / 1000;
			input_current_max_flag = 1;
		} else {
			input_current_max_flag = 0;
		}
		break;
	/* < DTS2014080705190 yuanzhen 20140807 begin */
	case POWER_SUPPLY_PROP_HOT_IBAT_LIMIT:
		if (val->intval) {
			hot_design_current = val->intval;
		} else {
			hot_design_current = HOT_DESIGN_MAX_CURRENT;
		}
		break;
	/* DTS2014080705190 yuanzhen 20140807 end > */
#endif
	/* DTS2014052400953 chenyuanquan 20140524 end> */
	case POWER_SUPPLY_PROP_VOLTAGE_MIN:
		qpnp_lbc_vinmin_set(chip, val->intval / 1000);
		break;
	case POWER_SUPPLY_PROP_SYSTEM_TEMP_LEVEL:
		qpnp_lbc_system_temp_level_set(chip, val->intval);
		break;
    /* <DTS2014071002612  mapengfei 20140710 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	case POWER_SUPPLY_PROP_RESUME_CHARGING:
		chip->cfg_soc_resume_charging = val->intval;
		break;
#endif
    /* DTS2014071002612  mapengfei 20140710 end> */
	default:
		return -EINVAL;
	}

	/* <DTS2014052400953 chenyuanquan 20140524 begin */
	/* <DTS2014070908384 chenyuanquan 20140710 begin */
    /* <DTS2014071002612  mapengfei 20140710 begin */
     if( POWER_SUPPLY_PROP_RESUME_CHARGING !=psp)
     {
#ifdef CONFIG_HUAWEI_KERNEL
	qpnp_batt_external_power_changed(&chip->batt_psy);
#endif
	/* DTS2014070908384 chenyuanquan 20140710 end> */
	/* DTS2014052400953 chenyuanquan 20140524 end> */
           power_supply_changed(&chip->batt_psy);
       }
     /* DTS2014071002612  mapengfei 20140710 end> */
	return rc;
}

static int qpnp_batt_power_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct qpnp_lbc_chip *chip =
		container_of(psy, struct qpnp_lbc_chip, batt_psy);
	/* <DTS2014060603035 jiangfei 20140708 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	int batt_level = 0;
#endif
	/* DTS2014060603035 jiangfei 20140708 end> */
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = get_prop_batt_status(chip);
		/* <DTS2014060603035 jiangfei 20140708 begin */
#ifdef CONFIG_HUAWEI_KERNEL
		batt_level = get_prop_capacity(chip);
             /* <DTS2014111002912 caiwei 20141112 begin */
		if(BATT_FULL_LEVEL == batt_level && qpnp_lbc_is_usb_chg_plugged_in(chip) && !bq2419x_otg_status_check()){
             /* DTS2014111002912 caiwei 20141112 end> */
			val->intval = POWER_SUPPLY_STATUS_FULL;
		}
#endif
		/* DTS2014060603035 jiangfei 20140708 end> */
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = get_prop_charge_type(chip);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = get_prop_batt_health(chip);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = get_prop_batt_present(chip);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = chip->cfg_max_voltage_mv * 1000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = chip->cfg_min_voltage_mv * 1000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = get_prop_battery_voltage_now(chip);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		/*<DTS2014080707237 liyuping 20140807 begin */
#ifdef CONFIG_HUAWEI_KERNEL
        /*DTS2014091301574,begin:rm linear charger, 2014.9.30*/
		if(!chip->use_other_charger || chip->use_other_charger_pad)
		/*DTS2014091301574,end:rm linear charger, 2014.9.30*/
		{
			val->intval = get_prop_batt_temp(chip);
		}
		else if (chip->ti_charger && chip->ti_charger->get_property)
		{
			chip->ti_charger->get_property(chip->ti_charger,POWER_SUPPLY_PROP_TEMP,val);
		}
		else
		{
			val->intval = DEFAULT_TEMP;
		}
#endif
		/* DTS2014080707237 liyuping 20140807 end> */
		break;
	case POWER_SUPPLY_PROP_COOL_TEMP:
		val->intval = chip->cfg_cool_bat_decidegc;
		break;
	case POWER_SUPPLY_PROP_WARM_TEMP:
		val->intval = chip->cfg_warm_bat_decidegc;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		/*<DTS2014070404260 liyuping 20140704 begin */
#ifdef CONFIG_HUAWEI_KERNEL
		if(!chip->use_other_charger || chip->use_other_charger_pad)
		{
			val->intval = get_prop_capacity(chip);
		}
		else if (chip->ti_charger && chip->ti_charger->get_property)
		{
			chip->ti_charger->get_property(chip->ti_charger,POWER_SUPPLY_PROP_CAPACITY,val);
		}
		/*<DTS2014080707237 liyuping 20140807 begin */
		else
		{
			val->intval = DEFAULT_CAPACITY;
		}
		/* DTS2014080707237 liyuping 20140807 end> */
#endif
		/* DTS2014070404260 liyuping 20140704 end> */
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = get_prop_current_now(chip);
		break;
    /* DTS2014072806213 by l00220156 for factory interfaces 20140728 begin */    
    case POWER_SUPPLY_PROP_CURRENT_REALTIME:
        val->intval = 0 - get_prop_current_now(chip)/1000; /* DTS2014072806213 by l00220156 for mmi_current direction */
        break;
    /* DTS2014072806213 by l00220156 for factory interfaces 20140728 end */    
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		val->intval = !(chip->cfg_charging_disabled);
		break;
	/* <DTS2014052400953 chenyuanquan 20140524 begin */
	/* <DTS2014061002202 jiangfei 20140610 begin */
	/*<DTS2014080807172 jiangfei 20140813 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	case POWER_SUPPLY_PROP_FACTORY_DIAG:
		val->intval = !(factory_diag_flag);
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_MAX:
		val->intval = qpnp_lbc_ibatmax_get(chip) * 1000;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		val->intval = get_prop_full_design(chip);
		break;
	case POWER_SUPPLY_PROP_HOT_IBAT_LIMIT:
		val->intval = hot_design_current;
		break;
	case POWER_SUPPLY_PROP_CHARGE_LOG:
		get_prop_charge_log(chip);
		val->strval = chip->log_buf;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LIPO;
		break;
#endif
	/* DTS2014080807172 jiangfei 20140813 end> */
	/* DTS2014061002202 jiangfei 20140610 end> */
	/* DTS2014052400953 chenyuanquan 20140524 end> */
	case POWER_SUPPLY_PROP_SYSTEM_TEMP_LEVEL:
		val->intval = chip->therm_lvl_sel;
		break;
/* <DTS2014051601896 jiangfei 20140516 begin */
/*<DTS2014072501338 jiangfei 20140725 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	case POWER_SUPPLY_PROP_RUNNING_TEST_STATUS:
		val->intval = get_running_test_result(chip);
		break;
#endif
/* DTS2014072501338 jiangfei 20140725 end> */
/* DTS2014051601896 jiangfei 20140516 end> */
    /* <DTS2014071002612  mapengfei 20140710 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	case POWER_SUPPLY_PROP_RESUME_CHARGING:
             val->intval = chip->cfg_soc_resume_charging;
        break;
#endif
    /* DTS2014071002612  mapengfei 20140710 end> */
	default:
		return -EINVAL;
	}

	return 0;
}

/* <DTS2014042804721 chenyuanquan 20140428 begin */
#ifdef CONFIG_HUAWEI_KERNEL
#define HYSTERISIS_DECIDEGC 20
/*====================================================================================
FUNCTION: hw_tm_warm_notification_zone

DESCRIPTION:	when qpnp_tm_state is warm, call this function.to acquire the
				temperature zone type

INPUT:	temperature,struct qpnp_lbc_chip *chip
OUTPUT: NULL
RETURN: hw_high_low_temp_configure_type

======================================================================================*/
static int hw_tm_warm_notification_zone(int temp,struct qpnp_lbc_chip *chip)
{
	if(chip == NULL)
	{
		pr_err("chip is null \n");
		return UNKNOW_ZONE;
	}
	if(temp > chip->cfg_cold_bat_decidegc + HYSTERISIS_DECIDEGC
		&& temp <= chip->cfg_cool_bat_decidegc + HYSTERISIS_DECIDEGC)
	{
		return COLD_COOL_ZONE;
	}
	else if(temp > chip->cfg_cool_bat_decidegc + HYSTERISIS_DECIDEGC
		&& temp <= chip->cfg_warm_bat_decidegc)
	{
		return COOL_WARM_ZONE;
	}
	else if(temp > chip->cfg_warm_bat_decidegc
		&& temp <= chip->cfg_hot_bat_decidegc)
	{
		return WARM_HOT_ZONE;
	}
	else if(temp > chip->cfg_hot_bat_decidegc)
	{
		return HOT_HOT_ZONE;
	}
	else
	{
		pr_err("warm notification error,temp is %d \n",temp);
		return UNKNOW_ZONE;
	}
}
/*====================================================================================
FUNCTION: hw_tm_cool_notification_zone

DESCRIPTION:	when qpnp_tm_state is cool, call this function.to acquire the
				temperature zone type

INPUT:	temperature,struct qpnp_lbc_chip *chip
OUTPUT: NULL
RETURN: hw_high_low_temp_configure_type

======================================================================================*/
static int hw_tm_cool_notification_zone(int temp,struct qpnp_lbc_chip *chip)
{
	if(chip == NULL)
	{
		pr_err("chip is null \n");
		return UNKNOW_ZONE;
	}
	if(temp < chip->cfg_cold_bat_decidegc)
	{
		return COLD_COLD_ZONE;
	}
	else if(temp >= chip->cfg_cold_bat_decidegc
		&& temp < chip->cfg_cool_bat_decidegc)
	{
		return COLD_COOL_ZONE;
	}
	else if(temp >= chip->cfg_cool_bat_decidegc
		&& temp < chip->cfg_warm_bat_decidegc - HYSTERISIS_DECIDEGC)
	{
		return COOL_WARM_ZONE;
	}
	else if(temp >= chip->cfg_warm_bat_decidegc - HYSTERISIS_DECIDEGC
		&& temp < chip->cfg_hot_bat_decidegc - HYSTERISIS_DECIDEGC)
	{
		return WARM_HOT_ZONE;
	}
	else
	{
		pr_err("cold notification error,temp is %d\n",temp);
		return UNKNOW_ZONE;
	}
}
/* <DTS2014062601697 jiangfei 20140626 begin */
/*====================================================================================
FUNCTION: hw_tm_set_configure

DESCRIPTION:	according the temperature zone type to set voltage,current,adc_param
				which is set to alarm ,and decided to enable charging or not

INPUT:	enum hw_high_low_temp_configure_type zone,struct qpnp_lbc_chip *chip
OUTPUT: NULL
RETURN: NULL

======================================================================================*/
static void hw_tm_set_configure(enum hw_high_low_temp_configure_type zone,struct qpnp_lbc_chip *chip)
{
	bool bat_warm = 0, bat_cool = 0,bad_temp;
	int rc;
	if(chip == NULL)
	{
		pr_err("chip is null \n");
		return ;
	}
	pr_debug("temperature zone type %d\n",zone);
	switch(zone){
	case COLD_COLD_ZONE:
		chip->adc_param.low_temp = chip->cfg_cold_bat_decidegc;
		chip->adc_param.high_temp = chip->cfg_cold_bat_decidegc + HYSTERISIS_DECIDEGC;
		chip->adc_param.state_request = ADC_TM_WARM_THR_ENABLE;
		bat_cool = true;
		bat_warm = false;
		bad_temp = true;
		break;
	case COLD_COOL_ZONE:
		chip->adc_param.low_temp = chip->cfg_cold_bat_decidegc;
		chip->adc_param.high_temp = chip->cfg_cool_bat_decidegc + HYSTERISIS_DECIDEGC;
		chip->adc_param.state_request = ADC_TM_HIGH_LOW_THR_ENABLE;
		bat_cool = true;
		bat_warm = false;
		bad_temp =false;
		break;
	case COOL_WARM_ZONE:
		chip->adc_param.low_temp = chip->cfg_cool_bat_decidegc;
		chip->adc_param.high_temp = chip->cfg_warm_bat_decidegc;
		chip->adc_param.state_request = ADC_TM_HIGH_LOW_THR_ENABLE;
		bat_cool = false;
		bat_warm = false;
		bad_temp =false;
		break;
	case WARM_HOT_ZONE:
		chip->adc_param.low_temp = chip->cfg_warm_bat_decidegc - HYSTERISIS_DECIDEGC;
		chip->adc_param.high_temp = chip->cfg_hot_bat_decidegc;
		chip->adc_param.state_request = ADC_TM_HIGH_LOW_THR_ENABLE;
		bat_cool = false;
		bat_warm = true;
		bad_temp = false;
		break;
	case HOT_HOT_ZONE:
		chip->adc_param.low_temp = chip->cfg_hot_bat_decidegc - HYSTERISIS_DECIDEGC;
		chip->adc_param.high_temp = chip->cfg_hot_bat_decidegc;
		chip->adc_param.state_request = ADC_TM_COOL_THR_ENABLE;
		bat_cool = false;
		bat_warm = true;
		bad_temp = true;
		break;
	default:
		chip->adc_param.low_temp = chip->cfg_cool_bat_decidegc;
		chip->adc_param.high_temp = chip->cfg_warm_bat_decidegc;
		chip->adc_param.state_request = ADC_TM_HIGH_LOW_THR_ENABLE;
		bat_cool = false;
		bat_warm = false;
		bad_temp = false;
		break;
	}
	if(bad_temp_flag ^ bad_temp)
	{

		bad_temp_flag= bad_temp;
		pr_info("bad_temp_flag is %d,qpnp_chg_charge_en is %d\n",bad_temp_flag,!bad_temp_flag);
		rc = qpnp_lbc_charger_enable(chip, THERMAL, !bad_temp_flag);
		if (rc){
			pr_err("Failed to disable/enable charging rc=%d\n", rc);
		}
	}

	if (chip->bat_is_cool ^ bat_cool || chip->bat_is_warm ^ bat_warm)
	{
		chip->bat_is_cool = bat_cool;
		chip->bat_is_warm = bat_warm;

		/**
		 * set appropriate voltages and currents.
		 *
		 * Note that when the battery is hot or cold, the charger
		 * driver will not resume with SoC. Only vbatdet is used to
		 * determine resume of charging.
		 */
		qpnp_lbc_set_appropriate_vddmax(chip);
		qpnp_lbc_set_appropriate_current(chip);
//		qpnp_chg_set_appropriate_vbatdet(chip);
	}

	pr_debug("warm %d, cool %d, low = %d deciDegC, high = %d deciDegC ,hot = %d ,warm = %d , cool = %d , cold = %d \n",
			chip->bat_is_warm, chip->bat_is_cool,chip->adc_param.low_temp, chip->adc_param.high_temp,
			chip->cfg_hot_bat_decidegc,chip->cfg_warm_bat_decidegc,chip->cfg_cool_bat_decidegc,chip->cfg_cold_bat_decidegc);

	if (qpnp_adc_tm_channel_measure(chip->adc_tm_dev, &chip->adc_param))
		pr_err("request ADC error\n");
}
/* DTS2014062601697 jiangfei 20140626 end> */
static void qpnp_lbc_jeita_adc_notification(enum qpnp_tm_state state, void *ctx)
{
	struct qpnp_lbc_chip *chip = ctx;
	int temp;
	enum hw_high_low_temp_configure_type temp_zone_type = UNKNOW_ZONE;

	if (state >= ADC_TM_STATE_NUM) {
		pr_err("invalid notification %d\n", state);
		return;
	}

	temp = get_prop_batt_temp(chip);

	pr_debug("temp = %d state = %s\n", temp,
			state == ADC_TM_WARM_STATE ? "warm" : "cool");
	if(state == ADC_TM_WARM_STATE)
	{
		temp_zone_type = hw_tm_warm_notification_zone(temp, chip);
	}
	else
	{
		temp_zone_type = hw_tm_cool_notification_zone(temp, chip);
	}
	/* <DTS2014062508292 sunwenyong 20140625 begin */
	/* < DTS2014061009305 zhaoxiaoli 20140611 begin */
	if(DISABLE_SOFTWARE_HARDWARE_TEMP_CONTROL == charge_no_limit
		|| DISABLE_SOFTWARE_TEMP_CONTROL == charge_no_limit)
	{
		pr_info("error,software temp control has been canceled\n");
		temp_zone_type = COOL_WARM_ZONE;
	}
	/* DTS2014061009305 zhaoxiaoli 20140611 end> */
	/* DTS2014062508292 sunwenyong 20140625 end> */
	hw_tm_set_configure(temp_zone_type, chip);
}
#else
static void qpnp_lbc_jeita_adc_notification(enum qpnp_tm_state state, void *ctx)
{
	struct qpnp_lbc_chip *chip = ctx;
	bool bat_warm = 0, bat_cool = 0;
	int temp;
	unsigned long flags;

	if (state >= ADC_TM_STATE_NUM) {
		pr_err("invalid notification %d\n", state);
		return;
	}

	temp = get_prop_batt_temp(chip);

	pr_debug("temp = %d state = %s\n", temp,
			state == ADC_TM_WARM_STATE ? "warm" : "cool");

	if (state == ADC_TM_WARM_STATE) {
		if (temp >= chip->cfg_warm_bat_decidegc) {
			/* Normal to warm */
			bat_warm = true;
			bat_cool = false;
			chip->adc_param.low_temp =
					chip->cfg_warm_bat_decidegc
					- HYSTERISIS_DECIDEGC;
			chip->adc_param.state_request =
				ADC_TM_COOL_THR_ENABLE;
		} else if (temp >=
			chip->cfg_cool_bat_decidegc + HYSTERISIS_DECIDEGC) {
			/* Cool to normal */
			bat_warm = false;
			bat_cool = false;

			chip->adc_param.low_temp =
					chip->cfg_cool_bat_decidegc;
			chip->adc_param.high_temp =
					chip->cfg_warm_bat_decidegc;
			chip->adc_param.state_request =
					ADC_TM_HIGH_LOW_THR_ENABLE;
		}
	} else {
		if (temp <= chip->cfg_cool_bat_decidegc) {
			/* Normal to cool */
			bat_warm = false;
			bat_cool = true;
			chip->adc_param.high_temp =
					chip->cfg_cool_bat_decidegc
					+ HYSTERISIS_DECIDEGC;
			chip->adc_param.state_request =
					ADC_TM_WARM_THR_ENABLE;
		} else if (temp <= (chip->cfg_warm_bat_decidegc -
					HYSTERISIS_DECIDEGC)){
			/* Warm to normal */
			bat_warm = false;
			bat_cool = false;

			chip->adc_param.low_temp =
					chip->cfg_cool_bat_decidegc;
			chip->adc_param.high_temp =
					chip->cfg_warm_bat_decidegc;
			chip->adc_param.state_request =
					ADC_TM_HIGH_LOW_THR_ENABLE;
		}
	}

	if (chip->bat_is_cool ^ bat_cool || chip->bat_is_warm ^ bat_warm) {
		spin_lock_irqsave(&chip->ibat_change_lock, flags);
		chip->bat_is_cool = bat_cool;
		chip->bat_is_warm = bat_warm;
		qpnp_lbc_set_appropriate_vddmax(chip);
		qpnp_lbc_set_appropriate_current(chip);
		spin_unlock_irqrestore(&chip->ibat_change_lock, flags);
	}

	pr_debug("warm %d, cool %d, low = %d deciDegC, high = %d deciDegC\n",
			chip->bat_is_warm, chip->bat_is_cool,
			chip->adc_param.low_temp, chip->adc_param.high_temp);

	if (qpnp_adc_tm_channel_measure(chip->adc_tm_dev, &chip->adc_param))
		pr_err("request ADC error\n");
}
#endif
/* DTS2014042804721 chenyuanquan 20140428 end> */

#define IBAT_TERM_EN_MASK		BIT(3)
static int qpnp_lbc_chg_init(struct qpnp_lbc_chip *chip)
{
	int rc;
	u8 reg_val;

	qpnp_lbc_vbatweak_set(chip, chip->cfg_batt_weak_voltage_uv);
	rc = qpnp_lbc_vinmin_set(chip, chip->cfg_min_voltage_mv);
	if (rc) {
		pr_err("Failed  to set  vin_min rc=%d\n", rc);
		return rc;
	}
	rc = qpnp_lbc_vddsafe_set(chip, chip->cfg_safe_voltage_mv);
	if (rc) {
		pr_err("Failed to set vdd_safe rc=%d\n", rc);
		return rc;
	}
	rc = qpnp_lbc_vddmax_set(chip, chip->cfg_max_voltage_mv);
	if (rc) {
		pr_err("Failed to set vdd_safe rc=%d\n", rc);
		return rc;
	}
	rc = qpnp_lbc_ibatsafe_set(chip, chip->cfg_safe_current);
	if (rc) {
		pr_err("Failed to set ibat_safe rc=%d\n", rc);
		return rc;
	}

	if (of_property_read_bool(chip->spmi->dev.of_node, "qcom,tchg-mins")) {
		rc = qpnp_lbc_tchg_max_set(chip, chip->cfg_tchg_mins);
		if (rc) {
			pr_err("Failed to set tchg_mins rc=%d\n", rc);
			return rc;
		}
	}

	/*
	 * Override VBAT_DET comparator to enable charging
	 * irrespective of VBAT above VBAT_DET.
	 */
	rc = qpnp_lbc_vbatdet_override(chip, OVERRIDE_0);
	if (rc) {
		pr_err("Failed to override comp rc=%d\n", rc);
		return rc;
	}

	/*
	 * Disable iterm comparator of linear charger to disable charger
	 * detecting end of charge condition based on DT configuration
	 * and float charge configuration.
	 */
	if (!chip->cfg_charger_detect_eoc || chip->cfg_float_charge) {
		rc = qpnp_lbc_masked_write(chip,
				chip->chgr_base + CHG_IBATTERM_EN_REG,
				IBAT_TERM_EN_MASK, 0);
		if (rc) {
			pr_err("Failed to disable EOC comp rc=%d\n", rc);
			return rc;
		}
	}

	/* Disable charger watchdog */
	reg_val = 0;
	rc = qpnp_lbc_write(chip, chip->chgr_base + CHG_WDOG_EN_REG,
				&reg_val, 1);

	return rc;
}

static int qpnp_lbc_bat_if_init(struct qpnp_lbc_chip *chip)
{
	u8 reg_val;
	int rc;

	/* Select battery presence detection */
	switch (chip->cfg_bpd_detection) {
	case BPD_TYPE_BAT_THM:
		reg_val = BATT_THM_EN;
		break;
	case BPD_TYPE_BAT_ID:
		reg_val = BATT_ID_EN;
		break;
	case BPD_TYPE_BAT_THM_BAT_ID:
		reg_val = BATT_THM_EN | BATT_ID_EN;
		break;
	default:
		reg_val = BATT_THM_EN;
		break;
	}

	rc = qpnp_lbc_masked_write(chip,
			chip->bat_if_base + BAT_IF_BPD_CTRL_REG,
			BATT_BPD_CTRL_SEL_MASK, reg_val);
	if (rc) {
		pr_err("Failed to choose BPD rc=%d\n", rc);
		return rc;
	}

	/* Force on VREF_BAT_THM */
	reg_val = VREF_BATT_THERM_FORCE_ON;
	rc = qpnp_lbc_write(chip,
			chip->bat_if_base + BAT_IF_VREF_BAT_THM_CTRL_REG,
			&reg_val, 1);
	if (rc) {
		pr_err("Failed to force on VREF_BAT_THM rc=%d\n", rc);
		return rc;
	}

	return 0;
}

static int qpnp_lbc_usb_path_init(struct qpnp_lbc_chip *chip)
{
	int rc;
	u8 reg_val;

	if (qpnp_lbc_is_usb_chg_plugged_in(chip)) {
		reg_val = 0;
		rc = qpnp_lbc_write(chip,
			chip->usb_chgpth_base + CHG_USB_ENUM_T_STOP_REG,
			&reg_val, 1);
		if (rc) {
			pr_err("Failed to write enum stop rc=%d\n", rc);
			return -ENXIO;
		}
	}

	if (chip->cfg_charging_disabled) {
		rc = qpnp_lbc_charger_enable(chip, USER, 0);
		if (rc)
			pr_err("Failed to disable charging rc=%d\n", rc);
	} else {
		/*
		 * Enable charging explictly,
		 * because not sure the default behavior.
		 */
		/* < DTS2014080200746 yuanzhen 20140802 begin */
#ifdef CONFIG_HUAWEI_KERNEL
        /*DTS2014091301574,begin:rm linear charger, 2014.9.30*/
		if (!chip->use_other_charger && !chip->use_other_charger_pad) {
		/*DTS2014091301574,end:rm linear charger, 2014.9.30*/
			reg_val = CHG_ENABLE;
			rc = qpnp_lbc_masked_write(chip, chip->chgr_base + CHG_CTRL_REG,
						CHG_EN_MASK, reg_val);
			if (rc)
				pr_err("Failed to enable charger rc=%d\n", rc);
		}
#else
		reg_val = CHG_ENABLE;
		rc = qpnp_lbc_masked_write(chip, chip->chgr_base + CHG_CTRL_REG,
					CHG_EN_MASK, reg_val);
		if (rc)
			pr_err("Failed to enable charger rc=%d\n", rc);
#endif
		/* DTS2014080200746 yuanzhen 20140802 end > */
	}

	return rc;
}

#define LBC_MISC_DIG_VERSION_1			0x01
static int qpnp_lbc_misc_init(struct qpnp_lbc_chip *chip)
{
	int rc;
	u8 reg_val, reg_val1, trim_center;

	/* Check if this LBC MISC version supports VDD trimming */
	rc = qpnp_lbc_read(chip, chip->misc_base + MISC_REV2_REG,
			&reg_val, 1);
	if (rc) {
		pr_err("Failed to read VDD_EA TRIM3 reg rc=%d\n", rc);
		return rc;
	}

	if (reg_val >= LBC_MISC_DIG_VERSION_1) {
		chip->supported_feature_flag |= VDD_TRIM_SUPPORTED;
		/* Read initial VDD trim value */
		rc = qpnp_lbc_read(chip, chip->misc_base + MISC_TRIM3_REG,
				&reg_val, 1);
		if (rc) {
			pr_err("Failed to read VDD_EA TRIM3 reg rc=%d\n", rc);
			return rc;
		}

		rc = qpnp_lbc_read(chip, chip->misc_base + MISC_TRIM4_REG,
				&reg_val1, 1);
		if (rc) {
			pr_err("Failed to read VDD_EA TRIM3 reg rc=%d\n", rc);
			return rc;
		}

		trim_center = ((reg_val & MISC_TRIM3_VDD_MASK)
					>> VDD_TRIM3_SHIFT)
					| ((reg_val1 & MISC_TRIM4_VDD_MASK)
					>> VDD_TRIM4_SHIFT);
		chip->init_trim_uv = qpnp_lbc_get_trim_voltage(trim_center);
		chip->delta_vddmax_uv = chip->init_trim_uv;
		pr_debug("Initial trim center %x trim_uv %d\n",
				trim_center, chip->init_trim_uv);
	}

	pr_debug("Setting BOOT_DONE\n");
	reg_val = MISC_BOOT_DONE;
	rc = qpnp_lbc_write(chip, chip->misc_base + MISC_BOOT_DONE_REG,
				&reg_val, 1);

	return rc;
}

#define OF_PROP_READ(chip, prop, qpnp_dt_property, retval, optional)	\
do {									\
	if (retval)							\
		break;							\
									\
	retval = of_property_read_u32(chip->spmi->dev.of_node,		\
					"qcom," qpnp_dt_property,	\
					&chip->prop);			\
									\
	if ((retval == -EINVAL) && optional)				\
		retval = 0;						\
	else if (retval)						\
		pr_err("Error reading " #qpnp_dt_property		\
				" property rc = %d\n", rc);		\
} while (0)

static int qpnp_charger_read_dt_props(struct qpnp_lbc_chip *chip)
{
	int rc = 0;
	const char *bpd;

	OF_PROP_READ(chip, cfg_max_voltage_mv, "vddmax-mv", rc, 0);
	OF_PROP_READ(chip, cfg_safe_voltage_mv, "vddsafe-mv", rc, 0);
	OF_PROP_READ(chip, cfg_min_voltage_mv, "vinmin-mv", rc, 0);
	OF_PROP_READ(chip, cfg_safe_current, "ibatsafe-ma", rc, 0);
	if (rc)
		pr_err("Error reading required property rc=%d\n", rc);

	OF_PROP_READ(chip, cfg_tchg_mins, "tchg-mins", rc, 1);
	OF_PROP_READ(chip, cfg_warm_bat_decidegc, "warm-bat-decidegc", rc, 1);
	OF_PROP_READ(chip, cfg_cool_bat_decidegc, "cool-bat-decidegc", rc, 1);
	OF_PROP_READ(chip, cfg_hot_batt_p, "batt-hot-percentage", rc, 1);
	OF_PROP_READ(chip, cfg_cold_batt_p, "batt-cold-percentage", rc, 1);
	OF_PROP_READ(chip, cfg_batt_weak_voltage_uv, "vbatweak-uv", rc, 1);
	OF_PROP_READ(chip, cfg_float_charge, "float-charge", rc, 1);
	/* <DTS2014042804721 chenyuanquan 20140428 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	OF_PROP_READ(chip, cfg_cold_bat_decidegc, "cold-bat-decidegc", rc, 1);
	OF_PROP_READ(chip, cfg_hot_bat_decidegc, "hot-bat-decidegc", rc, 1);
#endif
	/* DTS2014042804721 chenyuanquan 20140428 end> */

	if (rc) {
		pr_err("Error reading optional property rc=%d\n", rc);
		return rc;
	}

	rc = of_property_read_string(chip->spmi->dev.of_node,
						"qcom,bpd-detection", &bpd);
	if (rc) {

		chip->cfg_bpd_detection = BPD_TYPE_BAT_THM;
		rc = 0;
	} else {
		chip->cfg_bpd_detection = get_bpd(bpd);
		if (chip->cfg_bpd_detection < 0) {
			pr_err("Failed to determine bpd schema rc=%d\n", rc);
			return -EINVAL;
		}
	}

	/*
	 * Look up JEITA compliance parameters if cool and warm temp
	 * provided
	 */
	if (chip->cfg_cool_bat_decidegc || chip->cfg_warm_bat_decidegc) {
		chip->adc_tm_dev = qpnp_get_adc_tm(chip->dev, "chg");
		if (IS_ERR(chip->adc_tm_dev)) {
			rc = PTR_ERR(chip->adc_tm_dev);
			if (rc != -EPROBE_DEFER)
				pr_err("Failed to get adc-tm rc=%d\n", rc);
			return rc;
		}

		OF_PROP_READ(chip, cfg_warm_bat_chg_ma, "ibatmax-warm-ma",
				rc, 1);
		OF_PROP_READ(chip, cfg_cool_bat_chg_ma, "ibatmax-cool-ma",
				rc, 1);
		OF_PROP_READ(chip, cfg_warm_bat_mv, "warm-bat-mv", rc, 1);
		OF_PROP_READ(chip, cfg_cool_bat_mv, "cool-bat-mv", rc, 1);
		if (rc) {
			pr_err("Error reading battery temp prop rc=%d\n", rc);
			return rc;
		}
	}

	/* <DTS2014052906550 liyuping 20140530 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	chip->use_other_charger = of_property_read_bool(
			chip->spmi->dev.of_node,"qcom,use-other-charger");
	/*DTS2014091301574,begin:rm linear charger, 2014.9.30*/
    chip->use_other_charger_pad = of_property_read_bool(
			chip->spmi->dev.of_node,"qcom,use-other-charger-bq2419x");
	/*DTS2014091301574,end:rm linear charger, 2014.9.30*/
#endif
	/* <DTS2014062100152 jiangfei 20140623 begin */
#ifdef CONFIG_HUAWEI_DSM
	use_other_charger = chip->use_other_charger;
	/*DTS2014091301574,begin:rm linear charger, 2014.9.30*/
	if(chip->use_other_charger_pad)
	{
	    use_other_charger = chip->use_other_charger_pad;
		pr_err("use for pad 24296 charger\n");
	}
	/*DTS2014091301574,end:rm linear charger, 2014.9.30*/
#endif
	/* DTS2014062100152 jiangfei 20140623 end> */
	/* DTS2014052906550 liyuping 20140530 end> */
	/* Get the btc-disabled property */
	chip->cfg_btc_disabled = of_property_read_bool(
			chip->spmi->dev.of_node, "qcom,btc-disabled");

	/* Get the charging-disabled property */
	chip->cfg_charging_disabled =
		of_property_read_bool(chip->spmi->dev.of_node,
					"qcom,charging-disabled");

	/* Get the fake-batt-values property */
	chip->cfg_use_fake_battery =
			of_property_read_bool(chip->spmi->dev.of_node,
					"qcom,use-default-batt-values");

	/* Get peripheral reset configuration property */
	chip->cfg_disable_follow_on_reset =
			of_property_read_bool(chip->spmi->dev.of_node,
					"qcom,disable-follow-on-reset");

	/* Get the float charging property */
	chip->cfg_float_charge =
			of_property_read_bool(chip->spmi->dev.of_node,
					"qcom,float-charge");

	/* Get the charger EOC detect property */
	chip->cfg_charger_detect_eoc =
			of_property_read_bool(chip->spmi->dev.of_node,
					"qcom,charger-detect-eoc");

	/* Get the vbatdet disable property */
	chip->cfg_disable_vbatdet_based_recharge =
			of_property_read_bool(chip->spmi->dev.of_node,
					"qcom,disable-vbatdet-based-recharge");
	/* Disable charging when faking battery values */
	if (chip->cfg_use_fake_battery)
		chip->cfg_charging_disabled = true;

	chip->cfg_use_external_charger = of_property_read_bool(
			chip->spmi->dev.of_node, "qcom,use-external-charger");

	if (of_find_property(chip->spmi->dev.of_node,
					"qcom,thermal-mitigation",
					&chip->cfg_thermal_levels)) {
		chip->thermal_mitigation = devm_kzalloc(chip->dev,
			chip->cfg_thermal_levels,
			GFP_KERNEL);

		if (chip->thermal_mitigation == NULL) {
			pr_err("thermal mitigation kzalloc() failed.\n");
			return -ENOMEM;
		}

		chip->cfg_thermal_levels /= sizeof(int);
		rc = of_property_read_u32_array(chip->spmi->dev.of_node,
				"qcom,thermal-mitigation",
				chip->thermal_mitigation,
				chip->cfg_thermal_levels);
		if (rc) {
			pr_err("Failed to read threm limits rc = %d\n", rc);
			return rc;
		}
	}

	return rc;
}
/* < DTS2014092502339 zhaoxiaoli 20140929 begin */
#ifdef CONFIG_HUAWEI_KERNEL
static void mt_battery_average_method_init(unsigned int *bufferdata, unsigned int data, int *sum)
{
	int i =0;
	for (i=0; i<VINMIN_ADJUST_MAX_COUNT; i++)
	{
		bufferdata[i] = data;
	}
	*sum = data * VINMIN_ADJUST_MAX_COUNT;
}
static int mt_battery_average_method(unsigned int *bufferdata, unsigned int voltage)
{
	int avgdata;
    static int battery_index = 0;
    static int sum=0;
	static int batteryBufferFirst = 1;
	if(batteryBufferFirst == 1)
	{
		mt_battery_average_method_init(bufferdata, voltage, &sum);
		batteryBufferFirst = 0;
	}

	sum -= bufferdata[battery_index];
	sum +=  voltage;
	bufferdata[battery_index] = voltage;
	avgdata = (sum)/VINMIN_ADJUST_MAX_COUNT;
	battery_index++;
	if(battery_index >= VINMIN_ADJUST_MAX_COUNT)
	{
		battery_index=0;
	}
	return avgdata;
}
static void hw_adjuct_vinmin_set(int vbat_uv,struct qpnp_lbc_chip *chip)
{
	int voltage = 0;
	int avg_voltage = 0;
	static int batteryVoltageBuffer[VINMIN_ADJUST_MAX_COUNT];
	static bool adjust_flag = true;
	static int low_count = 0;
	static int high_count = 0;
	avg_voltage = mt_battery_average_method(&batteryVoltageBuffer[0],vbat_uv);
	pr_debug("avg_voltage=%d vbat_uv=%d\n",avg_voltage,vbat_uv);
	if (avg_voltage < BATTERY_VOL_ADJ_THR_UV)
	{
		if(adjust_flag)
		{
			low_count++;
			if(low_count >= VINMIN_ADJUST_MAX_COUNT)
			{
				voltage = HW_VINMIN_ADJ_THR_MV;
				qpnp_lbc_vinmin_set(chip,HW_VINMIN_ADJ_THR_MV);
				pr_info("set low vinmin voltage threthold =%d\n",voltage);
				adjust_flag = false;
				low_count = 0;
			}
		}
	}
	else
	{
		if(!adjust_flag)
		{
			high_count++;
			if(high_count >= VINMIN_ADJUST_MAX_COUNT)
			{
				voltage = HW_VINMIN_DEFAULT_MV;
				qpnp_lbc_vinmin_set(chip,HW_VINMIN_DEFAULT_MV);
				pr_info("set default vinmin voltage threthold =%d\n",voltage);
				adjust_flag = true;
				high_count = 0;
			}
		}
	}
}
#endif
/* DTS2014092502339 zhaoxiaoli 20140929 end> */

static irqreturn_t qpnp_lbc_usbin_valid_irq_handler(int irq, void *_chip)
{
	struct qpnp_lbc_chip *chip = _chip;
	int usb_present;
	unsigned long flags;

	usb_present = qpnp_lbc_is_usb_chg_plugged_in(chip);
	/* < DTS2014051304851 liyuping 20140513 begin */
	pr_info("usbin-valid triggered: %d\n", usb_present);
	/* DTS2014051304851 liyuping 20140513 end > */
	/*<DTS2014053001058 jiangfei 20140530 begin */

	if (chip->usb_present ^ usb_present) {
		chip->usb_present = usb_present;
		if (!usb_present) {
        /* <DTS2014070819388 zhaoxiaoli 20140711 begin */
#ifdef CONFIG_HUAWEI_KERNEL
        /* wake_lock for update the state of led,prevent phone enter sleep at once  */
        wake_lock_timeout(&chip->led_wake_lock,msecs_to_jiffies(LED_CHECK_PERIOD_MS));
#endif
        /* DTS2014070819388 zhaoxiaoli 20140711 end> */
#ifdef CONFIG_HUAWEI_DSM
            /*DTS2014091301574,begin:rm linear charger, 2014.9.30*/
            if(!chip->use_other_charger_pad)
			{
			    cancel_delayed_work(&chip->check_charging_status_work);
			}
			/*DTS2014091301574,end:rm linear charger, 2014.9.30*/
#endif
			/* < DTS2014092502339 zhaoxiaoli 20140929 begin */
#ifdef CONFIG_HUAWEI_KERNEL
				qpnp_lbc_vinmin_set(chip,HW_VINMIN_DEFAULT_MV);
#endif
			/* DTS2014092502339 zhaoxiaoli 20140929 end> */
			qpnp_lbc_charger_enable(chip, CURRENT, 0);
			spin_lock_irqsave(&chip->ibat_change_lock, flags);
			chip->usb_psy_ma = QPNP_CHG_I_MAX_MIN_90;
			qpnp_lbc_set_appropriate_current(chip);
			/*<DTS2014061605512 liyuping 20140617 begin */
			/* <DTS2014081410316 jiangfei 20140819 begin */
			wake_unlock(&chip->chg_wake_lock);
			/* DTS2014081410316 jiangfei 20140819 end> */
			/* DTS2014061605512 liyuping 20140617 end> */
			spin_unlock_irqrestore(&chip->ibat_change_lock,
								flags);
		} else {
			/*
			 * Override VBAT_DET comparator to start charging
			 * even if VBAT > VBAT_DET.
			 */
			if (!chip->cfg_disable_vbatdet_based_recharge)
				qpnp_lbc_vbatdet_override(chip, OVERRIDE_0);

			/*
			 * Enable SOC based charging to make sure
			 * charging gets enabled on USB insertion
			 * irrespective of battery SOC above resume_soc.
			 */
			qpnp_lbc_charger_enable(chip, SOC, 1);
			/*<DTS2014061605512 liyuping 20140617 begin */
			/* <DTS2014081410316 jiangfei 20140819 begin */
			wake_lock(&chip->chg_wake_lock);
			/* DTS2014081410316 jiangfei 20140819 end> */
			/* DTS2014061605512 liyuping 20140617 end> */
			/* <DTS2014070701815 jiangfei 20140707 begin */
#ifdef CONFIG_HUAWEI_DSM
            /*DTS2014091301574,begin:rm linear charger, 2014.9.30*/
            if(!chip->use_other_charger_pad)
			{
                schedule_delayed_work(&chip->check_charging_status_work,
				    msecs_to_jiffies(DELAY_TIME));
			}
			/*DTS2014091301574,end:rm linear charger, 2014.9.30*/
#endif
		}
			/* DTS2014070701815 jiangfei 20140707 end> */

		pr_debug("Updating usb_psy PRESENT property\n");
		power_supply_set_present(chip->usb_psy, chip->usb_present);
        /* <DTS2014071002612  mapengfei 20140710 begin */
#ifdef CONFIG_HUAWEI_KERNEL
		if (chip->bat_if_base) {
			pr_debug("power supply changed batt_psy\n");
			power_supply_changed(&chip->batt_psy);
		}
#endif
        /* DTS2014071002612  mapengfei 20140710 end> */
	}
#ifdef CONFIG_HUAWEI_DSM
	/* if usb online status is abnormal*/
	/*dump pmic registers and adc values, and notify to dsm server */
	else{
		chip->error_type = DSM_CHARGER_ONLINE_ABNORMAL_ERROR_NO;
		schedule_work(&chip->dump_work);
	}
#endif
	/* DTS2014053001058 jiangfei 20140530 end> */
	return IRQ_HANDLED;
}

/* < DTS2014092405372 gwx199358 20140924 begin */
#ifdef CONFIG_HUAWEI_KERNEL
static int cblpwr_usbin_request_irq(void)
{
        int irq = 0, rc = -1;

        irq = huawei_get_cblpwr_irq_no();
        if( irq <= 0) {
                pr_err("Get cblpwr_usbin irq nor failed, irq = %d\n", irq);
                return rc;
        }

        rc = devm_request_irq(g_lbc_chip->dev, irq, qpnp_lbc_usbin_valid_irq_handler,
                IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                "cblpwr_usbin", g_lbc_chip);
        if (rc < 0) {
                 pr_err("Unable to request cblpwr_usbin, %d\n", rc);
        }
        else {
                 enable_irq_wake(irq);
        }

        return rc;
}
#endif
/* DTS2014092405372 gwx199358 20140924 end > */

static int qpnp_lbc_is_batt_temp_ok(struct qpnp_lbc_chip *chip)
{
	u8 reg_val;
	int rc;

	rc = qpnp_lbc_read(chip, chip->bat_if_base + INT_RT_STS_REG,
				&reg_val, 1);
	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n",
				chip->bat_if_base + INT_RT_STS_REG, rc);
		return rc;
	}

	return (reg_val & BAT_TEMP_OK_IRQ) ? 1 : 0;
}

static irqreturn_t qpnp_lbc_batt_temp_irq_handler(int irq, void *_chip)
{
	struct qpnp_lbc_chip *chip = _chip;
	int batt_temp_good;

	batt_temp_good = qpnp_lbc_is_batt_temp_ok(chip);
	pr_debug("batt-temp triggered: %d\n", batt_temp_good);

	pr_debug("power supply changed batt_psy\n");
	power_supply_changed(&chip->batt_psy);
	return IRQ_HANDLED;
}

static irqreturn_t qpnp_lbc_batt_pres_irq_handler(int irq, void *_chip)
{
	struct qpnp_lbc_chip *chip = _chip;
	int batt_present;

	batt_present = qpnp_lbc_is_batt_present(chip);
	pr_debug("batt-pres triggered: %d\n", batt_present);

	if (chip->batt_present ^ batt_present) {
		chip->batt_present = batt_present;
		pr_debug("power supply changed batt_psy\n");
		power_supply_changed(&chip->batt_psy);

		if ((chip->cfg_cool_bat_decidegc
					|| chip->cfg_warm_bat_decidegc)
					&& batt_present) {
			pr_debug("enabling vadc notifications\n");
			if (qpnp_adc_tm_channel_measure(chip->adc_tm_dev,
						&chip->adc_param))
				pr_err("request ADC error\n");
		} else if ((chip->cfg_cool_bat_decidegc
					|| chip->cfg_warm_bat_decidegc)
					&& !batt_present) {
			qpnp_adc_tm_disable_chan_meas(chip->adc_tm_dev,
					&chip->adc_param);
			/*<DTS2014053001058 jiangfei 20140530 begin */
			/* if battery present irq is triggered, and is absent*/
			/*dump pmic registers and adc values, and notify to dsm server */
#ifdef CONFIG_HUAWEI_DSM
			chip->error_type = DSM_CHARGER_BATT_PRES_ERROR_NO;
			schedule_work(&chip->dump_work);
#endif
			/* DTS2014053001058 jiangfei 20140530 end> */
			pr_debug("disabling vadc notifications\n");
		}
	}
	return IRQ_HANDLED;
}

static irqreturn_t qpnp_lbc_chg_failed_irq_handler(int irq, void *_chip)
{
	struct qpnp_lbc_chip *chip = _chip;
	int rc;
	u8 reg_val = CHG_FAILED_BIT;

	/* < DTS2014051304851 liyuping 20140513 begin */
	pr_info("chg_failed triggered count=%u\n", ++chip->chg_failed_count);
	/* DTS2014051304851 liyuping 20140513 end > */
	rc = qpnp_lbc_write(chip, chip->chgr_base + CHG_FAILED_REG,
				&reg_val, 1);
	if (rc)
		pr_err("Failed to write chg_fail clear bit rc=%d\n", rc);

	if (chip->bat_if_base) {
		pr_debug("power supply changed batt_psy\n");
		power_supply_changed(&chip->batt_psy);
	}

	return IRQ_HANDLED;
}

static int qpnp_lbc_is_fastchg_on(struct qpnp_lbc_chip *chip)
{
	u8 reg_val;
	int rc;

	rc = qpnp_lbc_read(chip, chip->chgr_base + INT_RT_STS_REG,
				&reg_val, 1);
	if (rc) {
		pr_err("Failed to read interrupt status rc=%d\n", rc);
		return rc;
	}
	pr_debug("charger status %x\n", reg_val);
	return (reg_val & FAST_CHG_ON_IRQ) ? 1 : 0;
}

#define TRIM_PERIOD_NS			(50LL * NSEC_PER_SEC)
static irqreturn_t qpnp_lbc_fastchg_irq_handler(int irq, void *_chip)
{
	ktime_t kt;
	struct qpnp_lbc_chip *chip = _chip;
	bool fastchg_on = false;

	fastchg_on = qpnp_lbc_is_fastchg_on(chip);

	/* < DTS2014051304851 liyuping 20140513 begin */
	pr_info("FAST_CHG IRQ triggered, fastchg_on: %d\n", fastchg_on);
	/* DTS2014051304851 liyuping 20140513 end > */
	/* <DTS2014061002202 jiangfei 20140610 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	if (chip->resuming_charging) {
		chip->resuming_charging = false;
	}
#endif
	/* DTS2014061002202 jiangfei 20140610 end> */
	if (chip->fastchg_on ^ fastchg_on) {
		chip->fastchg_on = fastchg_on;
		if (fastchg_on) {
			mutex_lock(&chip->chg_enable_lock);
			chip->chg_done = false;
			mutex_unlock(&chip->chg_enable_lock);
			/*
			 * Start alarm timer to periodically calculate
			 * and update VDD_MAX trim value.
			 */
			if (chip->supported_feature_flag &
						VDD_TRIM_SUPPORTED) {
				kt = ns_to_ktime(TRIM_PERIOD_NS);
				alarm_start_relative(&chip->vddtrim_alarm,
							kt);
			}
		}

		if (chip->bat_if_base) {
			pr_debug("power supply changed batt_psy\n");
			power_supply_changed(&chip->batt_psy);
		}
	}

	return IRQ_HANDLED;
}

static irqreturn_t qpnp_lbc_chg_done_irq_handler(int irq, void *_chip)
{
	struct qpnp_lbc_chip *chip = _chip;

	pr_debug("charging done triggered\n");

	chip->chg_done = true;
	pr_debug("power supply changed batt_psy\n");
	power_supply_changed(&chip->batt_psy);

	return IRQ_HANDLED;
}

static irqreturn_t qpnp_lbc_vbatdet_lo_irq_handler(int irq, void *_chip)
{
	struct qpnp_lbc_chip *chip = _chip;
	int rc;

	/* < DTS2014051304851 liyuping 20140513 begin */
	pr_info("vbatdet-lo triggered\n");
	/* DTS2014051304851 liyuping 20140513 end > */

    /* < DTS2014060604706 zhaoxiaoli 20140610 begin */
#ifdef CONFIG_HUAWEI_KERNEL
    if(chip->cfg_charging_disabled == true)
    {
        pr_info("cfg_charging_disabled = %d\n",chip->cfg_charging_disabled);
        return IRQ_HANDLED;
    }
#endif
    /* DTS2014060604706 zhaoxiaoli 20140610 end > */
	/*
	 * Disable vbatdet irq to prevent interrupt storm when VBAT is
	 * close to VBAT_DET.
	 */
	qpnp_lbc_disable_irq(chip, &chip->irqs[CHG_VBAT_DET_LO]);

	/*
	 * Override VBAT_DET comparator to 0 to fix comparator toggling
	 * near VBAT_DET threshold.
	 */
	qpnp_lbc_vbatdet_override(chip, OVERRIDE_0);

	/*
	 * Battery has fallen below the vbatdet threshold and it is
	 * time to resume charging.
	 */
	rc = qpnp_lbc_charger_enable(chip, SOC, 1);
	if (rc)
		pr_err("Failed to enable charging\n");

	return IRQ_HANDLED;
}

static int qpnp_lbc_is_overtemp(struct qpnp_lbc_chip *chip)
{
	u8 reg_val;
	int rc;

	rc = qpnp_lbc_read(chip, chip->usb_chgpth_base + INT_RT_STS_REG,
				&reg_val, 1);
	if (rc) {
		pr_err("Failed to read interrupt status rc=%d\n", rc);
		return rc;
	}

	pr_debug("OVERTEMP rt status %x\n", reg_val);
	return (reg_val & OVERTEMP_ON_IRQ) ? 1 : 0;
}

static irqreturn_t qpnp_lbc_usb_overtemp_irq_handler(int irq, void *_chip)
{
	struct qpnp_lbc_chip *chip = _chip;
	int overtemp = qpnp_lbc_is_overtemp(chip);

	pr_warn_ratelimited("charger %s temperature limit !!!\n",
					overtemp ? "exceeds" : "within");

	return IRQ_HANDLED;
}

static int qpnp_disable_lbc_charger(struct qpnp_lbc_chip *chip)
{
	int rc;
	u8 reg;

	reg = CHG_FORCE_BATT_ON;
	rc = qpnp_lbc_masked_write(chip, chip->chgr_base + CHG_CTRL_REG,
							CHG_EN_MASK, reg);
	/* disable BTC */
	rc |= qpnp_lbc_masked_write(chip, chip->bat_if_base + BAT_IF_BTC_CTRL,
							BTC_COMP_EN_MASK, 0);
	/* Enable BID and disable THM based BPD */
	reg = BATT_ID_EN | BATT_BPD_OFFMODE_EN;
	rc |= qpnp_lbc_write(chip, chip->bat_if_base + BAT_IF_BPD_CTRL_REG,
								&reg, 1);
	return rc;
}

#define SPMI_REQUEST_IRQ(chip, idx, rc, irq_name, threaded, flags, wake)\
do {									\
	if (rc)								\
		break;							\
	if (chip->irqs[idx].irq) {					\
		if (threaded)						\
			rc = devm_request_threaded_irq(chip->dev,	\
				chip->irqs[idx].irq, NULL,		\
				qpnp_lbc_##irq_name##_irq_handler,	\
				flags, #irq_name, chip);		\
		else							\
			rc = devm_request_irq(chip->dev,		\
				chip->irqs[idx].irq,			\
				qpnp_lbc_##irq_name##_irq_handler,	\
				flags, #irq_name, chip);		\
		if (rc < 0) {						\
			pr_err("Unable to request " #irq_name " %d\n",	\
								rc);	\
		} else {						\
			rc = 0;						\
			if (wake) {					\
				enable_irq_wake(chip->irqs[idx].irq);	\
				chip->irqs[idx].is_wake = true;		\
			}						\
		}							\
	}								\
} while (0)

#define SPMI_GET_IRQ_RESOURCE(chip, rc, resource, idx, name)		\
do {									\
	if (rc)								\
		break;							\
									\
	rc = spmi_get_irq_byname(chip->spmi, resource, #name);		\
	if (rc < 0) {							\
		pr_err("Unable to get irq resource " #name "%d\n", rc);	\
	} else {							\
		chip->irqs[idx].irq = rc;				\
		rc = 0;							\
	}								\
} while (0)

static int qpnp_lbc_request_irqs(struct qpnp_lbc_chip *chip)
{
	int rc = 0;
/* < DTS2014092405372 gwx199358 20140924 begin */
#ifdef CONFIG_HUAWEI_KERNEL
        uint32_t boardid = 0;
#endif
/* DTS2014092405372 gwx199358 20140924 end > */

	SPMI_REQUEST_IRQ(chip, CHG_FAILED, rc, chg_failed, 0,
			IRQF_TRIGGER_RISING, 1);

	SPMI_REQUEST_IRQ(chip, CHG_FAST_CHG, rc, fastchg, 1,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING
			| IRQF_ONESHOT, 1);

	SPMI_REQUEST_IRQ(chip, CHG_DONE, rc, chg_done, 0,
			IRQF_TRIGGER_RISING, 0);

	SPMI_REQUEST_IRQ(chip, CHG_VBAT_DET_LO, rc, vbatdet_lo, 0,
			IRQF_TRIGGER_FALLING, 1);

	SPMI_REQUEST_IRQ(chip, BATT_PRES, rc, batt_pres, 1,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING
			| IRQF_ONESHOT, 1);

	SPMI_REQUEST_IRQ(chip, BATT_TEMPOK, rc, batt_temp, 0,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, 1);

/* < DTS2014092405372 gwx199358 20140924 begin */
#ifndef CONFIG_HUAWEI_KERNEL
       SPMI_REQUEST_IRQ(chip, USBIN_VALID, rc, usbin_valid, 0,
                       IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, 1);
#else
       boardid = socinfo_get_platform_type();
       if (BOARDID_T1_10 != boardid) {
            cblpwr_usbin_request_irq();
       }
       else {
                SPMI_REQUEST_IRQ(chip, USBIN_VALID, rc, usbin_valid, 0,
                        IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, 1);
       }
#endif
/* < DTS2014092405372 gwx199358 20140924 begin */

	SPMI_REQUEST_IRQ(chip, USB_OVER_TEMP, rc, usb_overtemp, 0,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, 0);

	return 0;
}

static int qpnp_lbc_get_irqs(struct qpnp_lbc_chip *chip, u8 subtype,
					struct spmi_resource *spmi_resource)
{
	int rc = 0;

	switch (subtype) {
	case LBC_CHGR_SUBTYPE:
		SPMI_GET_IRQ_RESOURCE(chip, rc, spmi_resource,
						CHG_FAST_CHG, fast-chg-on);
		SPMI_GET_IRQ_RESOURCE(chip, rc, spmi_resource,
						CHG_FAILED, chg-failed);
		if (!chip->cfg_disable_vbatdet_based_recharge)
			SPMI_GET_IRQ_RESOURCE(chip, rc, spmi_resource,
						CHG_VBAT_DET_LO, vbat-det-lo);
		if (chip->cfg_charger_detect_eoc)
			SPMI_GET_IRQ_RESOURCE(chip, rc, spmi_resource,
						CHG_DONE, chg-done);
		break;
	case LBC_BAT_IF_SUBTYPE:
		SPMI_GET_IRQ_RESOURCE(chip, rc, spmi_resource,
						BATT_PRES, batt-pres);
		SPMI_GET_IRQ_RESOURCE(chip, rc, spmi_resource,
						BATT_TEMPOK, bat-temp-ok);
		break;
	case LBC_USB_PTH_SUBTYPE:
		SPMI_GET_IRQ_RESOURCE(chip, rc, spmi_resource,
						USBIN_VALID, usbin-valid);
		SPMI_GET_IRQ_RESOURCE(chip, rc, spmi_resource,
						USB_OVER_TEMP, usb-over-temp);
		break;
	};

	return 0;
}

/* Get/Set initial state of charger */
static void determine_initial_status(struct qpnp_lbc_chip *chip)
{
	u8 reg_val;
	int rc;

	chip->usb_present = qpnp_lbc_is_usb_chg_plugged_in(chip);
	power_supply_set_present(chip->usb_psy, chip->usb_present);
	/*
	 * Set USB psy online to avoid userspace from shutting down if battery
	 * capacity is at zero and no chargers online.
	 */
	if (chip->usb_present)
		power_supply_set_online(chip->usb_psy, 1);

	/*
	 * Configure peripheral reset control
	 * This is a workaround only for SLT testing.
	 */
	if (chip->cfg_disable_follow_on_reset) {
		reg_val = 0x0;
		rc = __qpnp_lbc_secure_write(chip->spmi, chip->chgr_base,
				CHG_PERPH_RESET_CTRL3_REG, &reg_val, 1);
		if (rc)
			pr_err("Failed to configure PERPH_CTRL3 rc=%d\n", rc);
		else
			pr_warn("Charger is not following PMIC reset\n");
	}
}

/* <DTS2014042804721 chenyuanquan 20140428 begin */
#ifdef CONFIG_HUAWEI_KERNEL
static int qpnp_chg_load_battery_data(struct qpnp_lbc_chip *chip)
{
	struct bms_battery_data batt_data;
	struct device_node *node;
	struct qpnp_vadc_result result;
	int rc;

	node = of_find_node_by_name(chip->spmi->dev.of_node,
			"qcom,battery-data");
	if (node)
	{
		memset(&batt_data, 0, sizeof(struct bms_battery_data));
		rc = qpnp_vadc_read(chip->vadc_dev, LR_MUX2_BAT_ID, &result);
		if (rc)
		{
			pr_err("error reading batt id channel = %d, rc = %d\n",
						LR_MUX2_BAT_ID, rc);
			return rc;
		}

		batt_data.max_voltage_uv = -1;
		batt_data.iterm_ua = -1;
		rc = of_batterydata_read_data(node,
			&batt_data, result.physical);
		if (rc)
		{
			pr_err("failed to read battery data: %d\n", rc);
			batt_data = palladium_1500_data;
		}

		if ((chip->cfg_cool_bat_decidegc || chip->cfg_warm_bat_decidegc) &&
			(batt_data.warm_bat_decidegc || batt_data.cool_bat_decidegc))
		{
			chip->cfg_warm_bat_decidegc = batt_data.warm_bat_decidegc;
			chip->cfg_warm_bat_chg_ma = batt_data.warm_bat_chg_ma;
			chip->cfg_warm_bat_mv = batt_data.warm_bat_mv;

			chip->cfg_cool_bat_decidegc = batt_data.cool_bat_decidegc;
			chip->cfg_cool_bat_chg_ma = batt_data.cool_bat_chg_ma;
			chip->cfg_cool_bat_mv = batt_data.cool_bat_mv;

			chip->cfg_hot_bat_decidegc = batt_data.hot_bat_decidegc;
			chip->cfg_cold_bat_decidegc = batt_data.cold_bat_decidegc;

			pr_info("use special temp-cv parameters\n");
		}
		/*<DTS2014070404260 liyuping 20140704 begin */
		/*<DTS2014080704371 liyuping 20140815 begin */
#ifdef CONFIG_HUAWEI_KERNEL		
		if(chip->use_other_charger)
		{
			jeita_batt_param.cold.i_max = 0;
			jeita_batt_param.cold.v_max = 0;
			jeita_batt_param.cold.t_high = chip->cfg_cold_bat_decidegc;
			jeita_batt_param.cold.t_low = INT_MIN;
			jeita_batt_param.cold.selected = 0;
			jeita_batt_param.cold.last_zone = NULL;

			jeita_batt_param.cool.i_max = chip->cfg_cool_bat_chg_ma;
			jeita_batt_param.cool.v_max = chip->cfg_cool_bat_mv;
			jeita_batt_param.cool.t_high = chip->cfg_cool_bat_decidegc;
			jeita_batt_param.cool.t_low = chip->cfg_cold_bat_decidegc;
			jeita_batt_param.cool.selected = 0;
			jeita_batt_param.cool.last_zone = NULL;

			jeita_batt_param.normal.i_max = 1500;
			jeita_batt_param.normal.v_max = 4350;
			jeita_batt_param.normal.t_high = chip->cfg_warm_bat_decidegc;
			jeita_batt_param.normal.t_low = chip->cfg_cool_bat_decidegc;
			jeita_batt_param.normal.selected = 0;
			jeita_batt_param.normal.last_zone = NULL;

			jeita_batt_param.warm.i_max = chip->cfg_warm_bat_chg_ma;
			jeita_batt_param.warm.v_max = chip->cfg_warm_bat_mv;
			jeita_batt_param.warm.t_high = chip->cfg_hot_bat_decidegc;
			jeita_batt_param.warm.t_low = chip->cfg_warm_bat_decidegc;
			jeita_batt_param.warm.selected = 0;
			jeita_batt_param.warm.last_zone = NULL;

			jeita_batt_param.hot.i_max = 0;
			jeita_batt_param.hot.v_max = 0;
			jeita_batt_param.hot.t_high = INT_MAX;
			jeita_batt_param.hot.t_low = chip->cfg_hot_bat_decidegc;
			jeita_batt_param.hot.selected = 0;
			jeita_batt_param.hot.last_zone = NULL;
		}
#endif
		/* DTS2014080704371 liyuping 20140815 end> */
		/* DTS2014061605512 liyuping 20140617 end> */

		pr_info("warm_bat_decidegc=%d "
				"warm_bat_chg_ma=%d "
				"warm_bat_mv=%d "
				"cool_bat_decidegc=%d "
				"cool_bat_chg_ma=%d "
				"cool_bat_mv=%d "
				"hot_bat_decidegc=%d "
				"cold_bat_decidegc=%d \n",
				chip->cfg_warm_bat_decidegc,
				chip->cfg_warm_bat_chg_ma,
				chip->cfg_warm_bat_mv,
				chip->cfg_cool_bat_decidegc,
				chip->cfg_cool_bat_chg_ma,
				chip->cfg_cool_bat_mv,
				chip->cfg_hot_bat_decidegc,
				chip->cfg_cold_bat_decidegc);

		if (batt_data.max_voltage_uv >= 0)
		{
			chip->cfg_max_voltage_mv = batt_data.max_voltage_uv / 1000;
		}
		if (batt_data.iterm_ua >= 0)
		{
			chip->cfg_term_current = batt_data.iterm_ua / 1000;
		}

	}

	return 0;
}
#endif
/* DTS2014042804721 chenyuanquan 20140428 end> */

/*<DTS2014053001058 jiangfei 20140530 begin */
/* <DTS2014061002202 jiangfei 20140610 begin */
/* <DTS2014062100152 jiangfei 20140623 begin */
/*<DTS2014080807172 jiangfei 20140813 begin */
/* <DTS2014081205934 jiangfei 20140820 begin */
#ifdef CONFIG_HUAWEI_DSM
int dump_registers_and_adc(struct dsm_client *dclient, struct qpnp_lbc_chip *chip, int type )
{
	int i = 0;
	int rc = 0;
	u8 reg_val;
	int vbat_uv, current_ma, batt_temp,batt_id, vusb_uv, vchg_uv, vcoin_uv;
	union power_supply_propval val = {0};

	if( NULL == dclient )
	{
		pr_err("%s: there is no dclient!\n", __func__);
		return -1;
	}

	if( NULL == chip )
	{
		pr_err("%s: chip is NULL!\n", __func__);
		return -1;
	}

	/* try to get permission to use the buffer */
	if(dsm_client_ocuppy(dclient))
	{
		/* buffer is busy */
		pr_err("%s: buffer is busy!\n", __func__);
		return -1;
	}

	switch(type)
	{
		case DSM_BMS_POWON_SOC_ERROR_NO:
			/* report BMS_POWON_SOC basic error infomation */
			dsm_client_record(dclient,
				"poweron soc is different with last"
				"shutdown soc, the difference is more than 10 percent\n");
			break;

		case DSM_BMS_NORMAL_SOC_ERROR_NO:
			/* report BMS_NORMAL_SOC basic error infomation */
			dsm_client_record(dclient,
				"soc changed a lot in one minute"
				"the difference is more than 5 percent\n");
			break;

		case DSM_CHARGER_NOT_CHARGE_ERROR_NO:
			/* report CHARGER_NOT_CHARGE basic error infomation */
			dsm_client_record(dclient,
				"cannot charging when allowed charging\n");
			break;

		case DSM_CHARGER_OTG_OCP_ERROR_NO:
			/* report CHARGER_OTG_OCP basic error infomation */
			dsm_client_record(dclient,
				"OTG OCP happened!\n");
			break;

		case DSM_CHARGER_CHG_OVP_ERROR_NO:
			/* report CHARGER_CHG_OVP basic error infomation */
			dsm_client_record(dclient,
				"CHG_OVP happend!\n");
			break;

		case DSM_CHARGER_BATT_PRES_ERROR_NO:
			/* report CHARGER_BATT_PRES basic error infomation */
			dsm_client_record(dclient,
				"battery is absent!\n");
			pr_info("battery is absent!!!");
			break;

		//case DSM_CHARGER_ONLINE_ABNORMAL_ERROR_NO:
			/* report CHARGER_ONLINE_ABNORMAL basic erro infomation */
		//	dsm_client_record(dclient,
		//		"charger online status abnormal!\n");
		//	break;

		case DSM_CHARGER_ADC_ABNORMAL_ERROR_NO:
			/* report CHARGER_ADC_ABNORMAL basic erro infomation */
			dsm_client_record(dclient,
				"ADC abnormal!\n");
			break;

		//case DSM_CHARGER_BQ_BOOST_FAULT_ERROR_NO:
			/* report CHARGER_BQ_BOOST_FAULT basic erro infomation */
		//	dsm_client_record(dclient,
		//		"find BQ24152 fault in boost mode!\n");
		//	break;

		case DSM_CHARGER_BQ_NORMAL_FAULT_ERROR_NO:
			/* report CHARGER_BQ_NORMAL_FAULT basic erro infomation */
			dsm_client_record(dclient,
				"find BQ24152 fault in charger mode!\n");
			break;

		default:
			break;
	}

    /*DTS2014091301574,begin:rm linear charger, 2014.9.30*/
	if(chip->use_other_charger_pad)
	{
	    /*Last save battery vbat current and temp values*/
        vbat_uv = get_prop_battery_voltage_now(chip);
        current_ma = get_prop_current_now(chip);
        batt_temp = get_prop_batt_temp(chip);
	}
	else if(!chip->use_other_charger){
	/*DTS2014091301574,end:rm linear charger, 2014.9.30*/
		/*First dump LBC_CHGR registers*/
		dsm_client_record(dclient, "[LBC_CHGR] regs:\n");
		pr_info("[LBC_CHGR] regs:\n");
		for(i = 0; i < ARRAY_SIZE(LBC_CHGR); i++){
			rc = qpnp_lbc_read(chip, chip->chgr_base + LBC_CHGR[i],
					&reg_val, 1);
			if (rc) {
				pr_err("Failed to read chgr_base registers %d\n", rc);
				return -1;
			}
			dsm_client_record(dclient,
					"0x%x, 0x%x\n",
					chip->chgr_base+LBC_CHGR[i], reg_val);
			pr_info("0x%x, 0x%x\n",
					chip->chgr_base+LBC_CHGR[i], reg_val);
		}

		/*Then dump LBC_BAT_IF registers*/
		dsm_client_record(dclient, "[LBC_BAT_IF] regs:\n");
		pr_info("[LBC_BAT_IF] regs:\n");
		for(i = 0; i < ARRAY_SIZE(LBC_BAT_IF); i++){
			rc = qpnp_lbc_read(chip, chip->bat_if_base + LBC_BAT_IF[i],
					&reg_val, 1);
			if (rc) {
				pr_err("Failed to read bat_if_base registers %d\n", rc);
				return -1;
			}
			dsm_client_record(dclient,
					"0x%x, 0x%x\n",
					chip->bat_if_base+LBC_BAT_IF[i], reg_val);
			pr_info("0x%x, 0x%x\n",
					chip->bat_if_base+LBC_BAT_IF[i], reg_val);
		}

		/*Third dump LBC_USB registers*/
		dsm_client_record(dclient, "[LBC_USB] regs:\n");
		pr_info("[LBC_USB] regs:\n");
		for(i = 0; i < ARRAY_SIZE(LBC_USB); i++){
			rc = qpnp_lbc_read(chip, chip->usb_chgpth_base + LBC_USB[i],
					&reg_val, 1);
			if (rc) {
				pr_err("Failed to read usb_chgpth_base registers %d\n", rc);
				return -1;
			}
			dsm_client_record(dclient,
					"0x%x, 0x%x\n",
					chip->usb_chgpth_base+LBC_USB[i], reg_val);
			pr_info("0x%x, 0x%x\n",
					chip->usb_chgpth_base+LBC_USB[i], reg_val);
		}

		/*Fourth dump LBC_MISC registers*/
		dsm_client_record(dclient, "[LBC_MISC] regs:\n");
		pr_info("[LBC_MISC] regs:\n");
		for(i = 0; i < ARRAY_SIZE(LBC_MISC); i++){
			rc = qpnp_lbc_read(chip, chip->misc_base + LBC_MISC[i],
					&reg_val, 1);
			if (rc) {
				pr_err("Failed to read misc_base registers %d\n", rc);
				return -1;
			}
			dsm_client_record(dclient,
					"0x%x, 0x%x\n",
					chip->misc_base+LBC_MISC[i], reg_val);
			pr_info("0x%x, 0x%x\n",
					chip->misc_base+LBC_MISC[i], reg_val);
		}

		/*If error no is bms soc type, then dump VM_BMS registers*/
		if ((bms_base != 0) && ((DSM_BMS_POWON_SOC_ERROR_NO == type)
			||(DSM_BMS_NORMAL_SOC_ERROR_NO == type))){
			dsm_client_record(dclient, "[VM_BMS] regs:\n");
			pr_info("[VM_BMS] regs:\n");
			for(i = 0; i < ARRAY_SIZE(VM_BMS); i++){
				rc = qpnp_lbc_read(chip, bms_base + VM_BMS[i],
					&reg_val, 1);
				if (rc) {
					pr_err("Failed to read bms_base registers %d\n", rc);
					return -1;
				}
				dsm_client_record(dclient,
						"0x%x, 0x%x\n",
						bms_base+VM_BMS[i], reg_val);
				pr_info("0x%x, 0x%x\n",
						bms_base+VM_BMS[i], reg_val);
			}
		}
		/*Last save battery vbat current and temp values*/
		vbat_uv = get_prop_battery_voltage_now(chip);
		current_ma = get_prop_current_now(chip);
		batt_temp = get_prop_batt_temp(chip);
	}else{
		/*First dump BQ24152 registers*/
		bq2415x_dump_regs(dclient);
		/*Then dump BQ27510 registers*/
		bq27510_dump_regs(dclient);
		/*Last save battery vbat current and temp values*/
		if(chip->ti_charger && chip->ti_charger->get_property){
			chip->ti_charger->get_property(chip->ti_charger,POWER_SUPPLY_PROP_VOLTAGE_NOW,&val);
			vbat_uv = val.intval;
			chip->ti_charger->get_property(chip->ti_charger,POWER_SUPPLY_PROP_CURRENT_NOW,&val);
			current_ma = val.intval;
			chip->ti_charger->get_property(chip->ti_charger,POWER_SUPPLY_PROP_TEMP,&val);
			batt_temp = val.intval;
		}
	}

	batt_id = get_prop_battery_id(chip);
	vusb_uv = get_prop_vbus_uv(chip);
	vchg_uv = get_prop_vchg_uv(chip);
	vcoin_uv = get_prop_vcoin_uv(chip);

	dsm_client_record(dclient,
				"ADC values: vbat=%d current=%d batt_temp=%d "
				"batt_id=%d vusb=%d vchg=%d vcoin=%d\n",
				vbat_uv, current_ma, batt_temp, batt_id, vusb_uv, vchg_uv,
				vcoin_uv);

	pr_info("ADC values: vbat=%d current=%d batt_temp=%d "
				"batt_id=%d vusb=%d vchg=%d vcoin=%d\n",
				vbat_uv, current_ma, batt_temp, batt_id, vusb_uv, vchg_uv,
				vcoin_uv);

	dsm_client_notify(dclient, type);

	return 0;
}
/* DTS2014081205934 jiangfei 20140820 end> */
/* DTS2014080807172 jiangfei 20140813 end> */
/* DTS2014062100152 jiangfei 20140623 end> */

/* <DTS2014062100152 jiangfei 20140623 begin */
/* this work is lanch by OVP, batt_pres abnormal, and online status error*/
static void qpnp_lbc_dump_work(struct work_struct *work)
{
	struct qpnp_lbc_chip	*chip =
		container_of(work, struct qpnp_lbc_chip, dump_work);

	pr_info("qpnp_lbc_dump_work is invoked! error type is %d\n",chip->error_type);
	dump_registers_and_adc(charger_dclient, chip, chip->error_type);
}

/*when usb is plugged in, we lanch this work to monitor the charging status*/
/* <DTS2014070701815 jiangfei 20140707 begin */
/* <DTS2014071001904 jiangfei 20140710 begin */
/*<DTS2014072301355 jiangfei 20140723 begin */
/*<DTS2014080807172 jiangfei 20140813 begin */
/*<DTS2014090201517 jiangfei 20140902 begin */
static void check_charging_status_work(struct work_struct *work)
{
	int vbat_uv, batt_temp, bat_present, cur_status, batt_level, usb_present, input_current;
	int chg_type, bat_current, resume_en;
	u8 bat_sts, chg_sts, usb_sts, chg_ctrl, suspend_en;
	static int not_charge_count = 0;
	int mode = 0;
	int rc = 0;
	int reg_value[5] = {0};
	//u8 reg = 0;
	int current_max = 0;
	union power_supply_propval val = {0};
	struct qpnp_lbc_chip *chip =
		container_of(work, struct qpnp_lbc_chip, check_charging_status_work.work);

	batt_temp = DEFAULT_TEMP;
	bat_present = get_prop_batt_present(chip);

	/* get /sys/class/power_supply/usb/current_max value */
	chip->usb_psy->get_property(chip->usb_psy,
			POWER_SUPPLY_PROP_CURRENT_MAX, &val);
	current_max = val.intval / 1000;

	if(!chip->use_other_charger){
		vbat_uv = get_prop_battery_voltage_now(chip);
		batt_temp = get_prop_batt_temp(chip);
		cur_status = get_prop_batt_status(chip);
		batt_level = get_prop_capacity(chip);
		/* < DTS2014092502339 zhaoxiaoli 20140929 begin */
		hw_adjuct_vinmin_set(vbat_uv,chip);
		/* DTS2014092502339 zhaoxiaoli 20140929 end> */
	}else{
		if(chip->ti_charger && chip->ti_charger->get_property){
			chip->ti_charger->get_property(chip->ti_charger,POWER_SUPPLY_PROP_VOLTAGE_NOW,&val);
			vbat_uv = val.intval;
			chip->ti_charger->get_property(chip->ti_charger,POWER_SUPPLY_PROP_TEMP,&val);
			batt_temp = val.intval;
			chip->ti_charger->get_property(chip->ti_charger,POWER_SUPPLY_PROP_STATUS,&val);
			cur_status = val.intval;
			chip->ti_charger->get_property(chip->ti_charger,POWER_SUPPLY_PROP_CAPACITY,&val);
			batt_level = val.intval;
		}
		mode = is_bq24152_in_boost_mode();
	}
	/* if usb ovp happens*/
	/*dump pmic registers and adc values, and notify to dsm server */
	if(is_usb_ovp(chip)){
		dump_registers_and_adc(charger_dclient, chip, DSM_CHARGER_CHG_OVP_ERROR_NO);
	}

	/*if all the charging conditions are avaliable, but it still cannot charge, */
	/*we report the error and dump the registers values and ADC values  */
	if((((HOT_TEMP_DEFAULT - HOT_TEMP_BUFFER) > batt_temp)&& (bat_present) && (0 == mode)&& (current_max > 2)
		&&(!chip->cfg_charging_disabled) && (vbat_uv <= (chip->cfg_warm_bat_mv - WARM_VOL_BUFFER))
		&& (batt_temp >= chip->cfg_warm_bat_decidegc) && (POWER_SUPPLY_STATUS_FULL != cur_status))
		|| ((!chip->cfg_charging_disabled) && (bat_present)
		&& (COLD_TEMP_DEFAULT < batt_temp) && (0 == mode)
		&& (batt_temp < (chip->cfg_warm_bat_decidegc - WARM_TEMP_BUFFER))
		&& (POWER_SUPPLY_STATUS_FULL != cur_status) && (current_max > 2))){

		if((POWER_SUPPLY_STATUS_DISCHARGING == cur_status)
			&& (BATT_FULL_LEVEL != batt_level)){
			chip->error_type = DSM_CHARGER_NOT_CHARGE_ERROR_NO;
			pr_err("cannot charge! error type is %d\n",chip->error_type);
			dump_registers_and_adc(charger_dclient, chip, DSM_CHARGER_NOT_CHARGE_ERROR_NO);
		}
	}

	/* Add NFF log here */
	usb_present = qpnp_lbc_is_usb_chg_plugged_in(chip);
	if(usb_present && (cur_status == POWER_SUPPLY_STATUS_DISCHARGING
		|| cur_status == POWER_SUPPLY_STATUS_NOT_CHARGING)
		&& !chip->cfg_charging_disabled && (0 == mode)
		&& BATT_FULL_LEVEL != batt_level
		&& (current_max > 2)){
		pr_info("current status need to be checked %d %d %d\n",cur_status,usb_present,chip->cfg_charging_disabled);
		/*not charging for more then 120s*/
		if(not_charge_count++ < NOT_CHARGE_COUNT){
			goto skip_check_status;
		}
		else{
			not_charge_count = 0;
		}
	}
	else{
		pr_debug("right status\n");
		not_charge_count = 0;
		goto skip_check_status;
	}

	/* For 8916 linear charger */
	if(!chip->use_other_charger){
		/* get the battery info and charge sts */
		/* Remove redundancy code  */
		chg_type = get_prop_charge_type(chip);
		bat_current = get_prop_current_now(chip);

		rc = qpnp_lbc_read(chip, INT_RT_STS(chip->bat_if_base), &bat_sts, 1);
		if (rc)
			pr_err("failed to read batt_sts rc=%d\n", rc);
		rc = qpnp_lbc_read(chip, INT_RT_STS(chip->chgr_base), &chg_sts, 1);
		if (rc)
			pr_err("failed to read chg_sts rc=%d\n", rc);
		rc = qpnp_lbc_read(chip, INT_RT_STS(chip->usb_chgpth_base), &usb_sts, 1);
		if (rc)
			pr_err("failed to read usb_sts rc=%d\n", rc);
		rc = qpnp_lbc_read(chip, chip->chgr_base + CHG_CTRL_REG, &chg_ctrl, 1);
		if (rc)
			pr_err("failed to read chg_ctrl rc=%d\n", rc);
		rc = qpnp_lbc_read(chip, chip->usb_chgpth_base + USB_SUSP_REG, &suspend_en, 1);
		if (rc)
			pr_err("failed to read suspend_en rc=%d\n", rc);

		resume_en = chip->resuming_charging;
		input_current = qpnp_lbc_ibatmax_get(chip);

		if(!bat_present){
			MSG_WRAPPER(CHARGE_ERROR_BASE|BATTERY_ERR,
				"%d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
				cur_status, batt_level, bat_present, vbat_uv, batt_temp, chg_type, bat_current,
				bat_sts, chg_sts, usb_sts, chg_ctrl, suspend_en, resume_en, input_current);
		}
		else if(batt_temp >= chip->cfg_hot_bat_decidegc || batt_temp <= chip->cfg_cold_bat_decidegc){
			MSG_WRAPPER(CHARGE_ERROR_BASE|TMEPERATURE_ERR|TMEPERATURE_OVERFLOW,
				"%d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
				cur_status, batt_level, bat_present, vbat_uv, batt_temp, chg_type, bat_current,
				bat_sts, chg_sts, usb_sts, chg_ctrl, suspend_en, resume_en, input_current);
		}
		else if(batt_temp < chip->cfg_hot_bat_decidegc && batt_temp >= chip->cfg_warm_bat_decidegc
			&& vbat_uv >= chip->cfg_warm_bat_mv){
			MSG_WRAPPER(CHARGE_ERROR_BASE|TMEPERATURE_ERR|TMEPERATURE_LIMIT,
				"%d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
				cur_status, batt_level, bat_present, vbat_uv, batt_temp, chg_type, bat_current,
				bat_sts, chg_sts, usb_sts, chg_ctrl, suspend_en, resume_en, input_current);
		}
		else{
			MSG_WRAPPER(CHARGE_ERROR_BASE,
				"%d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
				cur_status, batt_level, bat_present, vbat_uv, batt_temp, chg_type, bat_current,
				bat_sts, chg_sts, usb_sts, chg_ctrl, suspend_en, resume_en, input_current);
		}

		pr_info("%d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
			cur_status, batt_level, bat_present, vbat_uv, batt_temp, chg_type, bat_current,
			bat_sts, chg_sts, usb_sts, chg_ctrl, suspend_en, resume_en, input_current);

	/* For G760 Ti charger */
	}else{
		if(chip->ti_charger && chip->ti_charger->get_property){
			chip->ti_charger->get_property(chip->ti_charger,POWER_SUPPLY_PROP_CURRENT_NOW,&val);
			bat_current = val.intval;
		}
		/* read bq2415x reg0 to reg4 values */
#if 0
		for(reg = BQ2415X_REG_STATUS; reg <= BQ2415X_REG_CURRENT; reg++){
			reg_value[reg] = get_bq2415x_reg_values(reg);
		}
#endif
		if(!bat_present){
			MSG_WRAPPER(CHARGE_ERROR_BASE|BATTERY_ERR,
				"%d %d %d %d %d %d %x %x %x %x %x\n",
				cur_status, batt_level, bat_present, vbat_uv, batt_temp, bat_current,
				reg_value[0], reg_value[1], reg_value[2], reg_value[3], 
				reg_value[4]);
		}
		else if(batt_temp >= chip->cfg_hot_bat_decidegc || batt_temp <= chip->cfg_cold_bat_decidegc){
			MSG_WRAPPER(CHARGE_ERROR_BASE|TMEPERATURE_ERR|TMEPERATURE_OVERFLOW,
				"%d %d %d %d %d %d %x %x %x %x %x\n",
				cur_status, batt_level, bat_present, vbat_uv, batt_temp, bat_current,
				reg_value[0], reg_value[1], reg_value[2], reg_value[3], 
				reg_value[4]);
		}
		else if(batt_temp < chip->cfg_hot_bat_decidegc && batt_temp >= chip->cfg_warm_bat_decidegc
			&& vbat_uv >= chip->cfg_warm_bat_mv){
			MSG_WRAPPER(CHARGE_ERROR_BASE|TMEPERATURE_ERR|TMEPERATURE_LIMIT,
				"%d %d %d %d %d %d %x %x %x %x %x\n",
				cur_status, batt_level, bat_present, vbat_uv, batt_temp, bat_current,
				reg_value[0], reg_value[1], reg_value[2], reg_value[3], 
				reg_value[4]);
		}
		else{
			MSG_WRAPPER(CHARGE_ERROR_BASE,
				"%d %d %d %d %d %d %x %x %x %x %x\n",
				cur_status, batt_level, bat_present, vbat_uv, batt_temp, bat_current,
				reg_value[0], reg_value[1], reg_value[2], reg_value[3], 
				reg_value[4]);
		}

		pr_info("%d %d %d %d %d %d %x %x %x %x %x\n",
				cur_status, batt_level, bat_present, vbat_uv, batt_temp, bat_current,
				reg_value[0], reg_value[1], reg_value[2], reg_value[3], 
				reg_value[4]);
	}
skip_check_status:
	schedule_delayed_work(&chip->check_charging_status_work,
			msecs_to_jiffies(CHECKING_TIME));
}
#endif
/* DTS2014090201517 jiangfei 20140902 end> */
/* DTS2014080807172 jiangfei 20140813 end> */
/* DTS2014072301355 jiangfei 20140723 end> */
/* DTS2014071001904 jiangfei 20140710 end> */
/* DTS2014070701815 jiangfei 20140707 end> */
/* DTS2014062100152 jiangfei 20140623 end> */
/* DTS2014061002202 jiangfei 20140610 end> */
/* DTS2014053001058 jiangfei 20140530 end> */
#define IBAT_TRIM			-300
static void qpnp_lbc_vddtrim_work_fn(struct work_struct *work)
{
	int rc, vbat_now_uv, ibat_now;
	u8 reg_val;
	ktime_t kt;
	struct qpnp_lbc_chip *chip = container_of(work, struct qpnp_lbc_chip,
						vddtrim_work);

	vbat_now_uv = get_prop_battery_voltage_now(chip);
	ibat_now = get_prop_current_now(chip) / 1000;
	pr_debug("vbat %d ibat %d capacity %d\n",
			vbat_now_uv, ibat_now, get_prop_capacity(chip));

	/*
	 * Stop trimming under following condition:
	 * USB removed
	 * Charging Stopped
	 */
	if (!qpnp_lbc_is_fastchg_on(chip) ||
			!qpnp_lbc_is_usb_chg_plugged_in(chip)) {
		pr_debug("stop trim charging stopped\n");
		goto exit;
	} else {
		rc = qpnp_lbc_read(chip, chip->chgr_base + CHG_STATUS_REG,
					&reg_val, 1);
		if (rc) {
			pr_err("Failed to read chg status rc=%d\n", rc);
			goto out;
		}

		/*
		 * Update VDD trim voltage only if following conditions are
		 * met:
		 * If charger is in VDD loop AND
		 * If ibat is between 0 ma and -300 ma
		 */
		if ((reg_val & CHG_VDD_LOOP_BIT) &&
				((ibat_now < 0) && (ibat_now > IBAT_TRIM)))
			qpnp_lbc_adjust_vddmax(chip, vbat_now_uv);
	}

out:
	kt = ns_to_ktime(TRIM_PERIOD_NS);
	alarm_start_relative(&chip->vddtrim_alarm, kt);
exit:
	pm_relax(chip->dev);
}

static enum alarmtimer_restart vddtrim_callback(struct alarm *alarm,
					ktime_t now)
{
	struct qpnp_lbc_chip *chip = container_of(alarm, struct qpnp_lbc_chip,
						vddtrim_alarm);

	pm_stay_awake(chip->dev);
	schedule_work(&chip->vddtrim_work);

	return ALARMTIMER_NORESTART;
}
/* DTS2014072806213 by l00220156 for factory interfaces 20140728 begin */
static ssize_t equip_at_batt_present_get_cdcval(struct EQUIP_PARAM* arg)
{
    int ret = 0, batt_present = 0;

    if (NULL == arg){
        pr_err("%s: NULL arg\n", __func__);
        return -EINVAL;
    }
    
    batt_present = get_prop_batt_present(global_chip);
    ret = snprintf(arg->str_out, MAX_SHOW_STRING_LENTH, "%d\n", batt_present);
    pr_err("%s: batt_present =%s\n", __func__,arg->str_out);
    
    return ret;
}

static ssize_t equip_at_batt_voltage_get_cdcval(struct EQUIP_PARAM* arg)
{
    int ret = 0, batt_volt = 0;

    if (NULL == arg){
        pr_err("%s: NULL arg\n", __func__);
        return -EINVAL;
    }

    batt_volt = get_prop_battery_voltage_now(global_chip);
    ret = snprintf(arg->str_out, MAX_SHOW_STRING_LENTH, "%d\n", batt_volt);
    pr_err("%s: batt_present =%s\n", __func__,arg->str_out);
    
    return ret;
}

static ssize_t equip_at_batt_temp_get_cdcval(struct EQUIP_PARAM* arg)
{
    int ret = 0, batt_temp = 0;

    if (NULL == arg){
        pr_err("%s: NULL arg\n", __func__);
        return -EINVAL;
    }

    batt_temp = get_prop_batt_temp(global_chip);
    ret = snprintf(arg->str_out, MAX_SHOW_STRING_LENTH, "%d\n", batt_temp);
    pr_err("%s: batt_present =%s\n", __func__,arg->str_out);
    
    return ret;
}

static ssize_t equip_at_batt_capacity_get_cdcval(struct EQUIP_PARAM* arg)
{
    int ret = 0, batt_cap = 0;

    if (NULL == arg){
        pr_err("%s: NULL arg\n", __func__);
        return -EINVAL;
    }

    batt_cap = get_prop_capacity(global_chip);
    ret = snprintf(arg->str_out, MAX_SHOW_STRING_LENTH, "%d\n", batt_cap);
    pr_err("%s: batt_present =%s\n", __func__,arg->str_out);
    
    return ret;
}

static ssize_t equip_at_batt_current_get_cdcval(struct EQUIP_PARAM* arg)
{
    int ret = 0, batt_curr = 0;

    if (NULL == arg){
        pr_err("%s: NULL arg\n", __func__);
        return -EINVAL;
    }

    batt_curr = get_prop_current_now(global_chip);
    ret = snprintf(arg->str_out, MAX_SHOW_STRING_LENTH, "%d\n", batt_curr);
    pr_err("%s: batt_curr =%s\n", __func__,arg->str_out);
    
    return ret;
}

static ssize_t equip_at_batt_charge_enable_set_cdcval(struct EQUIP_PARAM* arg)
{
    int ret = 0, val = 0;

    if (NULL == arg){
        pr_err("%s: NULL arg\n", __func__);
        return -EINVAL;
    }
    
    if ((strict_strtol(arg->str_in, 10, (long*)&val) < 0) || (val < 0) || (val > 1))
    {   
        pr_err("%s: arg->str_in err\n", __func__);
        return -EINVAL;
    }
    
    ret = bq2419x_equip_at_batt_charge_enable(val);
    pr_err("%s: ret =%d \n", __func__, ret);
    
    return ret;
}
/* <DTS2014110409521 caiwei 20141104 begin */
static ssize_t equip_at_hot_temperate_test_set_cdcval(struct EQUIP_PARAM* arg)
{
    int ret = 0, val = 0;

    if (NULL == arg)
    {
        pr_err("%s NULL arg\n", __func__);
        return -EINVAL;
    }

    if ((strict_strtol(arg->str_in, 10, (long*)&val) < 0) || (val < 0) || (val > 1))
    {
        pr_err("%s: arg->str_in err\n", __func__);
        return -EINVAL;
    }

    ret = equip_hot_temperate_test_set(global_chip, val);
    pr_err("%s: ret =%d \n", __func__, ret);

    return ret;
}

static ssize_t equip_at_hot_temperate_test_get_cdcval(struct EQUIP_PARAM* arg)
{
    int ret = 0, val = 0;

    if (NULL == arg)
    {
        pr_err("%s NULL arg\n", __func__);
        return -EINVAL;
    }

    if ((strict_strtol(arg->str_in, 10, (long*)&val) < 0) || (val < 0) || (val > 1))
    {
        pr_err("%s: arg->str_in err\n", __func__);
        return -EINVAL;
    }

    ret = equip_hot_temperate_test_get(global_chip);
    pr_err("%s: ret =%d \n", __func__, ret);

    return ret;
}
/* DTS2014110409521 caiwei 20141104 end> */
static void huawei_psy_register_equip(void)
{
    enum EQUIP_DEVICE equip_dev = 0;
    enum EQUIP_OPS ops = 0;

    /****************************** ����AT�����һ���ӿ� *************************/
    equip_dev = COULOMETER;
    ops = EP_READ;
    register_equip_func(equip_dev, ops, (void *)equip_at_batt_present_get_cdcval);
    ops = EP_WRITE;
    register_equip_func(equip_dev, ops, NULL);

    /***** ��ȡ��ص�ѹ����λuV *****/
    equip_dev = BATT_VOLTAGE;
    ops = EP_READ;
    register_equip_func(equip_dev, ops, (void *)equip_at_batt_voltage_get_cdcval);
    ops = EP_WRITE;
    register_equip_func(equip_dev, ops, NULL);

    /***** ��ȡ��ص�ѹ����λuV *****/

    /***** ��ȡ����¶� ����λ0.1���϶�*****/
    equip_dev = BATT_TEMP;
    ops = EP_READ;
    register_equip_func(equip_dev, ops, (void *)equip_at_batt_temp_get_cdcval);
    ops = EP_WRITE;
    register_equip_func(equip_dev, ops, NULL);

    /***** ��ȡ����¶� ����λ0.1���϶�*****/

    /***** ��ȡ��ص�ǰ�����ٷֱ� *****/
    equip_dev = BATT_CAPACITY;
    ops = EP_READ;
    register_equip_func(equip_dev, ops, (void *)equip_at_batt_capacity_get_cdcval);
    ops = EP_WRITE;
    register_equip_func(equip_dev, ops, NULL);

    /***** ��ȡ��ص�ǰ�����ٷֱ� *****/

    /***** ��ȡ������ص�������λmA *****/
    equip_dev = BATT_CURRENT;
    ops = EP_READ;
    register_equip_func(equip_dev, ops, (void *)equip_at_batt_current_get_cdcval);
    ops = EP_WRITE;
    register_equip_func(equip_dev, ops, NULL);

    /***** ��ȡ������ص�������λmA *****/

    /***** ���ʹ�ܽӿ� *****/
    equip_dev = BATT_CHARGE_ENABLE;
    ops = EP_READ;

    /* ��ѯ */
    register_equip_func(equip_dev, ops, NULL);
    ops = EP_WRITE;

    /* ���� */
    register_equip_func(equip_dev, ops, (void *)equip_at_batt_charge_enable_set_cdcval);

    /***** ���ʹ�ܽӿ� *****/
    /* <DTS2014110409521 caiwei 20141104 begin */
    /***** ���²���ʹ�ܽӿ�*****/
    equip_dev = HOT_TEMP_TEST;
    ops = EP_WRITE;
    register_equip_func(equip_dev, ops, (void*)equip_at_hot_temperate_test_set_cdcval);
    ops = EP_READ;
    register_equip_func(equip_dev, ops, (void*)equip_at_hot_temperate_test_get_cdcval);
    /* DTS2014110409521 caiwei 20141104 end> */
    /****************************** ����AT�����һ���ӿ� *************************/
}
/* DTS2014072806213 by l00220156 for factory interfaces 20140728 end */

static int qpnp_lbc_probe(struct spmi_device *spmi)
{
	u8 subtype;
	ktime_t kt;
	struct qpnp_lbc_chip *chip;
	struct resource *resource;
	struct spmi_resource *spmi_resource;
	struct power_supply *usb_psy;
	int rc = 0;

	usb_psy = power_supply_get_by_name("usb");
	if (!usb_psy) {
		pr_err("usb supply not found deferring probe\n");
		return -EPROBE_DEFER;
	}

	chip = devm_kzalloc(&spmi->dev, sizeof(struct qpnp_lbc_chip),
				GFP_KERNEL);
	if (!chip) {
		pr_err("memory allocation failed.\n");
		return -ENOMEM;
	}

	chip->usb_psy = usb_psy;
	chip->dev = &spmi->dev;
	chip->spmi = spmi;
	chip->fake_battery_soc = -EINVAL;
    /* <DTS2014071002612  mapengfei 20140710 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	chip->cfg_soc_resume_charging = false;
#endif
   /* DTS2014071002612  mapengfei 20140710 end> */
	dev_set_drvdata(&spmi->dev, chip);
	device_init_wakeup(&spmi->dev, 1);
	mutex_init(&chip->jeita_configure_lock);
	mutex_init(&chip->chg_enable_lock);
	spin_lock_init(&chip->hw_access_lock);
	spin_lock_init(&chip->ibat_change_lock);
	spin_lock_init(&chip->irq_lock);
	INIT_WORK(&chip->vddtrim_work, qpnp_lbc_vddtrim_work_fn);
	alarm_init(&chip->vddtrim_alarm, ALARM_REALTIME, vddtrim_callback);
/* <DTS2014093003947 (report SOC 100%) caiwei 20141009 begin */
    getnstimeofday(&g_last_ts);
/* DTS2014093003947 (report SOC 100%) caiwei 20141009 end> */
	/* <DTS2014042804721 chenyuanquan 20140428 begin */
	/* <DTS2014051601896 jiangfei 20140516 begin */
	/*<DTS2014053001058 jiangfei 20140530 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	chip->cfg_cold_bat_decidegc = COLD_TEMP_DEFAULT;
	chip->cfg_hot_bat_decidegc = HOT_TEMP_DEFAULT;
	chip->running_test_settled_status = POWER_SUPPLY_STATUS_CHARGING;
#endif
#ifdef CONFIG_HUAWEI_DSM
	chip->error_type = -EINVAL;
#endif
	/* DTS2014053001058 jiangfei 20140530 end> */
	/* DTS2014051601896 jiangfei 20140516 end> */
	/* DTS2014042804721 chenyuanquan 20140428 end> */
	/* Get all device-tree properties */
	rc = qpnp_charger_read_dt_props(chip);
	if (rc) {
		pr_err("Failed to read DT properties rc=%d\n", rc);
		return rc;
	}

	spmi_for_each_container_dev(spmi_resource, spmi) {
		if (!spmi_resource) {
			pr_err("spmi resource absent\n");
			rc = -ENXIO;
			goto fail_chg_enable;
		}

		resource = spmi_get_resource(spmi, spmi_resource,
							IORESOURCE_MEM, 0);
		if (!(resource && resource->start)) {
			pr_err("node %s IO resource absent!\n",
						spmi->dev.of_node->full_name);
			rc = -ENXIO;
			goto fail_chg_enable;
		}

		rc = qpnp_lbc_read(chip, resource->start + PERP_SUBTYPE_REG,
					&subtype, 1);
		if (rc) {
			pr_err("Peripheral subtype read failed rc=%d\n", rc);
			goto fail_chg_enable;
		}

		switch (subtype) {
		case LBC_CHGR_SUBTYPE:
			chip->chgr_base = resource->start;

			/* Get Charger peripheral irq numbers */
			rc = qpnp_lbc_get_irqs(chip, subtype, spmi_resource);
			if (rc) {
				pr_err("Failed to get CHGR irqs rc=%d\n", rc);
				goto fail_chg_enable;
			}
			break;
		case LBC_USB_PTH_SUBTYPE:
			chip->usb_chgpth_base = resource->start;
			rc = qpnp_lbc_get_irqs(chip, subtype, spmi_resource);
			if (rc) {
				pr_err("Failed to get USB_PTH irqs rc=%d\n",
						rc);
				goto fail_chg_enable;
			}
			break;
		case LBC_BAT_IF_SUBTYPE:
			chip->bat_if_base = resource->start;
			chip->vadc_dev = qpnp_get_vadc(chip->dev, "chg");
			if (IS_ERR(chip->vadc_dev)) {
				rc = PTR_ERR(chip->vadc_dev);
				if (rc != -EPROBE_DEFER)
					pr_err("vadc prop missing rc=%d\n",
							rc);
				goto fail_chg_enable;
			}
			/* Get Charger Batt-IF peripheral irq numbers */
			rc = qpnp_lbc_get_irqs(chip, subtype, spmi_resource);
			if (rc) {
				pr_err("Failed to get BAT_IF irqs rc=%d\n", rc);
				goto fail_chg_enable;
			}
			break;
		case LBC_MISC_SUBTYPE:
			chip->misc_base = resource->start;
			break;
		default:
			pr_err("Invalid peripheral subtype=0x%x\n", subtype);
			rc = -EINVAL;
		}
	}

	if (chip->cfg_use_external_charger) {
		pr_warn("Disabling Linear Charger (e-external-charger = 1)\n");
		rc = qpnp_disable_lbc_charger(chip);
		if (rc)
			pr_err("Unable to disable charger rc=%d\n", rc);
		return -ENODEV;
	}

	/* <DTS2014042804721 chenyuanquan 20140428 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	rc = qpnp_chg_load_battery_data(chip);
	if (rc)
		goto fail_chg_enable;
#endif
	/* DTS2014042804721 chenyuanquan 20140428 end> */
	/* Initialize h/w */
	rc = qpnp_lbc_misc_init(chip);
	if (rc) {
		pr_err("unable to initialize LBC MISC rc=%d\n", rc);
		return rc;
	}
	rc = qpnp_lbc_chg_init(chip);
	if (rc) {
		pr_err("unable to initialize LBC charger rc=%d\n", rc);
		return rc;
	}
	rc = qpnp_lbc_bat_if_init(chip);
	if (rc) {
		pr_err("unable to initialize LBC BAT_IF rc=%d\n", rc);
		return rc;
	}
	rc = qpnp_lbc_usb_path_init(chip);
	if (rc) {
		pr_err("unable to initialize LBC USB path rc=%d\n", rc);
		return rc;
	}

	if (chip->bat_if_base) {
		chip->batt_present = qpnp_lbc_is_batt_present(chip);
		chip->batt_psy.name = "battery";
		/* <DTS2014052906550 liyuping 20140530 begin */
#ifdef CONFIG_HUAWEI_KERNEL
        /*DTS2014091301574,begin:rm linear charger, 2014.9.30*/
        if(chip->use_other_charger_pad)
		{
		    chip->batt_psy.type = POWER_SUPPLY_TYPE_BATTERY;//for pad
		}
		else if(!chip->use_other_charger)
		/*DTS2014091301574,end:rm linear charger, 2014.9.30*/
			chip->batt_psy.type = POWER_SUPPLY_TYPE_BATTERY;
		else
			chip->batt_psy.type = POWER_SUPPLY_TYPE_UNKNOWN;
#else
		chip->batt_psy.type = POWER_SUPPLY_TYPE_BATTERY;
#endif
		/* DTS2014052906550 liyuping 20140530 end> */
		chip->batt_psy.properties = msm_batt_power_props;
		chip->batt_psy.num_properties =
			ARRAY_SIZE(msm_batt_power_props);
		chip->batt_psy.get_property = qpnp_batt_power_get_property;
		chip->batt_psy.set_property = qpnp_batt_power_set_property;
		chip->batt_psy.property_is_writeable =
			qpnp_batt_property_is_writeable;
		chip->batt_psy.external_power_changed =
			qpnp_batt_external_power_changed;
		chip->batt_psy.supplied_to = pm_batt_supplied_to;
		chip->batt_psy.num_supplicants =
			ARRAY_SIZE(pm_batt_supplied_to);
		rc = power_supply_register(chip->dev, &chip->batt_psy);
		if (rc < 0) {
			pr_err("batt failed to register rc=%d\n", rc);
			goto fail_chg_enable;
		}
	}

	if ((chip->cfg_cool_bat_decidegc || chip->cfg_warm_bat_decidegc)
			&& chip->bat_if_base) {
		chip->adc_param.low_temp = chip->cfg_cool_bat_decidegc;
		chip->adc_param.high_temp = chip->cfg_warm_bat_decidegc;
		chip->adc_param.timer_interval = ADC_MEAS1_INTERVAL_1S;
		chip->adc_param.state_request = ADC_TM_HIGH_LOW_THR_ENABLE;
		chip->adc_param.btm_ctx = chip;
		chip->adc_param.threshold_notification =
			qpnp_lbc_jeita_adc_notification;
		chip->adc_param.channel = LR_MUX1_BATT_THERM;

		if (get_prop_batt_present(chip)) {
			rc = qpnp_adc_tm_channel_measure(chip->adc_tm_dev,
					&chip->adc_param);
			if (rc) {
				pr_err("request ADC error rc=%d\n", rc);
				goto unregister_batt;
			}
		}
	}

	rc = qpnp_lbc_bat_if_configure_btc(chip);
	if (rc) {
		pr_err("Failed to configure btc rc=%d\n", rc);
		goto unregister_batt;
	}

	/* Get/Set charger's initial status */
	determine_initial_status(chip);

    /* <DTS2014070819388 zhaoxiaoli 20140711 begin */
	/* <DTS2014081410316 jiangfei 20140819 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	wake_lock_init(&chip->led_wake_lock, WAKE_LOCK_SUSPEND, "pm8916_led");
	wake_lock_init(&chip->chg_wake_lock, WAKE_LOCK_SUSPEND, "pm8916_chg");
#endif
	/* DTS2014081410316 jiangfei 20140819 end> */
    /* DTS2014070819388 zhaoxiaoli 20140711 end> */

	/*<DTS2014053001058 jiangfei 20140530 begin */
	/* <DTS2014071001904 jiangfei 20140710 begin */
	/* <DTS2014071608042 jiangfei 20140716 begin */
#ifdef CONFIG_HUAWEI_DSM
	/*Add two works, first work func is used to dump log, second work func is */
	/*checking charging status every 30 seconds */
	INIT_WORK(&chip->dump_work, qpnp_lbc_dump_work);
	/*DTS2014091301574,begin:rm linear charger, 2014.9.30*/
	if(!chip->use_other_charger_pad)
	{
	    INIT_DELAYED_WORK(&chip->check_charging_status_work, check_charging_status_work);
	}
	/*DTS2014091301574,end:rm linear charger, 2014.9.30*/

	if (!charger_dclient) {
		charger_dclient = dsm_register_client(&dsm_charger);
	}
	if(qpnp_lbc_is_usb_chg_plugged_in(chip)){
		/* <DTS2014081410316 jiangfei 20140819 begin */
		wake_lock(&chip->chg_wake_lock);
		/* DTS2014081410316 jiangfei 20140819 end> */
		/*DTS2014091301574,begin:rm linear charger, 2014.9.30*/
		if(!chip->use_other_charger_pad)
        {
	        schedule_delayed_work(&chip->check_charging_status_work,
			      msecs_to_jiffies(DELAY_TIME));
	    }
		/*DTS2014091301574,end:rm linear charger, 2014.9.30*/
	}
	g_lbc_chip = chip;
#endif
	/* DTS2014071001904 jiangfei 20140710 end> */
	/* DTS2014053001058 jiangfei 20140530 end> */
	rc = qpnp_lbc_request_irqs(chip);
	if (rc) {
		pr_err("unable to initialize LBC MISC rc=%d\n", rc);
		goto unregister_batt;
	}

	if (chip->cfg_charging_disabled && !get_prop_batt_present(chip))
		pr_info("Battery absent and charging disabled !!!\n");
	/* DTS2014071608042 jiangfei 20140716 end> */

	/* Configure initial alarm for VDD trim */
	if ((chip->supported_feature_flag & VDD_TRIM_SUPPORTED) &&
			qpnp_lbc_is_fastchg_on(chip)) {
		kt = ns_to_ktime(TRIM_PERIOD_NS);
		alarm_start_relative(&chip->vddtrim_alarm, kt);
	}

	pr_info("Probe chg_dis=%d bpd=%d usb=%d batt_pres=%d batt_volt=%d soc=%d\n",
			chip->cfg_charging_disabled,
			chip->cfg_bpd_detection,
			qpnp_lbc_is_usb_chg_plugged_in(chip),
			get_prop_batt_present(chip),
			get_prop_battery_voltage_now(chip),
			get_prop_capacity(chip));
            /* DTS2014072806213 by l00220156 for factory interfaces 20140728 begin */
            huawei_psy_register_equip();
            /* DTS2014072806213 by l00220156 for factory interfaces 20140728 end */
    /* < DTS2014052901670 zhaoxiaoli 20140529 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	global_chip = chip;
#endif
    /* DTS2014052901670 zhaoxiaoli 20140529 end> */
	return 0;

unregister_batt:
	if (chip->bat_if_base)
		power_supply_unregister(&chip->batt_psy);
fail_chg_enable:
	dev_set_drvdata(&spmi->dev, NULL);
	/*<DTS2014053001058 jiangfei 20140530 begin */
#ifdef CONFIG_HUAWEI_DSM
	g_lbc_chip = NULL;
#endif
	/* DTS2014053001058 jiangfei 20140530 end> */
	return rc;
}

static int qpnp_lbc_remove(struct spmi_device *spmi)
{
	struct qpnp_lbc_chip *chip = dev_get_drvdata(&spmi->dev);
    /* <DTS2014070819388 zhaoxiaoli 20140711 begin */
	/* <DTS2014081410316 jiangfei 20140819 begin */
#ifdef CONFIG_HUAWEI_KERNEL
	wake_lock_destroy(&chip->led_wake_lock);
	wake_lock_destroy(&chip->chg_wake_lock);
#endif
	/* DTS2014081410316 jiangfei 20140819 end> */
    /* DTS2014070819388 zhaoxiaoli 20140711 end> */

	/*<DTS2014053001058 jiangfei 20140530 begin */
#ifdef CONFIG_HUAWEI_DSM
	cancel_work_sync(&chip->dump_work);
	/*DTS2014091301574,begin:rm linear charger, 2014.9.30*/
	if(!chip->use_other_charger_pad)
	{
	    cancel_delayed_work_sync(&chip->check_charging_status_work);
	}
	/*DTS2014091301574,end:rm linear charger, 2014.9.30*/
	g_lbc_chip = NULL;
#endif
	/* DTS2014053001058 jiangfei 20140530 end> */

	if (chip->supported_feature_flag & VDD_TRIM_SUPPORTED) {
		alarm_cancel(&chip->vddtrim_alarm);
		cancel_work_sync(&chip->vddtrim_work);
	}
	if (chip->bat_if_base)
		power_supply_unregister(&chip->batt_psy);
	mutex_destroy(&chip->jeita_configure_lock);
	mutex_destroy(&chip->chg_enable_lock);
	dev_set_drvdata(&spmi->dev, NULL);
	return 0;
}

/*
 * S/W workaround to force VREF_BATT_THERM ON:
 * Switching between aVDD and LDO has h/w issues, forcing VREF_BATT_THM
 * alway ON to prevent switching to aVDD.
 */

static int qpnp_lbc_resume(struct device *dev)
{
	struct qpnp_lbc_chip *chip = dev_get_drvdata(dev);
	int rc = 0;

	if (chip->bat_if_base) {
		rc = qpnp_lbc_masked_write(chip,
			chip->bat_if_base + BAT_IF_VREF_BAT_THM_CTRL_REG,
			VREF_BATT_THERM_FORCE_ON, VREF_BATT_THERM_FORCE_ON);
		if (rc)
			pr_err("Failed to force on VREF_BAT_THM rc=%d\n", rc);
	}

	return rc;
}

static int qpnp_lbc_suspend(struct device *dev)
{
	struct qpnp_lbc_chip *chip = dev_get_drvdata(dev);
	int rc = 0;

	if (chip->bat_if_base) {
		rc = qpnp_lbc_masked_write(chip,
			chip->bat_if_base + BAT_IF_VREF_BAT_THM_CTRL_REG,
			VREF_BATT_THERM_FORCE_ON, VREF_BAT_THM_ENABLED_FSM);
		if (rc)
			pr_err("Failed to set FSM VREF_BAT_THM rc=%d\n", rc);
	}

	return rc;
}

static const struct dev_pm_ops qpnp_lbc_pm_ops = {
	.resume		= qpnp_lbc_resume,
	.suspend	= qpnp_lbc_suspend,
};

static struct of_device_id qpnp_lbc_match_table[] = {
	{ .compatible = QPNP_CHARGER_DEV_NAME, },
	{}
};

static struct spmi_driver qpnp_lbc_driver = {
	.probe		= qpnp_lbc_probe,
	.remove		= qpnp_lbc_remove,
	.driver		= {
		.name		= QPNP_CHARGER_DEV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= qpnp_lbc_match_table,
		.pm		= &qpnp_lbc_pm_ops,
	},
};

/*
 * qpnp_lbc_init() - register spmi driver for qpnp-chg
 */
static int __init qpnp_lbc_init(void)
{
	return spmi_driver_register(&qpnp_lbc_driver);
}
module_init(qpnp_lbc_init);

static void __exit qpnp_lbc_exit(void)
{
	spmi_driver_unregister(&qpnp_lbc_driver);
}
module_exit(qpnp_lbc_exit);

MODULE_DESCRIPTION("QPNP Linear charger driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" QPNP_CHARGER_DEV_NAME);

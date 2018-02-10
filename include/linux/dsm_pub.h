/**********************************************************
 * Filename:	dsm_pub.h
 *
 * Discription: Huawei device state monitor public head file
 *
 * Copyright: (C) 2014 huawei.
 *
 * Author: sjm
 *
**********************************************************/

#ifndef _DSM_PUB_H
#define _DSM_PUB_H
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/wait.h>

extern int debug_output;

#define DSM_LOG_DEBUG(format, ...)				\
	do {						\
		if (debug_output)			\
			 printk("[DSM] "format,## __VA_ARGS__);\
	} while (0)


#define DSM_LOG_INFO(format, ...)		printk("[DSM] "format,## __VA_ARGS__)
#define DSM_LOG_ERR(format, ...)		printk("[DSM] "format,## __VA_ARGS__)

#define CLIENT_NAME_LEN		(32)

/*dsm error no define*/
#define DSM_ERR_NO_ERROR		(0)
#define DSM_ERR_I2C_TIMEOUT		(1)

/*<DTS2014053001058 jiangfei 20140530 begin */
/* <DTS2014062100152 jiangfei 20140623 begin */
/* <DTS2014112905428 jiangfei 20141201 begin */
/* charger, bms and pmu error mumbers*/
#ifdef CONFIG_HUAWEI_DSM
#define DSM_BMS_POWON_SOC_ERROR_NO				(10100)
#define DSM_BMS_NORMAL_SOC_ERROR_NO				(10101)
#define DSM_BMS_BQ_UPDATE_FIRMWARE_FAIL_NO			(10102)
#define DSM_CHARGER_ADC_ABNORMAL_ERROR_NO  		(10206)
#define DSM_CHARGER_BQ_BOOST_FAULT_ERROR_NO  		(10207)
#define DSM_CHARGER_BQ_NORMAL_FAULT_ERROR_NO  	(10208)
#define DSM_CHARGER_BQ_I2C_ERROR_NO  				(10209)

enum DSM_BMS_ERR
{
	DSM_BMS_NORMAL_SOC_CHANGE_MUCH 	= 10100,
	DSM_BMS_VOL_SOC_DISMATCH_1,
	DSM_BMS_VOL_SOC_DISMATCH_2,
	DSM_BMS_VOL_SOC_DISMATCH_3,
	DSM_VM_BMS_VOL_SOC_DISMATCH_4,
	DSM_BMS_SOC_CHANGE_PLUG_INOUT,
	DSM_FUEL_GAUGE_I2C_ERROR,
	DSM_MAXIM_HANDLE_MODEL_FAIL,
	DSM_MAXIM_GET_VER_FAIL,
	DSM_MAXIM_REAL_SOC_OVER_100,
	DSM_BMS_POWON_SOC_CHANGE_MUCH,
};

enum DSM_CHARGER_ERR
{
	DSM_NOT_CHARGE_WHEN_ALLOWED 		= 10200,
	DSM_SPMI_ABNORMAL_ERROR_NO,
	DSM_CHG_OVP_ERROR_NO,
	DSM_BATT_PRES_ERROR_NO,
	DSM_WARM_CURRENT_LIMIT_FAIL,
	DSM_COOL_CURRENT_LIMIT_FAIL,
	DSM_FULL_WHEN_CHARGER_ABSENT,
	DSM_CHARGER_ONLINE_ABNORMAL_ERROR_NO,
	DSM_BATT_VOL_OVER_4400,
	DSM_BATT_VOL_TOO_LOW,
	DSM_FAKE_FULL,
	DSM_ABNORMAL_CHARGE_STATUS,
	DSM_NONSTANDARD_CHARGER_DETETED,
	DSM_STILL_CHARGE_WHEN_HOT,
	DSM_STILL_CHARGE_WHEN_COLD,
	DSM_STILL_CHARGE_WHEN_SET_DISCHARGE,
	DSM_STILL_CHARGE_WHEN_VOL_OVER_4350,
	DSM_HEATH_OVERHEAT,
	DSM_BATTERY_ID_UNKNOW,
	DSM_BATT_TEMP_JUMP,
	DSM_BATT_TEMP_BELOW_0,
	DSM_BATT_TEMP_OVER_60,
	DSM_NOT_CHARGING_WHEN_HOT,
	DSM_NOT_CHARGING_WHEN_COLD,
	DSM_USBIN_IRQ_INVOKE_TOO_QUICK,
	DSM_CHARGE_DISABLED_IN_USER_VERSION,
	DSM_SET_FACTORY_DIAG_IN_USER_VERSION,
	DSM_MAXIM_BAT_CONTACT_BREAK,
	DSM_MAXIM_BATTERY_REMOVED,
	DSM_MAXIM_BATT_UVP,
	DSM_MAXIM_DC_OVP,
	DSM_MAXIM_DC_UVP,
	DSM_MAXIM_AICL_NOK,
	DSM_MAXIM_VBAT_OVP,
	DSM_MAXIM_TEMP_SHUTDOWN,
	DSM_MAXIM_TIMER_FAULT,
	DSM_MAXIM_OTG_OVERCURRENT,
	DSM_MAXIM_USB_SUSPEND,
	DSM_MAXIM_ENABLE_OTG_FAIL,
	DSM_MAXIM_I2C_ERROR,
	DSM_MAXIM_CHARGE_WHEN_OTGEN,
	DSM_MAXIM_HEATTH_COLD,
	DSM_LINEAR_USB_OVERTEMP,
	DSM_LINEAR_CHG_FAILED,
	DSM_LINEAR_CHG_OVERCURRENT,
	DSM_LINEAR_BAT_OVERCURRENT,
};

enum DSM_PMU_ERR
{
	DSM_ABNORMAL_POWERON_REASON_1 		= 10300,
	DSM_ABNORMAL_POWERON_REASON_2,
	DSM_ABNORMAL_POWEROFF_REASON_1,
	DSM_ABNORMAL_POWEROFF_REASON_2,
	DSM_ABNORMAL_POWEROFF_REASON_3,
	DSM_ABNORMAL_POWEROFF_REASON_4,
	DSM_CPU_OVERTEMP,
	DSM_PA_OVERTEMP,
	DSM_THERMAL_ZONE2_OVERTEMP,
	DSM_THERMAL_ZONE4_OVERTEMP,
	DSM_LDO1_VOLTAGE_LOW,
	DSM_LDO2_VOLTAGE_LOW,
	DSM_LDO3_VOLTAGE_LOW,
	DSM_LDO4_VOLTAGE_LOW,
	DSM_LDO5_VOLTAGE_LOW,
	DSM_LDO6_VOLTAGE_LOW,
	DSM_LDO7_VOLTAGE_LOW,
	DSM_LDO8_VOLTAGE_LOW,
	DSM_LDO9_VOLTAGE_LOW,
	DSM_LDO10_VOLTAGE_LOW,
	DSM_LDO11_VOLTAGE_LOW,
	DSM_LDO12_VOLTAGE_LOW,
	DSM_LDO13_VOLTAGE_LOW,
	DSM_LDO14_VOLTAGE_LOW,
	DSM_LDO15_VOLTAGE_LOW,
	DSM_LDO16_VOLTAGE_LOW,
	DSM_LDO17_VOLTAGE_LOW,
	DSM_LDO18_VOLTAGE_LOW,
};
#endif
/* DTS2014112905428 jiangfei 20141201 end> */
/* DTS2014062100152 jiangfei 20140623 end> */
/* DTS2014053001058 jiangfei 20140530 end> */

/* touch panel error numbers */
#define DSM_TP_I2C_RW_ERROR_NO 				(20000)		// read or write i2c error
#define DSM_TP_FW_ERROR_NO 					(20001)		// fw has error
#define DSM_TP_DISTURB_ERROR_NO 			(20002)		// be disturbed by external condition
#define DSM_TP_IC_ERROR_NO 					(20003)		// ic has error
#define DSM_TP_BROKEN_ERROR_NO 				(20004)		// hardware broken
/* < DTS2014082605600 s00171075 20140830 begin */
/* < DTS2014061500170 yanghaizhou 20140618 begin */
#define DSM_TP_ESD_ERROR_NO					(20005)		// esd check error
/* DTS2014061500170 yanghaizhou 20140618 end > */
#define DSM_TP_F34_PDT_ERROR_NO 			(20006)		// fail to read pdt
#define DSM_TP_F54_PDT_ERROR_NO 			(20007)		// fail to read f54 pdt
/* DTS2014082605600 s00171075 20140830 end > */
/* < DTS2014110302206 songrongyuan 20141103 begin */
#define DSM_TP_PDT_PROPS_ERROR_NO 			(20008)		// fail to read pdt props
#define DSM_TP_F34_READ_QUERIES_ERROR_NO 	(20009)		// fail to read f34 queries
/* DTS2014110302206 songrongyuan 20141103 end > */

/* LCD error numbers */
/* < DTS2014051603610 zhaoyuxia 20140516 begin */
/* modify lcd error macro */
#define DSM_LCD_MIPI_ERROR_NO				(20100)
#define DSM_LCD_TE_TIME_OUT_ERROR_NO		(20101)
#define DSM_LCD_STATUS_ERROR_NO				(20102)
#define DSM_LCD_PWM_ERROR_NO				(20104)
#define DSM_LCD_BRIGHTNESS_ERROR_NO			(20105)
#define DSM_LCD_MDSS_DSI_ISR_ERROR_NO		(20106)
#define DSM_LCD_MDSS_MDP_ISR_ERROR_NO		(20107)
/* < DTS2014062405194 songliangliang 20140624 begin */
/* modify lcd error macro */
#define DSM_LCD_ESD_STATUS_ERROR_NO                    (20108)
#define DSM_LCD_ESD_REBOOT_ERROR_NO                    (20109)
/* < DTS2014080106240 renxigang 20140801 begin */
#define DSM_LCD_POWER_STATUS_ERROR_NO                    (20110)
/* DTS2014080106240 renxigang 20140801 end > */
/* < DTS2014062405194 songliangliang 20140624 end */
/* < DTS2014101301850 zhoujian 20141013 begin */
#define DSM_LCD_MDSS_UNDERRUN_ERROR_NO		(20111)
/* DTS2014101301850 zhoujian 20141013 end > */
/* < DTS2014111001776 zhaoyuxia 20141114 begin */
#define DSM_LCD_MDSS_IOMMU_ERROR_NO			(20112)
#define DSM_LCD_MDSS_PIPE_ERROR_NO			(20113)
#define DSM_LCD_MDSS_PINGPONG_ERROR_NO			(20114)
#define DSM_LCD_MDSS_BIAS_ERROR_NO			(20115)
#define DSM_LCD_MDSS_ROTATOR_ERROR_NO			(20116)
#define DSM_LCD_MDSS_FENCE_ERROR_NO			(20117)
#define DSM_LCD_MDSS_CMD_STOP_ERROR_NO			(20118)
#define DSM_LCD_MDSS_VIDEO_DISPLAY_ERROR_NO			(20119)
#define DSM_LCD_MDSS_MDP_CLK_ERROR_NO			(20120)
#define DSM_LCD_MDSS_MDP_BUSY_ERROR_NO			(20121)
/* DTS2014111001776 zhaoyuxia 20141114 end > */

#define DSM_ERR_SDIO_RW_ERROR				(20300)
#define DSM_ERR_SENSORHUB_IPC_TIMEOUT		(20400)
/* DTS2014051603610 zhaoyuxia 20140516 end > */

/* < DTS2014072201656 yangzhonghua 20140722 begin */
/* <DTS2014112605676 yangzhonghua 20141126 begin */
#define SENSOR_VAL_SAME_MAX_TIMES 20

#define CLIENT_NAME_GS_KX			"dsm_gs_kx023"
#define CLIENT_NAME_GS_LIS			"dsm_gs_lis3dh"
#define CLIENT_NAME_LPS_APDS		"dsm_lps_apds"
#define CLIENT_NAME_MS_AKM			"dsm_ms_akm09911"
#define CLIENT_SENSOR_SERVICE		"dsm_sensor_service"
#define CLIENT_H_SENSOR				"dsm_Hall_sensor"
/* < DTS2014121803938 yangzhonghua 20141218 begin */
#define CLIENT_DSM_KEY					"dsm_key"

#define DSM_SENSOR_BASE_ERR				21100
#define DSM_GS_BASE_ERR					(DSM_SENSOR_BASE_ERR)
#define DSM_LPS_BASE_ERR				(DSM_GS_BASE_ERR + 8)
#define DSM_MS_BASE_ERR					(DSM_LPS_BASE_ERR + 8)
#define DSM_HS_BASE_ERR					(DSM_MS_BASE_ERR + 8)
#define DSM_SENSOR_SRV_BASE_ERR			(DSM_HS_BASE_ERR + 8)
#define DSM_KEY_BASE_ERR				(DSM_SENSOR_SRV_BASE_ERR + 8)

enum DSM_SENSOR_ERR_TYPES {
	/* for monitor gsensor  21100 21101 21102 21103 ....*/
	DSM_GS_I2C_ERROR = DSM_GS_BASE_ERR,
	DSM_GS_DATA_ERROR,
	DSM_GS_XYZ_0_ERROR,
	DSM_GS_DATA_TIMES_NOTCHANGE_ERROR,/* gsensor data stay 20 times don't change*/
	/* for monitor aps sensor 21108 21109 21110 21111 .... */
	DSM_LPS_I2C_ERROR = DSM_LPS_BASE_ERR,
	DSM_LPS_WRONG_IRQ_ERROR,
	DSM_LPS_THRESHOLD_ERROR,
	DSM_LPS_ENABLED_IRQ_ERROR,
	/* for monitor compass 21116 21117 21118 21119 .... */
	DSM_MS_I2C_ERROR = DSM_MS_BASE_ERR,
	DSM_MS_DATA_ERROR,
	DSM_MS_DATA_TIMES_NOTCHANGE_ERROR,
	DSM_MS_SELF_TEST_ERROR,
	/* for hall sensor 21124 .... */
	DSM_HS_IRQ_TIMES_ERR = DSM_HS_BASE_ERR,
	DSM_HS_IRQ_INTERVAL_ERR,
	/* the sensor service progress exit unexpected. 21132 21133 .... */
	DSM_SENSOR_SERVICE_EXIT = DSM_SENSOR_SRV_BASE_ERR,
	DSM_SENSOR_SERVICE_NODATA,
	/* for monitor the vol_up/down ,  power key 21140 21141 21142 ....*/
	DSM_VOL_KEY_PRESS_TIMES_ERR = DSM_KEY_BASE_ERR,
	DSM_VOL_KEY_PRESS_INTERVAL_ERR,
	DSM_POWER_KEY_PRESS_TIMES_ERR,
	DSM_POWER_KEY_PRESS_INTERVAL_ERR,
};

enum DSM_KEYS_TYPE{
	DSM_VOL_KEY = 0,
	DSM_VOL_UP_KEY,
	DSM_VOL_DOWN_KEY,
	DSM_POW_KEY,
	DSM_HALL_IRQ,
};
#define DSM_POWER_KEY_VAL 		116
#define DSM_VOL_DOWN_KEY_VAL 	114

/* DTS2014121803938 yangzhonghua 20141218 end > */

#define DSM_SENSOR_BUF_MAX 				2048
#define DSM_SENSOR_BUF_COM				2048
/* DTS2014112605676 yangzhonghua 20141126 end> */

/* DTS2014072201656 yangzhonghua 20140722 end > */
/* < DTS2014071005378 yubin 20140711 begin */
#define DSM_AUDIO_ERROR_NUM                     (20200)
#define DSM_AUDIO_HANDSET_DECT_FAIL_ERROR_NO    (DSM_AUDIO_ERROR_NUM)
#define DSM_AUDIO_ADSP_SETUP_FAIL_ERROR_NO      (DSM_AUDIO_ERROR_NUM + 1)
#define DSM_AUDIO_CARD_LOAD_FAIL_ERROR_NO       (DSM_AUDIO_ERROR_NUM + 2)
/* < DTS2014120102447 wangzefan/wx224779 20141202 begin */
#define DSM_AUDIO_HAL_FAIL_ERROR_NO             (DSM_AUDIO_ERROR_NUM + 3)
#define DSM_AUDIO_MODEM_CRASH_ERROR_NO          (DSM_AUDIO_ERROR_NUM + 4)
#define DSM_AUDIO_HANDSET_PLUG_PRESS_ERROR      (DSM_AUDIO_ERROR_NUM + 5)
#define DSM_AUDIO_HANDSET_PRESS_RELEASE_ERROR   (DSM_AUDIO_ERROR_NUM + 6)
#define DSM_AUDIO_HANDSET_PLUG_RELEASE_ERROR    (DSM_AUDIO_ERROR_NUM + 7)
#define DSM_AUDIO_MMI_TEST_TIME_TOO_SHORT       (DSM_AUDIO_ERROR_NUM + 8)
/* < DTS2014122601745 wangzefan/wx224779 20141224 begin */
#define DSM_AUDIO_CFG_CODEC_CLK_FAIL_ERROR      (DSM_AUDIO_ERROR_NUM + 9)
#define DSM_AUDIO_MODEM_CRASH_CODEC_CALLBACK    (DSM_AUDIO_ERROR_NUM + 10)
#define DSM_AUDIO_CODEC_RESUME_FAIL_ERROR       (DSM_AUDIO_ERROR_NUM + 11)
/* DTS2014122601745 wangzefan/wx224779 20141224 end > */
/* DTS2014120102447 wangzefan/wx224779 20141202 end > */
/* DTS2014071005378 yubin 20140711 end > */


/* < DTS2014111105636 Houzhipeng hwx231787 20141111 begin */
/* < DTS2014081209165 Zhangbo 00166742 20140812 begin */
#define DSM_CAMERA_ERROR                            (20500)
#define DSM_CAMERA_SOF_ERR                          (DSM_CAMERA_ERROR + 1)
#define DSM_CAMERA_I2C_ERR                          (DSM_CAMERA_ERROR + 2)
#define DSM_CAMERA_CHIP_ID_NOT_MATCH                (DSM_CAMERA_ERROR + 3)
#define DSM_CAMERA_OTP_ERR                          (DSM_CAMERA_ERROR + 4)
//delete DSM_CAMERA_CPP_BUFF_ERR
/* DTS2014081209165 Zhangbo 00166742 20140812 end > */
#define DSM_CAMERA_SENSOR_NO_FRAME                  (DSM_CAMERA_ERROR + 6)
#define DSM_CAMERA_LED_FLASH_CIRCUIT_ERR            (DSM_CAMERA_ERROR + 7)
#define DSM_CAMERA_LED_FLASH_OVER_TEMPERATURE       (DSM_CAMERA_ERROR + 8)
#define DSM_CAMERA_LED_FLASH_VOUT_ERROR             (DSM_CAMERA_ERROR + 9)
#define DSM_CAMERA_ISP_OVERFLOW                     (DSM_CAMERA_ERROR + 10)
#define DSM_CAMERA_ISP_AXI_STREAM_FAIL              (DSM_CAMERA_ERROR + 11)
#define DSM_CAMERA_VIDC_OVERLOADED                  (DSM_CAMERA_ERROR + 12)
#define DSM_CAMERA_VIDC_SESSION_ERROR               (DSM_CAMERA_ERROR + 13)
#define DSM_CAMERA_VIDC_LOAD_FW_FAIL                (DSM_CAMERA_ERROR + 14)
#define DSM_CAMERA_POST_EVENT_TIMEOUT               (DSM_CAMERA_ERROR + 15)
#define DSM_CAMERA_ACTUATOR_INIT_FAIL               (DSM_CAMERA_ERROR + 16)
#define DSM_CAMERA_ACTUATOR_SET_INFO_ERR            (DSM_CAMERA_ERROR + 17)
#define DSM_CAMERA_ACTUATOR_MOVE_FAIL               (DSM_CAMERA_ERROR + 18)
/* DTS2014111105636 Houzhipeng hwx231787 20141111 end > */
/* < DTS2014111904638 sihongfang 20141120 begin */
#define DSM_BLUETOOTH_DM_ERROR                                 (30200)
#define DSM_BLUETOOTH_DM_OPEN_ERROR                       (DSM_BLUETOOTH_DM_ERROR)
#define DSM_BLUETOOTH_DM_GET_ADDR_ERROR               (DSM_BLUETOOTH_DM_ERROR+1)
#define DSM_BLUETOOTH_A2DP_ERROR                              (30210)
#define DSM_BLUETOOTH_A2DP_CONNECT_ERROR              (DSM_BLUETOOTH_A2DP_ERROR)
#define DSM_BLUETOOTH_A2DP_AUDIO_ERROR                  (DSM_BLUETOOTH_A2DP_ERROR+1)
#define DSM_BLUETOOTH_HFP_ERROR                                (30220)
#define DSM_BLUETOOTH_HFP_CONNECT_ERROR                (DSM_BLUETOOTH_HFP_ERROR)
#define DSM_BLUETOOTH_HFP_SCO_CONNECT_ERROR        (DSM_BLUETOOTH_HFP_ERROR+1)
#define DSM_BLUETOOTH_BLE_ERROR                                 (30230)
#define DSM_BLUETOOTH_BLE_CONNECT_ERROR                 (DSM_BLUETOOTH_BLE_ERROR)
/* DTS2014111904638 sihongfang 20141120 end > */

/* < DTS2014112702834 chenjikun 20141127 begin */
#define DSM_WIFI_ERR                                (30100)
#define DSM_WIFI_DRIVER_LOAD_ERR                    (DSM_WIFI_ERR + 0)
#define DSM_WIFI_SUPPLICANT_START_ERR               (DSM_WIFI_ERR + 1)
#define DSM_WIFI_CONNECT_SUPPLICANT_ERR             (DSM_WIFI_ERR + 2)
#define DSM_WIFI_MAC_ERR                            (DSM_WIFI_ERR + 3)
#define DSM_WIFI_SCAN_FAIL_ERR                      (DSM_WIFI_ERR + 4)
#define DSM_WIFI_FAIL_LOADFIRMWARE_ERR              (DSM_WIFI_ERR + 5)
#define DSM_WIFI_ABNORMAL_DISCONNECT_ERR            (DSM_WIFI_ERR + 6)
#define DSM_WIFI_CANNOT_CONNECT_AP_ERR              (DSM_WIFI_ERR + 7)
#define DSM_WIFI_ROOT_NOT_RIGHT_ERR                 (DSM_WIFI_ERR + 8)
/* DTS2014112702834 chenjikun 20141127 end > */
/* < DTS2014112200393 yanhong/00202429 20141122 begin */
#define CLIENT_NAME_NFC	"dsm_nfc"
#define DSM_NFC_ERROR_NO                                               (30300)
#define DSM_NFC_I2C_WRITE_ERROR_NO                                     (DSM_NFC_ERROR_NO)
#define DSM_NFC_I2C_READ_ERROR_NO                                      (DSM_NFC_ERROR_NO + 1)
#define DSM_NFC_CLK_ENABLE_ERROR_NO                                    (DSM_NFC_ERROR_NO + 2)
#define DSM_NFC_I2C_WRITE_EOPNOTSUPP_ERROR_NO                          (DSM_NFC_ERROR_NO + 3)
#define DSM_NFC_I2C_READ_EOPNOTSUPP_ERROR_NO                           (DSM_NFC_ERROR_NO + 4)
#define DSM_NFC_I2C_WRITE_EREMOTEIO_ERROR_NO                           (DSM_NFC_ERROR_NO + 5)
#define DSM_NFC_I2C_READ_EREMOTEIO_ERROR_NO                            (DSM_NFC_ERROR_NO + 6)
#define DSM_NFC_RD_I2C_WRITE_ERROR_NO                                  (DSM_NFC_ERROR_NO + 7)
#define DSM_NFC_RD_I2C_READ_ERROR_NO                                   (DSM_NFC_ERROR_NO + 8)
#define DSM_NFC_RD_I2C_WRITE_EOPNOTSUPP_ERROR_NO                       (DSM_NFC_ERROR_NO + 9)
#define DSM_NFC_RD_I2C_READ_EOPNOTSUPP_ERROR_NO                        (DSM_NFC_ERROR_NO + 10)
#define DSM_NFC_RD_I2C_WRITE_EREMOTEIO_ERROR_NO                        (DSM_NFC_ERROR_NO + 11)
#define DSM_NFC_RD_I2C_READ_EREMOTEIO_ERROR_NO                         (DSM_NFC_ERROR_NO + 12)
#define DSM_NFC_CHECK_I2C_WRITE_ERROR_NO                               (DSM_NFC_ERROR_NO + 13)
#define DSM_NFC_CHECK_I2C_WRITE_EOPNOTSUPP_ERROR_NO                    (DSM_NFC_ERROR_NO + 14)
#define DSM_NFC_CHECK_I2C_WRITE_EREMOTEIO_ERROR_NO                     (DSM_NFC_ERROR_NO + 15)
#define DSM_NFC_FW_DOWNLOAD_ERROR_NO                                    (DSM_NFC_ERROR_NO + 16)
#define DSM_NFC_HAL_OPEN_FAILED_ERROR_NO                                (DSM_NFC_ERROR_NO + 17)
#define DSM_NFC_POST_INIT_FAILED_ERROR_NO                               (DSM_NFC_ERROR_NO + 18)
#define DSM_NFC_NFCC_TRANSPORT_ERROR_NO                                 (DSM_NFC_ERROR_NO + 19)
#define DSM_NFC_NFCC_CMD_TIMEOUT_ERROR_NO                               (DSM_NFC_ERROR_NO + 20)
/* DTS2014112200393 yanhong/00202429 20141122 end > */
struct dsm_client_ops{
	int (*poll_state) (void);
	int (*dump_func) (int type, void *buff, int size);
};

struct dsm_dev{
	const char *name;
	struct dsm_client_ops *fops;
	size_t buff_size;
};

/* one client */
struct dsm_client{
	/* < DTS2014072201656 yangzhonghua 20140722 begin */
	void *driver_data;
	/* DTS2014072201656 yangzhonghua 20140722 end > */
	char client_name[CLIENT_NAME_LEN];
	int client_id;
	int error_no;
	unsigned long buff_flag;
	struct dsm_client_ops *cops;
	wait_queue_head_t waitq;
	size_t read_size;
	size_t used_size;
	size_t buff_size;
	u8 dump_buff[];
};

/* <DTS2014112605676 yangzhonghua 20141126 begin */
/*
* for userspace client, such as sensor service, please refer to it.
*/
struct dsm_extern_client{
	char client_name[CLIENT_NAME_LEN];
	int buf_size;
};
/* DTS2014112605676 yangzhonghua 20141126 end> */

#ifdef CONFIG_HUAWEI_DSM
struct dsm_client *dsm_register_client (struct dsm_dev *dev);
/* < DTS2014072201656 yangzhonghua 20140722 begin */
void dsm_unregister_client (struct dsm_client *dsm_client,struct dsm_dev *dev);
/* DTS2014072201656 yangzhonghua 20140722 end > */

int dsm_client_ocuppy(struct dsm_client *client);
int dsm_client_record(struct dsm_client *client, const char *fmt, ...);
int dsm_client_copy(struct dsm_client *client, void *src, int sz);
void dsm_client_notify(struct dsm_client *client, int error_no);
/* < DTS2014121803938 yangzhonghua 20141218 begin */
extern void dsm_key_pressed(int type);
/* DTS2014121803938 yangzhonghua 20141218 end > */
#else
static inline struct dsm_client *dsm_register_client (struct dsm_dev *dev)
{
	return NULL;
}
static inline int dsm_client_ocuppy(struct dsm_client *client)
{
	return 1;
}
static inline int dsm_client_record(struct dsm_client *client, const char *fmt, ...)
{
	return 0;
}
static inline int dsm_client_copy(struct dsm_client *client, void *src, int sz)
{
	return 0;
}
static inline void dsm_client_notify(struct dsm_client *client, int error_no)
{
	return;
}
#endif

#endif

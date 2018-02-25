/*Begin <DTS2012120510674>  add by nielimin/00164272 for normalizing internal interface of AT command  2012/12/5*/
#ifndef __EQUIP_H__
#define __EQUIP_H__

typedef void(*PFunc)(void*);
#define  EQUIP_STR_LEN   (1024)
#define  DEV_MAX_NUM     (50)


#define EQUIP_IOC_MAGIC 			'E'
#define IOCTL_EQUIP_SET_INDEX 		_IO(EQUIP_IOC_MAGIC, 1)
#define IOCTL_EQUIP_GET_INDEX 		_IO(EQUIP_IOC_MAGIC, 2)

/* Begin <DTS2013091700593 > modified by n00164272 for realize the base fucntion of sensors  2013/9/30 */
/*BEGIN:DTS2013101708474  add by nielimin 00164272 and huyouhua 00136760 at 2013-10-17 for AT Selftest*/
enum EQUIP_DEVICE
{
    GYRO_SENSOR,
	SIMDETECT,
	SAR,
	HADRWARE_VERSION,
	LCD,
	BATT_VOLTAGE, /* ��ص�ѹ */
    BATT_TEMP, /* ����¶� */
    BATT_CAPACITY, /* ��ص��� */
    BATT_CURRENT, /* ������ص���������0��磬С��0�ŵ� */
    BATT_CHARGE_ENABLE, /* ���ʹ�ܽӿڣ�1ʹ�ܣ�0��ֹ */
    BATT_CHARGE_CURRENT, /* �������������ã���ѯ */
	COULOMETER,			/*���ؼ�*/
    HDMI,				/*HDMI*/
    FLASH,				/*�����*/
	GRAVITY_SENSOR,	          /*������Ӧ*/
	COMPASS_SENSOR,	          /*ָ����*/
	LIGHT_SENSOR,  /*LIGHTSENSOR*/
	HALL_SENSOR,  /*HALL SENSOR*/
	TP_TOUCHINFO,
	TP_TEST,
/* <DTS2014110409521 caiwei 20141104 begin */
	HOT_TEMP_TEST,
/* DTS2014110409521 caiwei 20141104 end> */
};
/*END:DTS2013101708474 s add by nielimin 00164272 and huyouhua 00136760 at 2013-10-17 for AT Selftest*/
/* End <DTS2013091700593 > modified by n00164272 for realize the base fucntion of sensors  2013/9/30 */

struct EQUIP_PARAM
{
	char str_in[EQUIP_STR_LEN];
	char str_out[EQUIP_STR_LEN];
};

enum EQUIP_OPS
{
	EP_READ,
	EP_WRITE,
};

struct EQUIP_FUNC_ARRARY
{
	PFunc pfunc[DEV_MAX_NUM][2];
	enum EQUIP_DEVICE equip_index;
	enum EQUIP_OPS    equip_ops;
};

extern struct EQUIP_FUNC_ARRARY equip_func;
extern void   register_equip_func(enum EQUIP_DEVICE equip_dev, enum EQUIP_OPS ops, PFunc pfunc);

#endif
/*End <DTS2012120510674>  add by nielimin/00164272 for normalizing internal interface of AT command  2012/12/5*/


/* < DTS2014040104155 xufeng 20140401 begin */
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/list.h>
#include <linux/export.h>
#include <misc/app_info.h>
#include <linux/slab.h>
/* < DTS2014051702218 zhaiqi 20140530 begin */
#include <soc/qcom/smsm.h>

#define SAMSUNG_ID   1
#define ELPIDA_ID     3
#define HYNIX_ID     6
/* DTS2014051702218 zhaiqi 20140530 end > */

struct info_node
{
    char name[APP_INFO_NAME_LENTH];
    char value[APP_INFO_VALUE_LENTH];
    struct list_head entry;
};

static LIST_HEAD(app_info_list);
static DEFINE_SPINLOCK(app_info_list_lock);

int app_info_set(const char * name, const char * value)
{	
	/* <DTS2014121501441 x00267953 20141215 begin */
	struct info_node *node = NULL;
	/* <DTS2014121501441 x00267953 20141215 end */
    struct info_node *new_node = NULL;
    int name_lenth = 0;
    int value_lenth = 0;

    if(WARN_ON(!name || !value))
        return -1;

    name_lenth = strlen(name);
    value_lenth = strlen(value);

	/* <DTS2014121501441 x00267953 20141215 begin */
	if (!strncmp(name, "touch_panel", name_lenth)) {
		list_for_each_entry(node,&app_info_list,entry) {
			if (!strncmp(node->name, name, name_lenth)) {
				memcpy(node->value, value, \
					((value_lenth > (APP_INFO_VALUE_LENTH-1))?(APP_INFO_VALUE_LENTH-1):value_lenth));
				return 0;
			}
		}
	}
	/* <DTS2014121501441 x00267953 20141215 end */
    
	new_node = kzalloc(sizeof(*new_node), GFP_KERNEL);
    if(new_node == NULL)
    {
        return -1;
    }

    memcpy(new_node->name,name,((name_lenth > (APP_INFO_NAME_LENTH-1))?(APP_INFO_NAME_LENTH-1):name_lenth));
    memcpy(new_node->value,value,((value_lenth > (APP_INFO_VALUE_LENTH-1))?(APP_INFO_VALUE_LENTH-1):value_lenth));

    spin_lock(&app_info_list_lock);
    list_add_tail(&new_node->entry,&app_info_list);
    spin_unlock(&app_info_list_lock);

    return 0;
}

EXPORT_SYMBOL(app_info_set);


static int app_info_proc_show(struct seq_file *m, void *v)
{
    struct info_node *node;

    list_for_each_entry(node,&app_info_list,entry)
    {
        seq_printf(m,"%-32s:%32s\n",node->name,node->value);
    }
    return 0;
}

static int app_info_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, app_info_proc_show, NULL);
}

static const struct file_operations app_info_proc_fops =
{
    .open		= app_info_proc_open,
    .read		= seq_read,
    .llseek		= seq_lseek,
    .release	= single_release,
};

/* < DTS2014051702218 zhaiqi 20140530 begin */
/*
    Function to read the SMEM to get the lpDDR name
*/
void export_ddr_name(unsigned int ddr_vendor_id)
{
    char * ddr_info = NULL;
    char *SAMSUNG_DDR = "SAMSUNG";
    char *ELPIDA_DDR = "ELPIDA";
    char *HYNIX_DDR  = "HYNIX";

     switch (ddr_vendor_id)
     {
        case SAMSUNG_ID:
        {
            ddr_info = SAMSUNG_DDR;
            break;
        }
        case ELPIDA_ID:
        {
            ddr_info = ELPIDA_DDR;
            break;
        }
        case HYNIX_ID:
        {
            ddr_info = HYNIX_DDR;
            break;
        }
        default:
        {
            ddr_info = "UNKNOWN";
            break;
        }
     }

    /* Set the vendor name in app_info */
    /*<DTS2014062707115 roopesh 20140627 begin*/
    if (app_info_set("ddr_vendor", ddr_info))
        pr_err("Error setting DDR vendor name\n");
    /*DTS2014062707115 roopesh 20140627 end>*/

    /* Print the DDR Name in the kmsg log */
    pr_err("DDR VENDOR NAME is : %s", ddr_info);

    return;
}


void app_info_print_smem(void)
{
    unsigned int ddr_vendor_id = 0;
    /* < DTS2013112708848 zengfengming 20131202 begin */
    /* read share memory and get DDR ID */
    smem_exten_huawei_paramater *smem = NULL;

    smem = smem_alloc(SMEM_ID_VENDOR1, sizeof(smem_exten_huawei_paramater),
						  0,
						  SMEM_ANY_HOST_FLAG);
    if(NULL == smem)
    {
        /* Set the vendor name in app_info */
        /*<DTS2014062707115 roopesh 20140627 begin*/
        if (app_info_set("ddr_vendor", "UNKNOWN"))
            pr_err("Error setting DDR vendor name to UNKNOWN\n");
        /*DTS2014062707115 roopesh 20140627 end>*/

        pr_err("%s: SMEM Error, READING DDR VENDOR NAME", __func__);
        return;
    }

    ddr_vendor_id = smem->lpddrID;
    ddr_vendor_id &= 0xff;
    /* DTS2013112708848 zengfengming 20131202 end > */

    export_ddr_name(ddr_vendor_id);

    return;
}
/* DTS2014051702218 zhaiqi 20140530 end > */
static int __init proc_app_info_init(void)
{
    proc_create("app_info", 0, NULL, &app_info_proc_fops);

/* < DTS2014051702218 zhaiqi 20140530 begin */
    app_info_print_smem();
/* DTS2014051702218 zhaiqi 20140530 end > */

    return 0;
}

module_init(proc_app_info_init);
/* DTS2014040104155 xufeng 20140401 end > */

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/earlysuspend.h>
#include <linux/slab.h>
#include <asm/unistd.h>
#include <linux/kthread.h>

#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>


#include <asm/atomic.h>
#include <asm/uaccess.h>

#include <mach/mt_reg_base.h>
#include <mach/mt_boot.h>
#include <mtk_kpd.h>            
#include <mach/irqs.h>
#include <mach/eint.h>
#include <mach/mt_gpio.h>

#ifdef MT6577
#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>
#endif

#include <linux/aee.h>
static int qt_flag = 0;

 
void qt2160_handler();

static DECLARE_WAIT_QUEUE_HEAD(waiter);
struct task_struct *qt2160_thread = NULL;
static struct qt2160_data *data;
static const struct i2c_device_id qt2160_i2c_id[] = {{"qt2160",0},{}};
static struct i2c_board_info __initdata i2c_qt2160={ I2C_BOARD_INFO("qt2160", (0x0d))};
static void qt2160_late_resume(struct early_suspend *h);
static void qt2160_early_suspend(struct early_suspend *h);



struct qt2160_data {
        struct i2c_client *client;
        struct input_dev *input_dev;
	struct early_suspend    early_drv;
        atomic_t                early_suspend;
};


static int write_reg(u8 addr, u8 cmd)
{
        char buf[2];
        int ret = -1;

        buf[0] = addr;
        buf[1] = cmd;
	printk("qt2160 write reg------------------------------------\n");
        ret = i2c_master_send(data->client, buf, 2);
        if (ret < 0){
                printk("qt2160: write reg failed! \n");
                return -1;
        }
        return 0;
}

static kal_uint32 read_reg(u8 addr_read)
{
        int ret;
	char buf[1];
	buf[0] = addr_read;
	printk("qt2160 read reg-----------------------------------\n");
        i2c_master_send(data->client, buf, 1);
	udelay(31);
	char buf_rec[1];
        ret = i2c_master_recv(data->client, buf_rec, 1);
        if (ret < 0)
                printk("qt2160: i2c read error\n");
		
        return buf_rec[0];
}

kal_uint32 qt2160_read_interface (kal_uint8 RegNum)
{
	kal_uint32 i = 0;
	i = read_reg(RegNum);
	return i;
}

kal_uint32 qt2160_config_interface (kal_uint8 RegNum, kal_uint8 val)
{
	write_reg(RegNum,val);
}


void qt2160_handler()
{


	printk("qt2160 handler---------------------------------------------\n");

	mt65xx_eint_mask(4);

	kal_uint32 reg1 = 0;
	int i = 0;
	i = mt_get_gpio_in(GPIO72);
	if(!i)
	{

	qt2160_read_interface(0x2);
	reg1 = qt2160_read_interface(0x3);
	qt2160_read_interface(0x4);
	qt2160_read_interface(0x5);
	qt2160_read_interface(0x6);

	if(reg1&0x1)
	{
		input_report_key(data->input_dev, KEY_BACK, 1);
		input_sync(data->input_dev);
		printk("pressed back\n");
	}
	else if(reg1&0x4)
	{
		input_report_key(data->input_dev, KEY_MENU, 1);
		input_sync(data->input_dev);
		printk("pressed menu\n");
	}
	else if(reg1&0x2)
	{
		input_report_key(data->input_dev, KEY_HOMEPAGE, 1);
		input_sync(data->input_dev);
		printk("pressed homepage\n");
	}
	else if(reg1 == 0)
	{
		input_report_key(data->input_dev, KEY_MENU, 0);
		input_report_key(data->input_dev, KEY_HOMEPAGE, 0);
                input_report_key(data->input_dev, KEY_BACK, 0);
                input_sync(data->input_dev);
		printk("release\n");
	}

	}

	mt65xx_eint_unmask(4);

}


static void qt2160_thread_handler(void)
{
        do
        {
                mt65xx_eint_unmask(4);
                set_current_state(TASK_INTERRUPTIBLE);
                wait_event_interruptible(waiter,qt_flag!=0);

                qt_flag = 0;

                set_current_state(TASK_RUNNING);
                mt65xx_eint_mask(4);

		qt2160_handler();
        }while(!kthread_should_stop());

        return 0;

}

void qt2160_eint_handler(void)
{
         qt_flag = 1;
         wake_up_interruptible(&waiter);
}
void qt2160_init(void)
{

qt2160_config_interface(0xc,0x3);
qt2160_config_interface(0xd,0x10);
qt2160_config_interface(0xe,0x32);
qt2160_config_interface(0xf,0xa);
qt2160_config_interface(0x10,0x5);
qt2160_config_interface(0x11,0x2);
qt2160_config_interface(0x12,0x96);

qt2160_config_interface(0x13,0x14);
qt2160_config_interface(0x14,0x0);
qt2160_config_interface(0x15,0x0);
qt2160_config_interface(0x16,0x1);
qt2160_config_interface(0x17,0x1);
qt2160_config_interface(0x18,0x1);
qt2160_config_interface(0x19,0x0);
qt2160_config_interface(0x1a,0x0);
qt2160_config_interface(0x1b,0x0);
qt2160_config_interface(0x1c,0x0);
qt2160_config_interface(0x1d,0x0);
qt2160_config_interface(0x1e,0x0);
qt2160_config_interface(0x1f,0x0);
qt2160_config_interface(0x20,0x0);
qt2160_config_interface(0x21,0x0);
qt2160_config_interface(0x22,0x0);
qt2160_config_interface(0x23,0x0);
qt2160_config_interface(0x24,0x0);
qt2160_config_interface(0x25,0x0);
qt2160_config_interface(0x26,0x0b);
qt2160_config_interface(0x27,0x0a);
qt2160_config_interface(0x28,0x0b);
qt2160_config_interface(0x29,0xa);
qt2160_config_interface(0x2a,0xa);
qt2160_config_interface(0x2b,0xa);
qt2160_config_interface(0x2c,0xa);
qt2160_config_interface(0x2d,0xa);
qt2160_config_interface(0x2e,0xa);
qt2160_config_interface(0x2f,0xa);
qt2160_config_interface(0x30,0xa);
qt2160_config_interface(0x31,0xa);
qt2160_config_interface(0x32,0xa);
qt2160_config_interface(0x33,0xa);
qt2160_config_interface(0x34,0xa);
qt2160_config_interface(0x35,0xa);
qt2160_config_interface(0x36,0x16);
qt2160_config_interface(0x37,0x18);
qt2160_config_interface(0x38,0x16);
qt2160_config_interface(0x39,0x0);
qt2160_config_interface(0x3a,0x0);
qt2160_config_interface(0x3b,0x0);
qt2160_config_interface(0x3c,0x0);
qt2160_config_interface(0x3d,0x0);
qt2160_config_interface(0x3e,0x0);
qt2160_config_interface(0x3f,0x0);

qt2160_config_interface(0x40,0x0);
qt2160_config_interface(0x41,0x0);
qt2160_config_interface(0x42,0x0);
qt2160_config_interface(0x43,0x0);
qt2160_config_interface(0x44,0x0);
qt2160_config_interface(0x45,0x0);
qt2160_config_interface(0x46,0x0);
qt2160_config_interface(0x47,0x0);
qt2160_config_interface(0x48,0x0);

qt2160_config_interface(0x49,0x0);
qt2160_config_interface(0x4a,0x0);
qt2160_config_interface(0x4b,0x0);
qt2160_config_interface(0x4c,0x0);
qt2160_config_interface(0x4d,0x0);

qt2160_config_interface(0xa,0x1);


        qt2160_read_interface(0x2);
        qt2160_read_interface(0x3);
        qt2160_read_interface(0x4);
        qt2160_read_interface(0x5);
        qt2160_read_interface(0x6);

}


static int qt2160_driver_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int i;
	printk("qt2160_driver_probe\n");
	int retval = 1;

	hwPowerOn(MT65XX_POWER_LDO_VGP2,VOL_1800,"qt2160");

        data = kzalloc(sizeof(struct qt2160_data), GFP_KERNEL);
        if(!data)
	{
        	printk("qt2160: kzalloc qt2160_data fail!\n");
		return -1;
	}
	memset(data, 0, sizeof(*data));
	data->client = client;

        data->input_dev = input_allocate_device();
        if (!(data->input_dev))
                return -ENOMEM;
        set_bit(EV_KEY, data->input_dev->evbit);
	set_bit(KEY_BACK, data->input_dev->keybit);
	set_bit(KEY_HOMEPAGE, data->input_dev->keybit);
	set_bit(KEY_MENU, data->input_dev->keybit);
	i = input_register_device(data->input_dev);
        if (i) {
                printk("register input device failed (%d)\n", i);
                input_free_device(data->input_dev);
                return i;
        }
	qt2160_init();

	mt_set_gpio_mode(GPIO72, 1);
	mt_set_gpio_dir(GPIO72, GPIO_DIR_IN);

        mt_set_gpio_pull_select(GPIO72, GPIO_PULL_UP);
        mt_set_gpio_pull_enable(GPIO72, GPIO_PULL_ENABLE);


	mt65xx_eint_set_sens(4, 1);
	mt65xx_eint_set_polarity(4,0);
	mt65xx_eint_set_hw_debounce(4, 0);
	mt65xx_eint_registration(4, 0, 0, qt2160_eint_handler, 0); 
	mt65xx_eint_unmask(4);


        qt2160_thread = kthread_run(qt2160_thread_handler, 0, "qt2160");
        if (IS_ERR(qt2160_thread))
        {
                 retval = PTR_ERR(qt2160_thread);
		 printk("qt2160 kthread_run fail \n");
        }

	atomic_set(&(data->early_suspend), 0);
        data->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
        data->early_drv.suspend  = qt2160_early_suspend,
        data->early_drv.resume   = qt2160_late_resume,
        register_early_suspend(&data->early_drv);

	return 0;
}
/*
static int qt2160_driver_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
	strcpy(info->type, "qt2160");

        printk("[qt2160_driver_detect] \n");

	return 0;
}
*/
static void qt2160_late_resume(struct early_suspend *h)
{
	atomic_set(&(data->early_suspend), 0);
        qt2160_config_interface(0xc,0x1);
        qt2160_config_interface(0xa,0x1);
        printk("qt2160 resume------------------------------------------------\n");
}

static void qt2160_early_suspend(struct early_suspend *h)
{
	atomic_set(&(data->early_suspend), 1);
        qt2160_config_interface(0xc,0x0);
        printk("qt2160 suspend------------------------------------------------\n");
}


static struct i2c_driver touchkey_qt2160_driver = {
    .probe              = qt2160_driver_probe,
//    .detect		= qt2160_driver_detect,
    .id_table   	= qt2160_i2c_id,
    .driver     = {
        .name = "qt2160",
    },
};


static int __init qt2160_driver_init(void)
{

        i2c_register_board_info(3, &i2c_qt2160, 1);
	if(i2c_add_driver(&touchkey_qt2160_driver)!=0)
        {
		printk("failed to register qt2160 i2c driver\n");
        }
	else
	{
		printk("success to register qt2160 i2c driver\n");
	}
        return 0;	
}


static void __exit qt2160_driver_exit (void)
{
	i2c_del_driver(&touchkey_qt2160_driver);
}

module_init(qt2160_driver_init);
module_exit(qt2160_driver_exit);

MODULE_AUTHOR("zhang k");
MODULE_DESCRIPTION("Touchkey QT2160 Driver");
MODULE_LICENSE("GPL");


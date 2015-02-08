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


void backkey_handler(void);
void homekey_handler(void);
void menukey_handler(void);

static struct qt1040_data *data;
static DECLARE_TASKLET(homekey_tasklet, homekey_handler, 0);
static DECLARE_TASKLET(menukey_tasklet, menukey_handler, 0);
static DECLARE_TASKLET(backkey_tasklet, backkey_handler, 0);

struct qt1040_data {
        struct input_dev *input_dev;
};

void homekey_handler(void)
{
        int i;
        i = mt_get_gpio_in(GPIO72);
        printk("xxxxxxxxx value is %d xxxxxxxxx\n", i);
        if(!i){
        printk("pressed home!xxxxxxxxx\n");
                input_report_key(data->input_dev, KEY_HOMEPAGE, 1);
                input_sync(data->input_dev);

                mt_set_gpio_mode(GPIO72, 1);
                mt_set_gpio_dir(GPIO72, GPIO_DIR_IN);
                mt65xx_eint_set_sens(4, 0);
                mt65xx_eint_set_polarity(4, 1);
        }
        else if(i){
                printk("released home!xxxxxxx\n");
                input_report_key(data->input_dev, KEY_HOMEPAGE, 0);
                input_sync(data->input_dev);

                mt_set_gpio_mode(GPIO72, 1);
                mt_set_gpio_dir(GPIO72, GPIO_DIR_IN);
                mt65xx_eint_set_sens(4, 0);
                mt65xx_eint_set_polarity(4, 0);
        }
}

void menukey_handler(void)
{
        int i;
        i = mt_get_gpio_in(GPIO68);
        printk("xxxxxxxxx value is %d xxxxxxxxx\n", i);
        if(!i){
        printk("pressed menu!xxxxxxxxx\n");
                input_report_key(data->input_dev, KEY_MENU, 1);
                input_sync(data->input_dev);

                mt_set_gpio_mode(GPIO68, 2);
                mt_set_gpio_dir(GPIO68, GPIO_DIR_IN);
                mt65xx_eint_set_sens(11, 0);
                mt65xx_eint_set_polarity(11, 1);
        }
        else if(i){
                printk("released menu!xxxxxxx\n");
                input_report_key(data->input_dev, KEY_MENU, 0);
                input_sync(data->input_dev);

                mt_set_gpio_mode(GPIO68, 2);
                mt_set_gpio_dir(GPIO68, GPIO_DIR_IN);
                mt65xx_eint_set_sens(11, 0);
                mt65xx_eint_set_polarity(11, 0);
        }

}

void backkey_handler(void)
{
        int i;
        i = mt_get_gpio_in(GPIO14);
        printk("xxxxxxxxx value is %d xxxxxxxxx\n", i);
        if(!i){
        printk("pressed back!xxxxxxxxx\n");
                input_report_key(data->input_dev, KEY_BACK, 1);
                input_sync(data->input_dev);

                mt_set_gpio_mode(GPIO14, 2);
                mt_set_gpio_dir(GPIO14, GPIO_DIR_IN);
                mt65xx_eint_set_sens(12, 0);
                mt65xx_eint_set_polarity(12, 1);
        }
        else if(i){
                printk("released back!xxxxxxx\n");
                input_report_key(data->input_dev, KEY_BACK, 0);
                input_sync(data->input_dev);

                mt_set_gpio_mode(GPIO14, 2);
                mt_set_gpio_dir(GPIO14, GPIO_DIR_IN);
                mt65xx_eint_set_sens(12, 0);
                mt65xx_eint_set_polarity(12, 0);
        }
}

static void back_key_handler(void)
{
        tasklet_schedule(&backkey_tasklet);
}
static void home_key_handler(void)
{
        tasklet_schedule(&homekey_tasklet);

}
static void menu_key_handler(void)
{
        tasklet_schedule(&menukey_tasklet);
}


static int __init qt1040_driver_init(void)
{
	int i;
        data = kzalloc(sizeof(struct qt1040_data), GFP_KERNEL);
        if(!data)
        {
                printk("qt1040: kzalloc qt1040_data fail!\n");
                return -1;
        }
        memset(data, 0, sizeof(*data));

        data->input_dev = input_allocate_device();
        if (!(data->input_dev))
                return -ENOMEM;
        set_bit(EV_KEY, data->input_dev->evbit);
        set_bit(KEY_BACK, data->input_dev->keybit);
        set_bit(KEY_HOMEPAGE, data->input_dev->keybit);
        set_bit(KEY_MENU, data->input_dev->keybit);
        i = input_register_device(data->input_dev);
        if (i) {
                printk("qt1040:register input device failed (%d)\n", i);
                input_free_device(data->input_dev);
                return i;
        }


//        hwPowerOn(MT65XX_POWER_LDO_VGP2,VOL_1800,"qt1040");

        mt_set_gpio_mode(GPIO126, 0);
        mt_set_gpio_dir(GPIO126, GPIO_DIR_OUT);
        mt_set_gpio_out(GPIO126, GPIO_OUT_ONE);

//back key
        mt_set_gpio_mode(GPIO72, 1);
        mt_set_gpio_dir(GPIO72, GPIO_DIR_IN);

        mt65xx_eint_set_sens(4, 0);
        mt65xx_eint_set_hw_debounce(4, 0);
        mt65xx_eint_registration(4, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, 0, home_key_handler, 1);
        mt65xx_eint_unmask(4);


//home key
        mt_set_gpio_mode(GPIO68, 2);
        mt_set_gpio_dir(GPIO68, GPIO_DIR_IN);

        mt65xx_eint_set_sens(11, 0);
        mt65xx_eint_set_hw_debounce(11, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);
        mt65xx_eint_registration(11, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, 0, menu_key_handler, 1);
        mt65xx_eint_unmask(11);

//homepage key  
        mt_set_gpio_mode(GPIO14, 2);
        mt_set_gpio_dir(GPIO14, GPIO_DIR_IN);

        mt65xx_eint_set_sens(12, 0);
        mt65xx_eint_set_hw_debounce(12, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);
        mt65xx_eint_registration(12, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, 0, back_key_handler, 1);
        mt65xx_eint_unmask(12);
}

module_init(qt1040_driver_init);

MODULE_AUTHOR("zhang k");
MODULE_DESCRIPTION("Touchkey QT1040 Driver");
MODULE_LICENSE("GPL");


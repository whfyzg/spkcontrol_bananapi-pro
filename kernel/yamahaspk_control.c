/*
 *     io control for yamaha speaker demo board
 */

#include <linux/input.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/sys_config.h>
#include <mach/clock.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/fs.h>

#include <linux/input/mt.h>
#include <mach/gpio.h>
#include <linux/init-input.h>
#include <linux/slab.h>

#include <linux/ioctl.h>

#include <linux/time.h>
#include <linux/jiffies.h>
#include <linux/hrtimer.h>

#include <linux/cdev.h>
#include <linux/platform_device.h>

#define YAMAHA_MAJOR 378
//ioctl cmd

#define YAMAHA_SPK_BASE		   0xa1
//#define YAMAHA_SPK_RESET_LED       IO(YAMAHA_SPK_BASE, 0)
//#define YAMAHA_SPK_AUDIO_ROUTE     IOW(YAMAHA_SPK_BASE, 1, int)

#define YAMAHA_SPK_RESET_LED  _IO(YAMAHA_SPK_BASE, 0)
#define YAMAHA_SPK_AUDIO_ROUTE_1  _IO(YAMAHA_SPK_BASE, 10)
#define YAMAHA_SPK_AUDIO_ROUTE_2  _IO(YAMAHA_SPK_BASE, 11)
#define YAMAHA_SPK_AUDIO_ROUTE_3  _IO(YAMAHA_SPK_BASE, 12)
#define YAMAHA_SPK_STOP_LED  _IO(YAMAHA_SPK_BASE, 13)
#define YAMAHA_SPK_CONTINUE_LED _IO(YAMAHA_SPK_BASE, 14)
#define YAMAHA_SPK_START_TIMER  _IO(YAMAHA_SPK_BASE, 15)

#define LED_SECTION1 0
#define LED_SECTION2 1
#define LED_SECTION3 2

static struct class *yamaha_dev_class;

static int led_need_stop = 0;
static int led_breakpoint = 1;

static struct sysconfig {
	int used;
	u32 int_section1;
	u32 int_section2;
	u32 int_section3;
 	u32 control_section1;
	u32 control_section2;
	u32 control_section3;
	u32 int_vol_down;
	u32 int_vol_up;
	u32 int_play;
	u32 audio_switch1;
	u32 audio_switch2; 
} *sysconfig_demoboard;

struct timer_list seven_sec_timer;

static struct irq_config {
	u32 irq;
	enum gpio_eint_trigtype trig;
	u32 int_handler;
	char *name;
} irq_config_demoboard[6];

__kernel_time_t time_last_vol_irq = 0;

struct input_dev *yamaha_input_dev;

static int dump_sysconfig = 1;

static int get_sysconfig_para(void)
{
	int ret = -1, i;
	int value[12] = {0};
	script_item_u val;

	char *name[13] = {
		"yamahaspk_control",
		"yamahaspk_used",
		"int_section1",
		"int_section2",
		"int_section3",
		"control_section1",
		"control_section2",
		"control_section3",
		"int_vol_down",
		"int_vol_up",
		"int_play",
		"audio_switch1",
		"audio_switch2"
		};

	printk(KERN_INFO "yamahaspk<%s>:", __func__);
	
	for (i = 0; i < 12; i++){	
		val = get_para_value(name[0], name[i+1]);
		if (val.val == -1){
			ret = -1;
			goto failed_get_para;
		}
		if (i == 1){
			value[i] = val.val;
		}else{
			value[i] = val.gpio.gpio;
		}
	}

	sysconfig_demoboard->used = value[0];
	sysconfig_demoboard->int_section1 = value[1];
	sysconfig_demoboard->int_section2 = value[2];
	sysconfig_demoboard->int_section3 = value[3];
	sysconfig_demoboard->control_section1 = value[4];
	sysconfig_demoboard->control_section2 = value[5];
	sysconfig_demoboard->control_section3 = value[6];
	sysconfig_demoboard->int_vol_down = value[7];
	sysconfig_demoboard->int_vol_up   = value[8];
	sysconfig_demoboard->int_play     = value[9];
	sysconfig_demoboard->audio_switch1 = value[10];
	sysconfig_demoboard->audio_switch2 = value[11];

	irq_config_demoboard[0].irq   = sysconfig_demoboard->int_section1;
	irq_config_demoboard[0].name  = "int_section1";
	irq_config_demoboard[0].trig  = TRIG_EDGE_POSITIVE;
	irq_config_demoboard[1].irq   = sysconfig_demoboard->int_section2;
	irq_config_demoboard[1].name  = "int_section2";
	irq_config_demoboard[1].trig  = TRIG_EDGE_POSITIVE;
	irq_config_demoboard[2].irq   = sysconfig_demoboard->int_section3;
	irq_config_demoboard[2].name  = "int_section3";
	irq_config_demoboard[2].trig  = TRIG_EDGE_POSITIVE;

	irq_config_demoboard[3].irq   = sysconfig_demoboard->int_vol_down;
	irq_config_demoboard[3].name  = "int_vol_down";
	irq_config_demoboard[3].trig  = TRIG_EDGE_NEGATIVE;
	irq_config_demoboard[4].irq   = sysconfig_demoboard->int_vol_up;
	irq_config_demoboard[4].name  = "int_vol_up";
	irq_config_demoboard[4].trig  = TRIG_EDGE_NEGATIVE; 
	irq_config_demoboard[5].irq   = sysconfig_demoboard->int_play;
	irq_config_demoboard[5].name  = "int_play";
	irq_config_demoboard[5].trig  = TRIG_EDGE_NEGATIVE;		

	if (dump_sysconfig){
		printk("sysconfig_demoboard->used = %d\n"    
			"sysconfig_demoboard->int_section1 = %d\n"   
			"sysconfig_demoboard->int_section2 = %d\n"    
			"sysconfig_demoboard->int_section3 = %d\n"     
			"sysconfig_demoboard->control_section1 = %d\n"   
			"sysconfig_demoboard->control_section2 = %d\n"    
			"sysconfig_demoboard->control_section3 = %d\n"  
			"sysconfig_demoboard->int_vol_down = %d\n"
			"sysconfig_demoboard->int_vol_up   = %d\n"
			"sysconfig_demoboard->int_play     = %d\n"
			"sysconfig_demoboard->audio_switch1 = %d\n"
			"sysconfig_demoboard->audio_switch2 = %d\n",
			value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7], 
			value[8], value[9], value[10], value[11]);
	}

	ret = 0;
	return ret;

failed_get_para:
	printk(KERN_ERR "yamahaspk<%s>: err occurs when get sysconfig\n", __func__);
	return ret;
}

static int init_platform_gpio(void){
	int cnt = 0, i = 0;
	script_item_u  *list = NULL;
	cnt = script_get_pio_list("yamahaspk_control", &list);
	if (0 == cnt) {
		printk(KERN_ERR "yamahaspk<%s>: please config gpio list in sysconfig\n", __func__);
		return -1;
	}
	
	/*request gpios*/
	for (i = 0; i < cnt; i++){
		if (0 != gpio_request(list[i].gpio.gpio, NULL)){
			printk(KERN_ERR "yamahaspk<%s>: request gpio : %d failed\n", __func__, list[i].gpio.gpio);
	//		return -1;
		}
	}

	/*configure gpios*/
	if (0 != sw_gpio_setall_range(&list[0].gpio, cnt)){
		printk(KERN_ERR "yamahaspk<%s>: error occurs when configure gpios\n", __func__);
	//	return -1;
	}
	
	return 0;
}

static void free_platform_gpio(void)
{
	int gpio_cnt, i;
	script_item_u *list = NULL;

	gpio_cnt = script_get_pio_list("yamahaspk_control", &list);
	for(i = 0; i < gpio_cnt; i++){
		gpio_free(list[i].gpio.gpio);
	}

	return;
}

/*
 * route = 0: analog audio 1 on, analog audio 2 off
 * route = 1: analog audio 1 off, analog audio 2 on
 */
static void switch_audio_route(int route){
	u32 gpio_status;

	printk(KERN_INFO "yamahaspk<%s>: route = %d\n", __func__, route);

	gpio_status = sw_gpio_getcfg(sysconfig_demoboard->audio_switch1);
	if(gpio_status != 1){
		sw_gpio_setcfg(sysconfig_demoboard->audio_switch1, 1);
	}
	gpio_status = sw_gpio_getcfg(sysconfig_demoboard->audio_switch2);
	if(gpio_status != 1){
		sw_gpio_setcfg(sysconfig_demoboard->audio_switch2, 1);
	}

	if (route == 0){
		__gpio_set_value(sysconfig_demoboard->audio_switch1, 1);
		__gpio_set_value(sysconfig_demoboard->audio_switch2, 0);
	}else if(route == 1){
		__gpio_set_value(sysconfig_demoboard->audio_switch1, 0);
		__gpio_set_value(sysconfig_demoboard->audio_switch2, 1);
	}else if (route == 2){
		__gpio_set_value(sysconfig_demoboard->audio_switch1, 0);
		__gpio_set_value(sysconfig_demoboard->audio_switch2, 0);
	}
	return ;
	
}

/*
 * Control led On/Off
 * section: led section
 * on: 1--on, 0--off
 */
static void switch_led(int section, int on){
	u32 gpio_status;
	
	printk(KERN_INFO "yamahaspk<%s>: \n", __func__);

	switch(section){
	case LED_SECTION1:
		gpio_status = sw_gpio_getcfg(sysconfig_demoboard->control_section1);
		if (gpio_status != 1){
			sw_gpio_setcfg(sysconfig_demoboard->control_section1, 1);
		}

		__gpio_set_value(sysconfig_demoboard->control_section1, !!on);

		break;

	case LED_SECTION2:
		gpio_status = sw_gpio_getcfg(sysconfig_demoboard->control_section2);
		if (gpio_status != 1){
			sw_gpio_setcfg(sysconfig_demoboard->control_section2, 1);
		}

		__gpio_set_value(sysconfig_demoboard->control_section2, !!on);	

		break;
	case LED_SECTION3:
		gpio_status = sw_gpio_getcfg(sysconfig_demoboard->control_section3);
		if (gpio_status != 1){
			sw_gpio_setcfg(sysconfig_demoboard->control_section3, 1);
		}

		__gpio_set_value(sysconfig_demoboard->control_section3, !!on);
		break;
		default:
		printk(KERN_ERR "yamahaspk<%s>: unkown led section", __func__);
		break;
	}
	
	return;
	
}

static int yamahaspk_open(struct inode *inode, struct file *file){
	return 0;
}

static int yamahaspk_release(struct inode *inode, struct file *file) {
	return 0;
}

static void yamahaspk_continue_led(void);

static int yamahaspk_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long args){
	
	switch(cmd){
	case YAMAHA_SPK_RESET_LED:
		//turn off all led
		switch_led(LED_SECTION1, 0);
		switch_led(LED_SECTION2, 0);
		switch_led(LED_SECTION3, 0);
		led_need_stop = 0;
		mdelay(300);
		//turn on led section 1
		switch_led(LED_SECTION1, 1);
		break;
	case YAMAHA_SPK_STOP_LED:
		printk(KERN_INFO "YAMAHA_SPK_STOP_LED");
		led_need_stop = 1;
		break;
	case YAMAHA_SPK_CONTINUE_LED:
/*
		led_need_stop = 0;
		if (led_breakpoint == 1){
			switch_led(LED_SECTION1, 1);
		}else if (led_breakpoint == 2){
			switch_led(LED_SECTION1, 2);
		}else if (led_breakpoint == 3){
			switch_led(LED_SECTION1, 3);
		}
*/
		yamahaspk_continue_led();
		break;		
	case YAMAHA_SPK_AUDIO_ROUTE_1:
		switch_audio_route(0);
		break;
	case YAMAHA_SPK_AUDIO_ROUTE_2:
		switch_audio_route(1);
		break;
	case YAMAHA_SPK_AUDIO_ROUTE_3:
		switch_audio_route(2);
		break;
	case YAMAHA_SPK_START_TIMER:
		printk(KERN_INFO "YAMAHA_SPK_START_TIMER");
		mod_timer(&seven_sec_timer, jiffies + msecs_to_jiffies(7000));
		break;
	default:
		printk(KERN_ERR "yamahaspk<%s>: unkown cmd: %d\n", __func__, cmd);
		return -1;
		
	}

	return 0;
}

static const struct file_operations yamahasp_ops = {
	.owner          = THIS_MODULE,
	.open		= yamahaspk_open,
	.release	= yamahaspk_release,
	.unlocked_ioctl = yamahaspk_unlocked_ioctl,
};

static void yamahaspk_continue_led(void)
{
	printk(KERN_INFO "yamahaspk<%s>: led_breakpoint = %d\n", __func__, led_breakpoint);
	led_need_stop = 0;
        if (led_breakpoint == 1){ 
		switch_led(LED_SECTION1, 0); 
                switch_led(LED_SECTION2, 0); 
                switch_led(LED_SECTION3, 0); 
		mdelay(10);
        	switch_led(LED_SECTION1, 1); 
        }else if (led_breakpoint == 2){ 
                switch_led(LED_SECTION2, 1); 
        }else if (led_breakpoint == 3){ 
                switch_led(LED_SECTION3, 1); 
        }

	led_breakpoint = 1;

	return;   
	
}

static void yamahaspk_timer_func(void)
{
	printk(KERN_INFO "yamahaspk<%s>: \n", __func__);
        input_report_key(yamaha_input_dev, KEY_F7, 1); 
        input_sync(yamaha_input_dev);
        input_report_key(yamaha_input_dev, KEY_F7, 0); 
        input_sync(yamaha_input_dev);
}

//irq handler
static irqreturn_t yamahaspk_int_handler(void *data){
	struct irq_config *irq_config = (struct irq_config *)data;
	printk(KERN_INFO "yamahaspk<%s>: irq = %s\n", __func__, irq_config->name);

	if(irq_config->irq ==  sysconfig_demoboard->int_section1) {
                //turn off all led
//                switch_led(LED_SECTION1, 0); 
//                switch_led(LED_SECTION2, 0); 
//                switch_led(LED_SECTION3, 0);
		//turn on led section 1
//		switch_led(LED_SECTION1, 1);
		//report key F4
		input_report_key(yamaha_input_dev, KEY_F4, 1);
		input_sync(yamaha_input_dev);
		input_report_key(yamaha_input_dev, KEY_F4, 0);
		input_sync(yamaha_input_dev);
		return IRQ_HANDLED;
	}else if(irq_config->irq ==  sysconfig_demoboard->int_section2){
		//report key F5
		input_report_key(yamaha_input_dev, KEY_F5, 1);
		input_sync(yamaha_input_dev);
		input_report_key(yamaha_input_dev, KEY_F5, 0);
		input_sync(yamaha_input_dev);
                if (led_need_stop == 1){
			led_breakpoint = 2;
                        return IRQ_HANDLED;
                 }
		switch_led(LED_SECTION2, 1);
		return IRQ_HANDLED;
	}else if (irq_config->irq ==  sysconfig_demoboard->int_section3){
		//report key F6
		input_report_key(yamaha_input_dev, KEY_F6, 1);
		input_sync(yamaha_input_dev);
		input_report_key(yamaha_input_dev, KEY_F6, 0);
		input_sync(yamaha_input_dev);
                if (led_need_stop == 1){
			led_breakpoint = 3;
                        return IRQ_HANDLED;
                }
                //turn on led section 3
                switch_led(LED_SECTION3, 1);
		return IRQ_HANDLED;
	}else if (irq_config->irq ==  sysconfig_demoboard->int_vol_down){
		struct timespec now;
		memset(&now, 0, sizeof(struct timespec)); 
		now = current_kernel_time();
		__kernel_time_t irq_interval = now.tv_sec - time_last_vol_irq;
		if (irq_interval > 1){
			//report key F2
			input_report_key(yamaha_input_dev, KEY_F2, 1);
			input_sync(yamaha_input_dev);
			input_report_key(yamaha_input_dev, KEY_F2, 0);
			input_sync(yamaha_input_dev);
//				led_need_stop = 1;
//				mod_timer(&seven_sec_timer, jiffies + msecs_to_jiffies(7000));
//			 	add_timer(&seven_sec_timer);
		}else{
			input_report_key(yamaha_input_dev, KEY_VOLUMEDOWN, 1);
			input_sync(yamaha_input_dev);
			input_report_key(yamaha_input_dev, KEY_VOLUMEDOWN, 0);
			input_sync(yamaha_input_dev);
		}
		time_last_vol_irq = now.tv_sec;
		return IRQ_HANDLED;
	}else if (irq_config->irq ==  sysconfig_demoboard->int_vol_up){
		struct timespec now;
                memset(&now, 0, sizeof(struct timespec)); 
                now = current_kernel_time();
                __kernel_time_t irq_interval = now.tv_sec - time_last_vol_irq;
                if (irq_interval > 1){
			//report key F1
			input_report_key(yamaha_input_dev, KEY_F1, 1);
			input_sync(yamaha_input_dev);
			input_report_key(yamaha_input_dev, KEY_F1, 0);	
			input_sync(yamaha_input_dev);
//				led_need_stop = 1;
//				mod_timer(&seven_sec_timer, jiffies + msecs_to_jiffies(7000));
//				add_timer(&seven_sec_timer);
		}else{
                        input_report_key(yamaha_input_dev, KEY_VOLUMEUP, 1); 
                        input_sync(yamaha_input_dev);
                        input_report_key(yamaha_input_dev, KEY_VOLUMEUP, 0); 
                        input_sync(yamaha_input_dev);
                }   
                time_last_vol_irq = now.tv_sec;
		return IRQ_HANDLED;
	}else if (irq_config->irq ==  sysconfig_demoboard->int_play){
		//report key F3
		input_report_key(yamaha_input_dev, KEY_F3, 1);
		input_sync(yamaha_input_dev);
		input_report_key(yamaha_input_dev, KEY_F3, 0);
		input_sync(yamaha_input_dev);
	       	return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static int __init yamahaspk_init(void){
	printk(KERN_INFO "yamahaspk<%s>: \n");

	struct input_dev *input_device;
	struct device *dev;
	int rc, i;

	sysconfig_demoboard = kzalloc(sizeof(*sysconfig_demoboard), GFP_KERNEL);
	if(!sysconfig_demoboard){
		printk(KERN_ERR "yamahaspk<%s>: fial to alloc sysconfig_demoboard");
		return -1;
	}

	rc = get_sysconfig_para();
	if (rc){
		goto error_alloc_dev;
	}

	//regest input device 
	input_device = input_allocate_device();
	if (!input_device) {
		rc = -ENOMEM;
		goto error_alloc_dev;
	}

	input_device->name = "yamahaspk";
	input_device->phys = "yamahaspk/input0";
	input_device->id.bustype = BUS_HOST;
	input_device->id.vendor = 0x0101;
	input_device->id.product = 0x0001;
	input_device->id.version = 0x0100;

	input_device->evbit[0] = BIT_MASK(EV_KEY);
	set_bit(KEY_F1, input_device->keybit);
	set_bit(KEY_F2, input_device->keybit);
	set_bit(KEY_F3, input_device->keybit);
	set_bit(KEY_F4, input_device->keybit);
	set_bit(KEY_F5, input_device->keybit);
	set_bit(KEY_F6, input_device->keybit);
	set_bit(KEY_F7, input_device->keybit);
        set_bit(KEY_VOLUMEDOWN, input_device->keybit);
        set_bit(KEY_VOLUMEUP, input_device->keybit);
		
	
	rc = input_register_device(input_device);
	if (rc)
		goto error_unreg_device;

	yamaha_input_dev = input_device;
	
	init_platform_gpio();

	//request irqs
	for (i = 0; i < 6; i++){
		irq_config_demoboard[i].int_handler = sw_gpio_irq_request(irq_config_demoboard[i].irq, 
			irq_config_demoboard[i].trig, (peint_handle)yamahaspk_int_handler, irq_config_demoboard+i);	
		if(!irq_config_demoboard[i].int_handler){
			printk(KERN_ERR "yamahaspk<%s>: request irq %d fail\n", __func__, irq_config_demoboard[i].irq);
		}
	}

        //register char device
	rc = register_chrdev(YAMAHA_MAJOR, "yamahaspk", &yamahasp_ops);
	if (rc){
		printk(KERN_ERR "yamahaspk<%s>: fail to register char dev\n",__func__);
		goto error_unreg_device;
	}

	yamaha_dev_class = class_create(THIS_MODULE,"yamahaspk");
	if (IS_ERR(yamaha_dev_class)) {	
		rc = PTR_ERR(yamaha_dev_class);
		class_destroy(yamaha_dev_class);
		printk(KERN_ERR "yamahaspk<%s>: fail to  create class\n",__func__);
		goto error_unreg_device;
	}

	dev = device_create(yamaha_dev_class, NULL, MKDEV(YAMAHA_MAJOR, 1), NULL, "yamahaspk");
	if(IS_ERR(dev)){
		rc = PTR_ERR(dev);
		printk(KERN_ERR "yamahaspk<%s>: fail to  create device inode\n",__func__);
		goto error_create_device;
	}

	init_timer(&seven_sec_timer);
        seven_sec_timer.expires = jiffies + msecs_to_jiffies(7000);
        seven_sec_timer.function = &yamahaspk_timer_func;

	return 0;

error_create_device:
error_unreg_device:
error_alloc_dev:
	kfree(sysconfig_demoboard);
	return rc;
	
}

static void __exit yamahaspk_exit(void)
{
	int i; 
	device_destroy(yamaha_dev_class, MKDEV(YAMAHA_MAJOR, 1));

	for (i = 0; i < 6; i++){
		sw_gpio_irq_free(irq_config_demoboard[i].int_handler);
	}

	free_platform_gpio();

	input_unregister_device(yamaha_input_dev);
	input_free_device(yamaha_input_dev);
	kfree(sysconfig_demoboard);

	return ;
}

module_init(yamahaspk_init);
module_exit(yamahaspk_exit);

MODULE_AUTHOR("Wang Hua(whfyzg@gmail.com)");
MODULE_DESCRIPTION("io control for yamaha speaker demo board");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>


/*
 * Debug option
 */
#define GPIO_OK04_MODULE_DEBUG

#undef PDEBUG
#ifdef GPIO_OK04_MODULE_DEBUG
#  ifdef __KERNEL__
#    define PDEBUG(fmt, args...) printk(KERN_DEBUG "[GPIO-OK04] " fmt, ## args)
#  else
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...)
#endif

/* Module major number */
static int DEV_OK04_MAJOR_NUMBER = 224;
/* Module name */
#define DEV_OK04_NAME "gpio-ok04"


/* Macro for function select (mode)
 * 000 : input
 * 001 : output
 * 100 : alternate function 0
 * 101 : alternate function 1
 * 110 : alternate function 2
 * 111 : alternate function 3
 * 011 : alternate function 4
 * 010 : alternate function 5
 */
#define M_INPUT 0
#define M_OUTPUT 1

/* Macro for GPIO status */
#define S_LOW 0
#define S_HIGH 1

/* LED status */
#define S_OFF 0
#define S_ON 1

#define TIMER_DELAY 500000

/* controlled GPIO */
#define CUR_GPIO 16

/* I/O base address on the virtual memory in kernel */
#define BCM2837_PERI_BASE 0xF2000000

/* GPIO base address on the virtual memory in kernel */
#define GPIO_BASE (BCM2837_PERI_BASE + 0x00200000)

/* System Timer base address on the virtual memory in kernel */
#define TIMER_BASE (BCM2837_PERI_BASE + 0x00003000)

/* return GPIO base address */
static volatile unsigned int *get_gpio_addr()
{
	return (volatile unsigned int *)GPIO_BASE; 
}


static int set_bits(volatile unsigned int *addr
					const unsigned int shift,
					const unsigned int val,
					const unsigned int mask)
{
	unsigned int temp = *addr;

	/* initialize an assigned part */
	temp &= ~(mask << shift);

	/* set val into addr */
	temp |= (val & mask) << shift;
	*addr = temp;
	
	return 0;
}

/* 
 * assign a function of GPIO 16 to mode
 * Macro for function select (mode)
 * 000 : input
 * 001 : output
 * 100 : alternate function 0
 * 101 : alternate function 1
 * 110 : alternate function 2
 * 111 : alternate function 3
 * 011 : alternate function 4
 * 010 : alternate function 5
 */
static int func_pin(const unsigned int pin_num,
					const unsigned int mode)
{
	volatile unsigned int *gpio = get_gpio_addr(); /* return 0xF2200000 */
	/* we can set 10 gpio function to one register */
	unsigned int pin_bank = pin_num / 10;
	
	/* we can control total 53 gpio */
	if (pin_num > 53) {
		return -1;
	}

	/* one gpio can be reprented by 3 bits */
	if (mode > 7) {
		return -1;
	}

	gpio += pin_bank;
	
	/* assign 18~20bit of addr 0xF2200004 to mode */
	/* basic operation unit is sizeof(unsigned int) */
	/* access to GPIO 16 after (gpio + 1) */
	/* shift 0x7 because it is 111b */
	/* shfit+1, shift+2 set to 1b  */
	set_bits(gpio, (pin_num % 10) * 3, mode, 0x07);
	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);
	
	return 0;
}

static int set_pin(const unsigned int pin_num,
				   const unsigned int status)
{
	volatile unsigned int *gpio = get_gpio_addr();
	unsigned int pin_bank = pin_num >> 5;

	if (pin_num > 53) {
		return -1;
	}

	if (status != S_OFF && status != S_ON) {
		return -1;
	}

	gpio += pin_bank;

	if (status == S_OFF) {
		/* clear output */
		set_bits(gpio + (0x28/sizeof(unsigned int)), pin_num, S_HIGH, 0x01);
		set_bits(gpio + (0x28/sizeof(unsigned int)), pin_num, S_LOW, 0x01);
	} else if (status == S_ON) {
		/* set output */
		set_bits(gpio + (0x1C/sizeof(unsigned int)), pin_num, S_HIGH, 0x01);
		set_bits(gpio + (0x1C/sizeof(unsigned int)), pin_num, S_LOW, 0x01);
	}
	
	return 0;
}

/* return System Timer address */
static volatile unsigned int *get_timer_addr(void)
{
	return (volatile unsigned int *)TIEMR_BASE;
}

/* return time stamp */
static unsigned long get_time_stamp(void)
{
	volatile unsigned long *timer = (volatile unsigned long *)get_timer_addr();

	/* There is a register that can store time stamp
	   that is far as 0x04 from base */
	/* Write 64bits constant using two 32bits from 0x04 to 0x08 */
	return *(timer + 0x04/sizeof(unsigned int));
}

/*  */
static int timer_wait(const unsigned long delay)
{
	unsigned long start = get_time_stamp();
	unsigned long elapsed = 0;

	while (elapsed < delay) {
		elapsed = get_time_stamp() - start;
	}
	return 0;
}



static int ok04_open(struct inode *inode, struct file *filp)
{
	int i = 10;

	if (func_pin(CUR_GPIO, M_OUTPUT) != 0) {
		PDEBUG("%s:%d: func_pin() Error\n", __FUNCTION__, __LINE__);
		return -1;
	}
	
	while (i--) {
		if (set_pin(CUR_GPIO, S_OFF) != 0) {
			PDEBUG("%s:%d: set_pin() Error\n", __FUNCTION__, __LINE__);
			return -1;
		}

		timer_wait(TIMER_DELAY);

		if (set_pin(CUR_GPIO, S_ON) != 0) {
			PDEBUG("%s:%d: set_pin() Error\n", __FUNCTION__, __LINE__);
			return -1;
		}
		timer_wait(TIMER_DELAY);		
	}

	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);	
	return 0;
}

static int ok04_release(struct inode *inode, struct file *filp)
{
	set_pin(CUR_GPIO, S_OFF);
	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);	
	return 0;
}

static ssize_t ok04_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);	
	return 0;
}

static ssize_t ok04_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);	
	return 0;
}


static struct file_operations ok04_fops = {
	.owner = THIS_MODULE,
	.open = ok04_open,
	.release = ok04_release,
	.read = ok04_read,
	.write = ok04_write
};

static int ok04_init(void)
{
	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);
	register_chrdev(DEV_OK04_MAJOR_NUMBER, DEV_OK04_NAME, &ok04_fops);
	return 0;
}

static void ok04_exit(void)
{
	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);
	unregister_chrdev(DEV_OK04_MAJOR_NUMBER, DEV_OK04_NAME);
}


module_init(ok04_init);
module_exit(ok04_exit);
MODULE_LICENSE("Dual BSD/GPL");



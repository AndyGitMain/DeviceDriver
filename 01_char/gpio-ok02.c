
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>

/*
 * Debug option
 */
#define GPIO_OK02_MODULE_DEBUG

#undef PDEBUG
#ifdef GPIO_OK02_MODULE_DEBUG
#  ifdef __KERNEL__
#    define PDEBUG(fmt, args...) printk(KERN_DEBUG "[GPIO-OK02] " fmt, ## args)
#  else
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...)
#endif

/* Module major number */
static int DEV_OK02_MAJOR_NUMBER 222;
/* Module name */
#define DEV_OK02_NAME "gpio-ok02"

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

/* I/O base address on the virtual memory in kernel */
#define BCM2837_PERI_BASE 0xF2000000

/* GPIO base address on the virtual memory in kernel */
#define GPIO_BASE (BCM2837_PERI_BASE + 0x00200000)

/* return GPIO base address */
static volatile unsigned int *get_gpio_addr()
{
	return (volatile unsigned int *)GPIO_BASE; 
}

/* 주소 addr에 값 val을 mask만큼 잘라낸 후 shift만큼 이동시켜 할당하는 함수 */
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
static int func_pin_16(const unsigned int mode)
{
	volatile unsigned int *gpio = get_gpio_addr(); /* return 0xF2200000 */
	const unsigned int sel = 0x04;                 /* Function Select 1 to use GPIO 16 */
	const unsigned int shift = 18;                 /* GPIO 16 <== 18~20 bit */

	if (mode > 7) {
		return -1;
	}

	/* assign 18~20bit of addr 0xF2200004 to mode */
	/* basic operation unit is sizeof(unsigned int) */
	/* access to GPIO 16 after (gpio + 1) */
	/* shift 0x7 because it is 111b */
	/* shfit+1, shift+2 set to 1b  */
	set_bits(gpio + sel/sizeof(unsigned int), shift, mode, 0x07);
	
	return 0;
}


/* set GPIO 16 */
static int set_pin_16(void)
{
	volatile unsigned int *gpio = get_gpio_addr(); /* return 0xF2200000 */
	const unsigned int sel = 0x1C;
	const unsigned int shift = 16;

	/* initialize register that is GPIO 16 after setting to High */
	set_bits(gpio + sel/sizeof(unsigned int), shift, S_HIGH, 0x01);
	set_bits(gpio + sel/sizeof(unsigned int), shift, S_LOW, 0x01);
	
	return 0;
}

/* clear GPIO 16 */
static int clr_pin_16(void)
{
	volatile unsigned int *gpio = get_gpio_addr();
	const unsigned int sel = 0x28;
	const unsigned int shift = 16;

	/* initialize register that is GPIO 16 after setting to High */
	set_bits(gpio + sel/sizeof(unsigned int), shift, S_HIGH, 0x01);
	set_bits(gpio + sel/sizeof(unsigned int), shift, S_LOW, 0x01);
	
	return 0;
}

/* blink LED 10 times */
static int ok02_open(struct inode *inode, struct file *filp)
{
	int i = 10;

	volatile int busy_wait;
	if (func_pin_16(M_OUTPUT) != 0) {
		PDEBUG("%s:%d: Error!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	while(i--) {
		/* turn off LED */
		clr_pin_16();

		/* busy wait for delay time */
		busy_wait = 0x3F00000;
		while (busy_wait--);

		/* turn on LED */
		set_pin_16();

		/* busy wait for delay time */
		busy_wait = 0x3F00000;
		while (busy_wait--);
 	}

	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);
	
	return 0;
}

/* turn off LED that is connected to GPIO 16 and print a message */
static int ok02_release(struct inode *inode, struct file *filp)
{
	clr_pin_16();
	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);
	return 0;
}

static ssize_t ok02_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);
	return 0;
}

static ssize_t ok02_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);
	return 0;
}

static struct file_operations ok02_fops = {
	.owner = THIS_MODULE,
	.open = ok02_open,
	.release = ok02_release,
	.read = ok02_read,
	.write = ok02_write,
	//.ioctl = ok02_ioctl
};

static int ok02_init(void)
{
	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);
	register_chrdev(DEV_OK02_MAJOR_NUMBER, DEV_OK02_NAME, &ok02_fops);
	return 0;
}

static void ok02_exit(void)
{
	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);
	unregister_chrdev(DEV_OK02_MAJOR_NUMBER, DEV_OK02_NAME);
}

module_init(ok02_init);
module_exit(ok02_exit);
MODULE_LICENSE("Dual BSD/GPL");









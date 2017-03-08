#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>		/* size_t, loff_t */
#include <asm/uacess.h>			/* get_user() */

/*
 * Debug option
 */
#define GPIO_OK05_MODULE_DEBUG

#undef PDEBUG
#ifdef GPIO_OK05_MODULE_DEBUG
#  ifdef __KERNEL__
#    define PDEBUG(fmt, args...) printk(KERN_DEBUG "[GPIO-OK05] " fmt, ## args)
#  else
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...)
#endif

/* Module major number */
static int DEV_OK05_MAJOR_NUMBER = 225;
/* Module name */
#define DEV_OK05_NAME "gpio-ok05"


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

#define MORSE_DELAY 250000

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


static int ok05_open(struct inode *inode, struct file *filp)
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

static int ok05_release(struct inode *inode, struct file *filp)
{
	set_pin(CUR_GPIO, S_OFF);
	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);	
	return 0;
}

static int is_digit_or_alpha(const char character)
{
	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);
	if (character == ' ') return 0;
	if (character < '0' || character > 'z') return -1;
	if (character > '9' && character < 'A') return -1;
	if (character > 'Z' && character < 'a') return -1;
	return 0;	
}

static char to_upper(char character)
{
	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);	
	if (character >= 'a' && character <= 'z') {
		character -= 32;
	}
	return character;
}

/* dot action */
static int dot(void)
{
	printk(".");
	set_pin(CUR_GPIO, S_ON);
	timer_wait(MORSE_DELAY);
	set_pin(CUR_GPIO, S_OFF);
	timer_wait(MORSE_DELAY);
	return 0;
}

/* dash action */
static int dash(void)
{
	printk("-");
	set_pin(CUR_GPIO, S_ON);
	timer_wait(MORSE_DELAY);
	set_pin(CUR_GPIO, S_OFF);
	timer_wait(MORSE_DELAY);
	return 0;
}

/* output character to LED using morse code */
static int print_morse(char status)
{
	static char pre_status = 0;

	if (is_digit_or_alpha(status) != 0) {
		PDEBUG("%s:%s : is_digit_or_alpha() error\n", __FUNCTION__, __LINE__);
		return -1;
	}

	PDEBUG("%s:%d : status = %c\n", __FUNCTION__, __LINE__, status);
	status = to_upper(status);

	switch (status) {
    /* numbers */
    case '0' : dash(); dash(); dash(); dash(); dash(); break;
    case '1' : dot(); dash(); dash(); dash(); dash(); break;
    case '2' : dot(); dot(); dash(); dash(); dash(); break;
    case '3' : dot(); dot(); dot(); dash(); dash(); break;
    case '4' : dot(); dot(); dot(); dot(); dash(); break;
    case '5' : dot(); dot(); dot(); dot(); dot(); break;
    case '6' : dash(); dot(); dot(); dot(); dot(); break;
    case '7' : dash(); dash(); dot(); dot(); dot(); break;
    case '8' : dash(); dash(); dash(); dot(); dot(); break;
    case '9' : dash(); dash(); dash(); dash(); dot(); break;
    /* alphabets */
    case 'A' : dot(); dash(); break;
    case 'B' : dash(); dot(); dot(); dot(); break;
    case 'C' : dash(); dot(); dash(); dot(); break;
    case 'D' : dash(); dot(); dot(); break;
    case 'E' : dot(); break;
    case 'F' : dot(); dot(); dash(); dot(); break;
    case 'G' : dash(); dash(); dot(); break;
    case 'H' : dot(); dot(); dot(); dot(); break;
    case 'I' : dot(); dot(); break;
    case 'J' : dot(); dash(); dash(); dash(); break;
    case 'K' : dash(); dot(); dash(); break;
    case 'L' : dot(); dash(); dot(); dot(); break;
    case 'M' : dash(); dash(); break;
    case 'N' : dash(); dot(); break;
    case 'O' : dash(); dash(); dash(); break;
    case 'P' : dot(); dash(); dash(); dot(); break;
    case 'Q' : dash(); dash(); dot(); dash(); break;
    case 'R' : dot(); dash(); dot(); break;
    case 'S' : dot(); dot(); dot(); break;
    case 'T' : dash(); break;
    case 'U' : dot(); dot(); dash(); break;
    case 'V' : dot(); dot(); dot(); dash(); break;
    case 'W' : dot(); dash(); dash(); break;
    case 'X' : dash(); dot(); dot(); dash(); break;
    case 'Y' : dash(); dot(); dash(); dash(); break;
    case 'Z' : dash(); dash(); dot(); dot(); break;
	/* space */
	case ' ' : timer_wait(MORSE_DELAY * 6); break;
	}

	if (pre_status != status) {
		timer_wait(MORSE_DELAY * 2);
	}
	pre_status = status;
	printk("\n");
	return 0;
}


static ssize_t ok05_read(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);	
	return 0;
}

static ssize_t ok05_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	int i;
	char status;
	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);

	for (i = 0; i < count; i++) {
		get_user(status, (char *)buf + i);
		if (print_morse(status) != 0) {
			continue;
		}
	}
	return count;
}

static struct file_operations ok05_fops = {
	.owner = THIS_MODULE,
	.open = ok05_open,
	.release = ok05_release,
	.read = ok05_read,
	.write = ok05_write
};


static int ok05_init(void)
{
	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);
	register_chrdev(DEV_OK05_MAJOR_NUMBER, DEV_OK05_NAME, &ok05_fops);
	return 0;
}


static void ok05_exit(void)
{
	PDEBUG("%s:%d\n", __FUNCTION__, __LINE__);
	unregister_chrdev(DEV_OK05_MAJOR_NUMBER, DEV_OK05_NAME);
}


module_init(ok05_init);
module_exit(ok05_exit);
MODULE_LICENSE("Dual BSD/GPL");


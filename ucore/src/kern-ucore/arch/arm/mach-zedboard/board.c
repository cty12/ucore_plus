/*
 * =====================================================================================
 *
 *       Filename:  board.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/24/2012 01:21:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <board.h>
#include <picirq.h>
#include <serial.h>
#include <pmm.h>
#include <clock.h>

static const char *message = "Initializing ZEDBOARD...\n";

/*
static void put_string(const char *str)
{
	while (*str)
		serial_putc(*str++);
}
*/

#define EXT_CLK 38400000

void board_init_early()
{
	// put_string(message);
	gpio_init();
	gpio_test();

	serial_init(1, PER_IRQ_BASE_NONE_SPI + ZEDBOARD_UART1_IRQ);
	// Tianyu: we place pic init here
	//pic_init2(ZEDBOARD_APU_BASE);
	pic_init4(ZEDBOARD_APU_BASE);
	kprintf("Tianyu: picirq inited! \n");
	// Tianyu: we place clock init here
	//clock_init_arm(ZEDBOARD_TIMER0_BASE, GLOBAL_TIMER0_IRQ + PER_IRQ_BASE_SPI);
	clock_init_arm(0xF8F00600, 29);
	kprintf("Tianyu: clock inited! \n");
	uint32_t reg[5];
	asm (
		"ldr r0, =0x0\n"
		"mcr p15, 0, r0, c12, c0, 0\n"
	);
	asm(
		"mrc p15, 0, r1, c12, c0, 0\n"
		"ldr r0, =0xc004c1d4\n"
		"str r1, [r0]"
	);
	kprintf("&VBAR: 0x%08x, VBAR: 0x%08x\n", &reg[0], reg[0]);
	asm(
		"mrc p15, 0, r1, c1, c1, 0\n"
		"ldr r0, =0xc004c1d8\n"
		"str r1, [r0]"
	);
	kprintf("&SCR: 0x%08x, SCR: 0x%08x\n", &reg[1], reg[1]);
	asm(
		"mrc p15, 0, r1, c12, c0, 1\n"
		"ldr r0, =0xc004c1dc\n"
		"str r1, [r0]"
	);
	kprintf("&MVBAR: 0x%08x, MVBAR: 0x%08x\n", &reg[2], reg[2]);
	asm(
		"mrc p15, 0, r1, c1, c0, 0\n"
		"ldr r0, =0xc004c1e0\n"
		"str r1, [r0]"
	);
	kprintf("&SCTLR: 0x%08x, SCTLR: 0x%08x\n", &reg[3], reg[3]);
	asm volatile ("mrs r0, cpsr;"
			      "bic r0, r0, #0x80;"
			      "msr cpsr, r0;":::"r0", "memory", "cc");
	kprintf("Intr enabled!\n");
	/*asm (
		"mvn r0, #0\n"
		"ldr r1, =0xE000A040\n"
		"str r0, [r1]"
	);*/
	clock_test();
	while(1);
}

void board_init()
{
	return;
}

/* no nand */
int check_nandflash()
{
	return 0;
}

struct nand_chip *get_nand_chip()
{
	return NULL;
}

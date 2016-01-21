/**
	@Author: Tianyu Chen
	Tsinghua Univ. 2016
*/

#include <stdlib.h>
#include <types.h>
#include <arm.h>
#include <picirq.h>
#include <clock.h>
#include <serial.h>
#include <intr.h>
#include <kio.h>
#include <memlayout.h>
#include <assert.h>

#define ICCICR			0x00000100		// CPU Interface Control Register (RW)
#define ICCPMR			0x00000104		// CPU Interface Priority Mask Register (RW)
#define ICCIAR			0x0000010C		// Interrupt Acknowledge Register (RW)
#define ICCEOIR			0x00000110		// End of Interrupt Register (RW)

#define ICDDCR			0x00001000		// Distributor Control Register (RW)
#define ICDISR0			0x00001080		// Interrupt Security Register 0 (RW)
#define ICDISER0		0x00001100		// Distributor Interrupt Set Enable Register 0 (RW)
#define ICDICER0		0x00001180		// Distributor Interrupt Clear Enable Register 0 (RW)
#define ICDISPR0		0x00001200		// Distributor Set Pending Register 0 (RW)
#define ICDIPR0			0x00001400		// Distributor Interrupt Priority Register 0 (RW)
#define ICDIPTR0		0x00001800		// Interrupt Processor Targets Register (RO)
#define ICDICFR0		0x00001C00		// Interrupt Configuration Register (RO)
#define ICDSGIR			0x00001F00		// Distributor Software Generated Interrupt Register (RW)

#define MAX_IRQS_NR  128

static uint32_t apu_base = 0;

// Current IRQ mask
// static uint32_t irq_mask = 0xFFFFFFFF & ~(1 << INT_UART0);
static bool did_init = 0;

struct irq_action {
	ucore_irq_handler_t handler;
	void *opaque;
};

struct irq_action actions[MAX_IRQS_NR];

void pic_disable(unsigned int irq) {
	// not implemented
}

void pic_enable(unsigned int irq) {
	if (irq >= MAX_IRQS_NR)
		return;
	int off = irq / 32;
	int j = irq % 32;

	outw(apu_base + ICCICR, 0);		// Disable CPU Interface (it is already disabled, but nevermind)
	outw(apu_base + ICDDCR, 0);		// Disable Distributor (it is already disabled, but nevermind)

	outw(apu_base + ICDISER0 + off * 4, 1 << j);
	outw(apu_base + ICDIPTR0 + (irq & ~0x3), 0x01010101);

	outw(apu_base + ICCICR, 3);		// Enable CPU Interface
	outw(apu_base + ICDDCR, 1);		// Enable Distributor
}

void register_irq(int irq, ucore_irq_handler_t handler, void * opaque) {
	if (irq >= MAX_IRQS_NR) {
		kprintf("WARNING: register_irq: irq>=%d\n", MAX_IRQS_NR);
		return;
	}
	actions[irq].handler = handler;
	actions[irq].opaque = opaque;
}

void pic_init_early() {
	// not implemented
}

void pic_init() {
	// not implemented
}

/* pic_init
 * initialize the interrupt, but doesn't enable them */
void pic_init2(uint32_t base) {
	if (did_init)
		return;

	did_init = 1;
	//disable all
	apu_base = base;
	outw(apu_base + ICCICR, 0);			// Disable CPU Interface (it is already disabled, but nevermind)
	outw(apu_base + ICDDCR, 0);			// Disable Distributor (it is already disabled, but nevermind)
	outw(apu_base + ICCPMR, 0xffff);	// All interrupts whose priority is > 0xff are unmasked

	// fill all 1
	int i;
	for(i = 0; i < 32; i ++) {
		outw(apu_base + ICDICER0 + (i << 2), ~0);
	}

	outw(apu_base + ICCICR, 3);			// Enable CPU Interface
	outw(apu_base + ICDDCR, 1);			// Enable Distributor

}

void pic_init3(uint32_t base) {
	apu_base = base;
	/* PPI & SGI */
	//a. Specify which interrupts are Non-secure.
	//outw(apu_base + ICDISR0, 1 << 29); //CPU Private Timer
	kprintf("Interrupt Security Register 0: 0x%08x\n", inw(apu_base + ICDISR0));
	//b. Specify whether each interrupt is level-sensitive or edge-triggered.
	//c. Specify the priority value for each interrupt.
	//d. Enable the PPIs.
	outw(apu_base + ICDISER0, 1 << 29); //CPU Private Timer
	kprintf("Distributor Interrupt Set-Enable Register 0: 0x%08x\n", inw(apu_base + ICDISER0));
	/* CPU interface */
	//a. Set the priority mask for the interface.
	outw(apu_base + ICCPMR, 0xff);
	kprintf("CPU Interface Priority Mask Register: 0x%08x\n", inw(apu_base + ICCPMR));
	//b. Set the binary point position.
	//outw(apu_base + 0x0000011C, 0); //ICCABPR: Aliased Non-secure Binary Point Register
	kprintf("Non-secure Binary Point Register: 0x%08x\n", inw(apu_base + 0x0000011C));
	kprintf("Binary Point Register: 0x%08x\n", inw(apu_base + 0x00000108));
	//c. Enable signalling of interrupts by the interface.
	outw(apu_base + ICCICR, 7);
	kprintf("CPU Interface Control Register: 0x%08x\n", inw(apu_base + ICCICR));
	//d. Enable the Distributor.
	outw(apu_base + ICDDCR, 3);
	kprintf("Distributor Control Register: 0x%08x\n", inw(apu_base + ICDDCR));

	//Get Interrupt Processor Target
	kprintf("Global Timer Interrupt Target: CPU%d\n", (inw(apu_base + ICDIPTR0 + 27 & ~0x3) >> 24) - 1);
	kprintf("CPU Private Timer Interrupt Target: CPU%d\n", (inw(apu_base + ICDIPTR0 + 29 & ~0x3) >> 8) & 0xFF - 1);
	kprintf("SCU Non-secure Access Control Register: 0x%03x\n", inw(apu_base + 0x0054));
}

void pic_init4(uint32_t base) {
	apu_base = base;
	int int_id, cpu_id = 1;
	//a. Disable the Distributor.
	outw(apu_base + ICDDCR, 0);
	//b. Set the trigger mode
	for (int_id = 32; int_id < MAX_IRQS_NR; int_id += 16)
		outw(apu_base + ICDICFR0 + (int_id / 16) * 4, 0);
	//c. Set the default priority.
	for (int_id = 0; int_id < MAX_IRQS_NR; int_id += 4)
		outw(apu_base + ICDIPR0 + (int_id / 4) * 4, 0xa0a0a0a0);
	//d. cpu interface
	for (int_id = 32; int_id < MAX_IRQS_NR; int_id += 4) {
		cpu_id |= cpu_id << 8;
		cpu_id |= cpu_id << 16;
		outw(apu_base + ICDIPTR0 + (int_id / 4) * 4, cpu_id);
	}
	//e. Enable SPI.
	for (int_id = 0; int_id < MAX_IRQS_NR; int_id += 32)
		outw(apu_base + ICDICER0 + (int_id / 32) * 4, 0xffffffff);
	//f. Enable the Distributor.
	outw(apu_base + ICDDCR, 1);
	
	//a. Set the priority mask for the interface.
	outw(apu_base + ICCPMR, 0xf0);
	//b. Enable signalling of interrupts by the interface.
	outw(apu_base + ICCICR, 7);
	
	//Enable the interrupt for the Timer at GIC
	int int_id = 29;
	outw(apu_base + ICDISER0 + (int_id / 32) * 4, 1 << (int_id % 32));
}

void irq_handler() {
	uint32_t intnr = inw(apu_base + ICCIAR) & 0x3FF;
	if(actions[intnr].handler) {
		(* actions[intnr].handler) (intnr, actions[intnr].opaque);
	} else {
		panic("Unhandled HW IRQ %d\n", intnr);
	}

	// end of interrupt
	outw(apu_base + ICCEOIR, intnr & 0x3FF);
}

/* irq_clear
 * 	Clear a pending interrupt request
 *  necessary when handling an irq */
void irq_clear(unsigned int source) {
	// not implemented
}

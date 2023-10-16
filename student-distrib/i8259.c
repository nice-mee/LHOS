/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void i8259_init(void) {
    /* mask all the interrupts on the PIC */
    uint8_t INTR_MASK = 0xFF;
    outb(INTR_MASK, MASTER_8259_PORT + 1);   // 21h
    outb(INTR_MASK, SLAVE_8259_PORT + 1);    // A1h

    /* initialize the PIC */
    outb(ICW1, MASTER_8259_PORT);
    outb(ICW1, SLAVE_8259_PORT);

    outb(ICW2_MASTER, MASTER_8259_PORT + 1); // 21h
    outb(ICW2_SLAVE, SLAVE_8259_PORT + 1);   // A1h

    outb(ICW3_MASTER, MASTER_8259_PORT + 1); // 21h
    outb(ICW3_SLAVE, SLAVE_8259_PORT + 1);   // A1h

    outb(ICW4, MASTER_8259_PORT + 1);        // 21h
    outb(ICW4, SLAVE_8259_PORT + 1);         // A1h
    
    outb(INTR_MASK, MASTER_8259_PORT + 1);   // 21h
    outb(INTR_MASK, SLAVE_8259_PORT + 1);    // A1h

    /* unmask the port on the M PIC which connects the S PIC */
    INTR_MASK = 0xFB; 
    outb(INTR_MASK, MASTER_8259_PORT + 1);
}

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
    /* check if the arg is valid*/
    if(irq_num < 0 || irq_num > 15) {
        return;
    }
    // uint32_t irq_flag;
    uint8_t cur_irq;

    /* critical section */
    // spin_lock_irqsave(irq_status_lock[irq_num], irq_flag);
    if(irq_num < 8) {   // M PIC
        /* get current IRQ state */
        cur_irq = inb(MASTER_8259_PORT + 1);
        /* update the specified IRQ*/ 
        cur_irq &= ~(1<<irq_num);
        outb(cur_irq, MASTER_8259_PORT + 1);
    }
    else {  // S PIC
        cur_irq = inb(SLAVE_8259_PORT + 1);
        /* update the specified IRQ*/ 
        cur_irq &= ~(1<<(irq_num - 8));
        outb(cur_irq, SLAVE_8259_PORT + 1);
    }
    // spin_lock_irqrestore(irq_status_lock[irq_num], irq_flag);
}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
    /* check if the arg is valid*/
    if(irq_num < 0 || irq_num > 15) {
        return;
    }
    // uint32_t irq_flag;
    uint8_t cur_irq;

    /* critical section */
    // spin_lock_irqsave(irq_status_lock[irq_num], irq_flag);
    if(irq_num < 8) {   // M PIC
        /* get current IRQ state */
        cur_irq = inb(MASTER_8259_PORT + 1);
        /* update the specified IRQ*/ 
        cur_irq |= (1<<irq_num);
        outb(cur_irq, MASTER_8259_PORT + 1);
    }
    else {  // S PIC
        cur_irq = inb(SLAVE_8259_PORT + 1);
        /* update the specified IRQ*/ 
        cur_irq |= (1<<(irq_num - 8));
        outb(cur_irq, SLAVE_8259_PORT + 1);
    }
    // spin_lock_irqrestore(irq_status_lock[irq_num], irq_flag);
}

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {
    /* check if the arg is valid*/
    if(irq_num < 0 || irq_num > 15) {
        return;
    }
    // uint32_t irq_flag;

    /* critical section */
    // spin_lock_irqsave(irq_status_lock[irq_num], irq_flag);
    if(irq_num < 8) {   // M PIC
        outb(EOI | irq_num, MASTER_8259_PORT);
    }
    else {  // S PIC
        outb(EOI | (irq_num - 8), SLAVE_8259_PORT);
        outb(EOI | ICW3_SLAVE, MASTER_8259_PORT);   // update the M PIC
    }
    // spin_lock_irqrestore(irq_status_lock[irq_num], irq_flag);
}

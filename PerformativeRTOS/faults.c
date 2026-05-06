// Shell functions
// J Losh

// Sean Slater *why does this say Shell Functions???*

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "faults.h"

// student added includes
#include "uart0.h"
#include "asm.h"
#include "kernel.h"
#include "mm.h"
#include "shell.h"

// student added structs
// from kernel.c
extern uint8_t taskCurrent;
extern struct _tcb
{
    uint8_t state;                 // see STATE_ values above
    void *pid;                     // used to uniquely identify thread (add of task fn)
    void *sp;                      // current stack pointer
    uint8_t priority;              // 0=highest
    uint8_t currentPriority;       // 0=highest (needed for pi)
    uint32_t ticks;                // ticks until sleep complete
    uint64_t srd;                  // MPU subregion disable bits
    char name[16];                 // name of task used in ps command
    uint8_t mutex;                 // index of the mutex in use or blocking the thread
    uint8_t semaphore;             // index of the semaphore that is blocking the thread
    uint32_t cpu_time;             // student added, cpu timer
    uint32_t kernel_time;          // student added, os timer
} tcb[MAX_TASKS];

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void mpuFaultIsr(void) {
    // clear mem pending bit
    NVIC_SYS_HND_CTRL_R &= ~NVIC_SYS_HND_CTRL_MEMP;

    // print out data
    printFaultMessage(0);
    printCoreRegisters();
    printFaultStatRegister();
    printMPUFaultAddress();

    // inform user
    putsUart0("...task will be killed\n\r");
    printPengu(4);

    // set pendsv bit
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}

void hardFaultIsr(void) {
    // print out data
    printFaultMessage(1);
    printCoreRegisters();
    printHFaultStatRegister();

    // hard faults are not recoverable
    endlessLoop();
}

void busFaultIsr(void) {
    // clear bus pending bit
    NVIC_SYS_HND_CTRL_R &= ~NVIC_SYS_HND_CTRL_BUSP;

    // print message
    printFaultMessage(2);
    printBusFaultAddress();

    // inform user
    putsUart0("...task will be killed\n\r");
    printPengu(4);

    // set pendsv bit
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}

void usageFaultIsr(void) {
    // clear usage pending bit
    NVIC_SYS_HND_CTRL_R &= ~NVIC_SYS_HND_CTRL_USAGEP;

    // print message
    printFaultMessage(3);

    // inform user
    putsUart0("...task should be killed but it won't be...\n\r");
    printPengu(4);

    // set pendsv bit
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}

// prints fault messages based on type of fault
// 0: MPU fault         1: hard fault       2: bus fault        3: usage fault
void printFaultMessage(uint8_t which) {
    // which to switch? switch on which.
    switch (which) {
    case 0:
        putsUart0("\n\r ! MPU FAULT - PID: ");
        print8d(taskCurrent);
        putsUart0(" \"");
        putsUart0(tcb[taskCurrent].name);
        putsUart0("\"");
        putsUart0(" ! \n\r");
        break;
    case 1:
        putsUart0("\n\r ! HARD FAULT - PID: ");
        print8d(taskCurrent);
        putsUart0(" \"");
        putsUart0(tcb[taskCurrent].name);
        putsUart0("\"");
        putsUart0(" ! \n\r");
        break;
    case 2:
        putsUart0("\n\r ! BUS FAULT - PID: ");
        print8d(taskCurrent);
        putsUart0(" \"");
        putsUart0(tcb[taskCurrent].name);
        putsUart0("\"");
        putsUart0(" ! \n\r");
        break;
    case 3:
        putsUart0("\n\r ! USAGE FAULT - PID: ");
        print8d(taskCurrent);
        putsUart0(" \"");
        putsUart0(tcb[taskCurrent].name);
        putsUart0("\"");
        putsUart0(" ! \n\r");
        break;
    }
}

// dumps core registers during faults, for cleanliness
void printCoreRegisters() {
    uint32_t* psp = getPsp();               // get program stack pointer
    uint8_t is_unpriv = getTmpl();          // get if we are privileged or not

    uint32_t r0, r1, r2, r3, r12;
    uint32_t lr, pc, xPSR;                  // define registers for storage

    uint32_t* msp = getMsp();               // get MSP (always the current stack pointer)
    r0 = *(psp);                            // get all the register values off the stack
    r1 = *(psp+1);
    r2 = *(psp+2);
    r3 = *(psp+3);
    r12 = *(psp+4);
    lr = *(psp+5);
    pc = *(psp+6);
    xPSR = *(psp+7);                        // psp now points to program stack

    putsUart0("\tPSP:\t");                  // print out all values
    print32h((uint32_t)psp);
    putsUart0("\n\r\tMSP:\t");
    print32h((uint32_t)msp);
    putsUart0("\n\r");
    if (is_unpriv) {
        putsUart0("\tnot privileged!\n\r\t\tunable to print instruction\n\r");
    } else {
        putsUart0("\toff. inst.:\n\r\t\t");
        print32h(*((uint32_t*)pc));
        putsUart0("\n\r");
    }

    putsUart0("\tprocess stack dump:\n\r\txPSR:\t");
    print32h(xPSR);
    putsUart0("\n\r\tPC:\t");
    print32h(pc);
    putsUart0("\n\r\tLR:\t");
    print32h(lr);
    putsUart0("\n\r\tR0:\t");
    print32h(r0);
    putsUart0("\n\r\tR1:\t");
    print32h(r1);
    putsUart0("\n\r\tR2:\t");
    print32h(r2);
    putsUart0("\n\r\tR3:\t");
    print32h(r3);
    putsUart0("\n\r\tR12:\t");
    print32h(r12);
    putsUart0("\n\r");
}

// print fault stat register
void printFaultStatRegister() {
    uint32_t faultstat = NVIC_FAULT_STAT_R; // read in fault status register
    uint16_t ufaultstat = (faultstat & UFAULT_STAT_MASK) >> 16;
    uint8_t bfaultstat = (faultstat & BFAULT_STAT_MASK) >> 8;
    uint8_t mfaultstat = faultstat & MFAULT_STAT_MASK;

    putsUart0("\tfaulstat register:\n\r");
    putsUart0("\tu:\t");
    print16h(ufaultstat);
    putsUart0("\n\r");
    putsUart0("\tb:\t");
    print8h(bfaultstat);
    putsUart0("\n\r");
    putsUart0("\tm:\t");
    print8h(mfaultstat);
    putsUart0("\n\r");
}

// print hfault status register
void printHFaultStatRegister() {
    uint32_t hfaultstat = NVIC_HFAULT_STAT_R; // read in hfault status register

    putsUart0("\thfaulstat register:\n\r\t\t");
    print16h(hfaultstat);
    putsUart0("\n\r");
}

// print MPU fault address, if valid
void printMPUFaultAddress() {
    uint32_t mem_addr = NVIC_MM_ADDR_R;

    // check BFARV bit, make sure addr is valid, if not, print it out
    if (NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_MMARV) {
        putsUart0("\tm_addr:\t");
        print32h(mem_addr);
        putsUart0("\n\r");
    } else {
        putsUart0("\tMMARV: m_addr not valid!\n\r");
    }
}

// print bus fault address
void printBusFaultAddress() {
    uint32_t fault_addr = NVIC_FAULT_ADDR_R;

    // check BFARV bit, make sure addr is valid, if so, print it out
    if (NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_BFARV) {
        putsUart0("\tf_addr:\t");
        print32h(fault_addr);
        putsUart0("\n\r");
    } else {
        putsUart0("\tBFARV: f_addr not valid!\n\r");
    }
}

// enter an endless loop for checking register status
void endlessLoop() {
    while (1) {}
}

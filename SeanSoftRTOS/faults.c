// Shell functions
// J Losh

#include "tm4c123gh6pm.h"
#include "shell.h"
#include "tasks.h"

#include "faults.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// REQUIRED: code this function
void mpuFaultIsr(void)
{
    // print out data
    printFaultMessage(0);
    printCoreRegisters();
    printFaultStatRegister();
    printMPUFaultAddress();

    #ifndef RECOVERABLE
        idle();
    #endif

    // clear mem pending bit, set pendsv bit
    NVIC_SYS_HND_CTRL_R &= ~NVIC_SYS_HND_CTRL_MEMP;
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}

// REQUIRED: code this function
void hardFaultIsr(void)
{
    // print out data
    printFaultMessage(1);
    printCoreRegisters();
    printHFaultStatRegister();

    #ifndef RECOVERABLE
        idle();
    #endif
}

// REQUIRED: code this function
void busFaultIsr(void)
{
    // print message
    printFaultMessage(2);
    printBusFaultAddress();

    // clear bus pending bit
    NVIC_SYS_HND_CTRL_R &= ~NVIC_SYS_HND_CTRL_BUSP;

    #ifndef RECOVERABLE
        idle();
    #endif
}

// REQUIRED: code this function
void usageFaultIsr(void)
{
    // print message
    printFaultMessage(3);

    // clear usage pending bit
    NVIC_SYS_HND_CTRL_R &= ~NVIC_SYS_HND_CTRL_USAGEP;

    #ifndef RECOVERABLE
        idle();
    #endif
}


// Faults functions
// J Losh

// Sean Slater

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef FAULTS_H_
#define FAULTS_H_

// student added defines
#define UFAULT_STAT_MASK 0xFFFF0000
#define BFAULT_STAT_MASK 0x0000FF00
#define MFAULT_STAT_MASK 0x000000FF

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void mpuFaultIsr(void);
void hardFaultIsr(void);
void busFaultIsr(void);
void usageFaultIsr(void);

// student added functions
// for fault handling
void printFaultMessage(uint8_t which);
void printCoreRegisters();
void printFaultStatRegister();
void printHFaultStatRegister();

// for address printing
void printMPUFaultAddress();
void printBusFaultAddress();

// endless loop
void endlessLoop();

#endif

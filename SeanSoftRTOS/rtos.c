// RTOS Framework - Fall 2025
// J Losh

// Student Name: Sean Slater

#include <stdint.h>

#include "tm4c123gh6pm.h"
#include "clock.h"
#include "gpio.h"
#include "uart0.h"
#include "wait.h"
#include "mm.h"
#include "kernel.h"
#include "faults.h"
#include "tasks.h"
#include "shell.h"
#include "asm.h"

// compilation options
#define UART0_BAUD_RATE 115200
#define SYS_CLOCK_RATE 40e6
#define RECOVERABLE         // do we want to attempt to continue execution after faults?
                            // this might be broken up later into multiple options for each fault...

int main(void)
{
    bool ok;

    // Initialize hardware
    initSystemClockTo40Mhz();
    initHw();
    initUart0();
    initMemoryManager();
    initMpu();
    initRtos();

    // Setup UART0 baud rate
    setUart0BaudRate(UART0_BAUD_RATE, SYS_CLOCK_RATE);

    // Initialize mutexes and semaphores
    initMutex(resource);
    initSemaphore(keyPressed, 1);
    initSemaphore(keyReleased, 0);
    initSemaphore(flashReq, 5);

    // Add required idle process at lowest priority
    ok =  createThread(idle, "Idle", 7, 512);

    // Add other processes
    ok &= createThread(lengthyFn, "LengthyFn", 6, 1024);
    ok &= createThread(flash4Hz, "Flash4Hz", 4, 512);
    ok &= createThread(oneshot, "OneShot", 2, 1024);
    ok &= createThread(readKeys, "ReadKeys", 6, 512);
    ok &= createThread(debounce, "Debounce", 6, 1024);
    ok &= createThread(important, "Important", 1, 1024);
    ok &= createThread(uncooperative, "Uncoop", 6, 1024);
    ok &= createThread(errant, "Errant", 6, 1024);
    ok &= createThread(shell, "Shell", 0, 4096);

    // Start up RTOS
    if (ok)
        startRtos(); // never returns
    else
        while(true);
}

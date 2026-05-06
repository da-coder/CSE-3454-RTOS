// Kernel functions
// J Losh

#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdint.h>
#include <stdint.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// function pointer
typedef void (*_fn)();

// mutex
#define MAX_MUTEXES 1
#define MAX_MUTEX_QUEUE_SIZE 2
#define resource 0

// semaphore
#define MAX_SEMAPHORES 3
#define MAX_SEMAPHORE_QUEUE_SIZE 2
#define keyPressed 0
#define keyReleased 1
#define flashReq 2

// tasks
#define MAX_TASKS 12

// systick timer
#define SYSTICK_TIMER_MS 1

// tcb
#define NUM_PRIORITIES 8
struct _tcb
{
    uint8_t state;                  // see STATE_ values above
    void *pid;                      // used to uniquely identify thread (add of task fn)
    void *sp;                       // current stack pointer
    uint8_t priority;               // 0=highest
    uint8_t currentPriority;        // 0=highest (needed for pi)
    uint32_t ticks;                 // ticks until sleep complete
    uint32_t srd;                   // MPU subregion disable bits (why 64b???)
    char name[16];                  // name of task used in ps command
    uint8_t mutex;                  // index of the mutex in use or blocking the thread
    uint8_t semaphore;              // index of the semaphore that is blocking the thread
} tcb[MAX_TASKS];

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

bool initMutex(uint8_t mutex);
bool initSemaphore(uint8_t semaphore, uint8_t count);

void initRtos(void);
uint8_t rtosScheduler(void);
void startRtos(void);

bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes);
void killThread(_fn fn);
void restartThread(_fn fn);
void setThreadPriority(_fn fn, uint8_t priority);

void yield(void);
void sleep(uint32_t tick);
void wait(int8_t semaphore);
void post(int8_t semaphore);
void lock(int8_t mutex);
void unlock(int8_t mutex);

void systickIsr(void);
void pendSvIsr(void);
void svCallIsr(void);

void rebootuC();

#endif

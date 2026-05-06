// Kernel functions
// J Losh

// Sean Slater

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef KERNEL_H_
#define KERNEL_H_

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

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

// student added defines
// systick timer
#define SYSTICK_TIMER_MS 1

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

bool initMutex(uint8_t mutex);
bool initSemaphore(uint8_t semaphore, uint8_t count);

void initRtos(void);
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

// student added structs (so shell.c can see this)
typedef struct {
    bool ps;            // priority scheduler
    bool pi;            // priority inheritance
    bool pr;            // preemption

    uint8_t ct;         // current task
    uint8_t tc;         // task count

    uint8_t s;          // state
    uint8_t p;          // priority
    uint8_t cp;         // current priority (pi=on)
    void* pid;          // pid, function pointer
    char n[16];         // name of function

    uint8_t qs;         // mutex queue size
    uint8_t lc;         // locked / count
    void* pq[MAX_SEMAPHORES]; // process queue, take the larger of the two; for tasks this is used for timer!
} p_data;

// student added functions
void requestData(uint8_t task, p_data* response);
void offerData(uint8_t type, p_data* response);
void rebootuC();
uint32_t getCurrentTimeus();
void privKillThread(void* pid);
void privRestartThread(void* pid);

#endif

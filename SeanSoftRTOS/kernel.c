// Kernel functions
// J Losh

#include "tm4c123gh6pm.h"
#include "mm.h"
#include "asm.h"

#include "kernel.h"

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// mutex
typedef struct _mutex
{
    bool lock;
    uint8_t queueSize;
    uint8_t processQueue[MAX_MUTEX_QUEUE_SIZE];
    uint8_t lockedBy;
} mutex;
mutex mutexes[MAX_MUTEXES];

// semaphore
typedef struct _semaphore
{
    uint8_t count;
    uint8_t queueSize;
    uint8_t processQueue[MAX_SEMAPHORE_QUEUE_SIZE];
} semaphore;
semaphore semaphores[MAX_SEMAPHORES];

// task states
#define STATE_INVALID           0   // no task
#define STATE_UNRUN             1   // task has never been run
#define STATE_READY             2   // has run, can resume at any time
#define STATE_DELAYED           3   // has run, but now awaiting timer
#define STATE_BLOCKED_SEMAPHORE 4   // has run, but now blocked by semaphore
#define STATE_BLOCKED_MUTEX     5   // has run, but now blocked by mutex
#define STATE_KILLED            6   // task has been killed

// task
uint8_t taskCurrent = 0;            // index of last dispatched task
uint8_t taskCount = 0;              // total number of valid tasks

// control
bool priorityScheduler = true;       // priority (true) or round-robin (false)
bool priorityInheritance = false;    // priority inheritance for mutexes
bool preemption = false;             // preemption (true) or cooperative (false)

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

bool initMutex(uint8_t mutex)
{
    bool ok = (mutex < MAX_MUTEXES);
    if (ok)
    {
        mutexes[mutex].lock = false;
        mutexes[mutex].lockedBy = 0;
    }
    return ok;
}

bool initSemaphore(uint8_t semaphore, uint8_t count)
{
    bool ok = (semaphore < MAX_SEMAPHORES);
    {
        semaphores[semaphore].count = count;
    }
    return ok;
}

void initRtos(void)
{
    uint8_t i;
    uint32_t st_ctrl;

    // no tasks running
    taskCount = 0;

    // clear out tcb records
    for (i = 0; i < MAX_TASKS; i++)
    {
        tcb[i].state = STATE_INVALID;
        tcb[i].pid = 0;
    }

    // set reload count based on specified ms
    st_ctrl = (SYSTICK_TIMER_MS * 4000) - 1;    // PIOSC is 16Mhz / 4 = 4Mhz; 1ms -> 0.25us = 4000 per ms
    NVIC_ST_RELOAD_R |= NVIC_ST_RELOAD_M & st_ctrl;

    // enable systick and related interrupt
    st_ctrl = NVIC_ST_CTRL_R;
    st_ctrl &= ~NVIC_ST_CTRL_CLK_SRC;   // use the PIOSC
    NVIC_ST_CTRL_R = st_ctrl | NVIC_ST_CTRL_INTEN | NVIC_ST_CTRL_ENABLE;
}

uint8_t rtosScheduler(void) {
    bool ok = false;
    static uint8_t task = 0xFF;         // done so this starts at 0
    uint8_t highest_p = 255;
    int i;

    if (priorityScheduler) {
        taskCount = 0;
        for (i=0; i<MAX_TASKS; i++) {   // iterate over every task in queue
            // filter out tasks that are NOT ready to run
            if (tcb[i].state != STATE_READY && tcb[i].state != STATE_UNRUN) {
                continue;
            }
            taskCount++;                // increment number of tasks valid to run

            // find highest priority task; does not handle two of same priority!
            if (tcb[i].priority < highest_p) {
                highest_p = tcb[i].priority;
                task = i;               // mark this as the currently selected task
            }
        }

        if (task == 0xFF) {
            task = 0;                   // mark the task as not found, set for 0 (idle?)
        }
    } else {
        // is the number of valid tasks needed for roundrobin scheduling?
        while (!ok)                     // iterate until we find a task that is ready to run
        {
            task++;
            if (task >= MAX_TASKS)      // wrap around
                task = 0;
            ok = (tcb[task].state == STATE_READY || tcb[task].state == STATE_UNRUN);
        }
    }
    return task;                        // return array index of process to run
}

void startRtos(void) {
    // call scheduler
    uint8_t i = rtosScheduler();

    // apply srd bits
    applySramAccessMask(tcb[i].srd);

    // set currently running task (might be called in pendSV instead?)
    taskCurrent = i;

    // update task state to READY?

    // set PSP, set ASP, call function
    startFunc(tcb[i].sp, tcb[i].pid);
}

bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes)
{
    int index = 0;
    uint8_t i = 0;
    bool ok = false;
    bool found = false;
    if (taskCount < MAX_TASKS)
    {
        // make sure fn not already in list (prevent reentrancy)
        while (!found && (i < MAX_TASKS))
        {
            found = (tcb[i++].pid ==  fn);
        }
        if (!found)
        {
            // find first available tcb record
            i = 0;
            while (tcb[i].state != STATE_INVALID) {i++;}
            tcb[i].state = STATE_UNRUN;
            tcb[i].pid = fn;
            tcb[i].priority = priority;

            // copy over name
            while (name[index] != '\0') {
                tcb[i].name[index] = name[index];
                index++;
                if (index >= 16) {
                    return false;
                }
            }

            // allocate stack space, assign a stack pointer
            // malloc returns a pointer to LOWEST memory address, sp needs HIGHEST
            current_pid = fn;
            stackBytes = roundSizeUp(stackBytes);
            tcb[i].sp = (void*)((uint32_t)mallocHeap(stackBytes) + stackBytes);

            // get srdBits for this pid from malloc
            tcb[i].srd = createSramAccessMask(fn);

            // increment task count
            taskCount++;
            ok = true;
        }
    }
    return ok;
}

// REQUIRED: modify this function to kill a thread
// REQUIRED: free memory, remove any pending semaphore waiting,
//           unlock any mutexes, mark state as killed
void killThread(_fn fn)
{
}

// REQUIRED: modify this function to restart a thread, including creating a stack
void restartThread(_fn fn)
{
}

// REQUIRED: modify this function to set a thread priority
void setThreadPriority(_fn fn, uint8_t priority)
{
}

void yield(void) {
    __asm(" SVC #0");
    __asm(" BX LR");
}

// REQUIRED: modify this function to support 1ms system timer
// execution yielded back to scheduler until time elapses using pendsv
void sleep(uint32_t tick)
{
}

// REQUIRED: modify this function to wait a semaphore using pendsv
void wait(int8_t semaphore)
{
}

// REQUIRED: modify this function to signal a semaphore is available using pendsv
void post(int8_t semaphore)
{
}

// REQUIRED: modify this function to lock a mutex using pendsv
void lock(int8_t mutex)
{
}

// REQUIRED: modify this function to unlock a mutex using pendsv
void unlock(int8_t mutex)
{
}

void systickIsr(void) {
    int i;

    // iterate over each task
    for (i=0; i<MAX_TASKS; i++) {
        // is the process waiting on timer?
        if (tcb[i].state == STATE_DELAYED) {
            // decrement timer
            tcb[i].ticks--;
            // check if timer is complete (pendSV might be better suited for this check)
            if (tcb[i].ticks <= 0) {
                // time has elapsed, switch state to ready
                tcb[i].state = STATE_READY;
            }
        }
    }

    // preemption handling, if enabled
    if (preemption) {
        // call pendsv to switch tasks, if needed
        NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
    }
}

void pendSvIsr(void) {
    // if the MPU DERR or IERR bits are set, clear them and return
    if (NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_DERR) {
        NVIC_FAULT_STAT_R != NVIC_FAULT_STAT_DERR;
        return;
    }
    if (NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_IERR) {
        NVIC_FAULT_STAT_R != NVIC_FAULT_STAT_IERR;
        return;
    }

    // unset pendsv (set unpendsv)
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_UNPEND_SV;

    // current task will never be unrun
    // is new task unrun or ready? *this function should only be called if it's one or the other!
    // save current context
    // store nu-LR (EXC_RETURN)
    __asm(" MRS R0, PSP");          // get PSP without overwriting LR
    __asm(" MOV R1, R0");           // copy PSP into R1 for modification
    __asm(" SUB R1, R1, #4");       // sub 4 from it, byte right before R0
    __asm(" STR LR, [R1]");         // store at that address for later
    // already saved: R0-3, R12, and xPSR
    // save: R4-11, R13-15
    __asm(" SUB R1, R1, #4");       // increment to next byte
    __asm(" STR R4, [R1]");         // store R4
    __asm(" SUB R1, R1, #4");       // rinse and repeat
    __asm(" STR R5, [R1]");         // store R5
    __asm(" SUB R1, R1, #4");
    __asm(" STR R6, [R1]");         // store R6
    __asm(" SUB R1, R1, #4");
    __asm(" STR R7, [R1]");         // store R7
    __asm(" SUB R1, R1, #4");
    __asm(" STR R8, [R1]");         // store R8
    __asm(" SUB R1, R1, #4");
    __asm(" STR R9, [R1]");         // store R9
    __asm(" SUB R1, R1, #4");
    __asm(" STR R10, [R1]");        // store R10
    __asm(" SUB R1, R1, #4");
    __asm(" STR R11, [R1]");        // store R11
    __asm(" SUB R1, R1, #4");
    __asm(" STR R13, [R1]");        // store R13
    // store sp -> tcb
    void* psp = getPsp();           // get PSP (again, but this time where C likes it)
    tcb[taskCurrent].sp = psp;      // store PSP in table

    // get which task to run, should only return a task which is unrun or ready!
    uint8_t nu_task = rtosScheduler();

    // check if new task is the same as current task
    if (nu_task == taskCurrent) {
        // nothing else to do here
        return;
    }

    // apply SRAM access mask
    applySramAccessMask(tcb[nu_task].srd);

    // set currently running task (might be called in pendSV instead?)
    taskCurrent = nu_task;

    // update to new PSP
    setPsp((uint32_t*)tcb[nu_task].sp);

    // handle restore differently if unrun
    if (tcb[nu_task].state == STATE_UNRUN) {
        callFunc(tcb[nu_task].pid);     // sets up stack for exception return
        __asm(" MVN LR, #2");           // set LR to 0xFFFF.FFFD
    } else {
        // restore new context
        __asm(" MRS R0, PSP");          // make sure PSP is in R0
        __asm(" MOV R1, R0");           // copy PSP into R1 for modification
        __asm(" SUB R1, R1, #4");       // sub 4 from it, byte right before R0
        __asm(" LDR LR, [R1]");         // restore from that address
        // already saved: R0-3, R12, and xPSR
        // save: R4-11, R13-14
        __asm(" SUB R1, R1, #4");       // increment to next byte
        __asm(" LDR R4, [R1]");         // restore R4
        __asm(" SUB R1, R1, #4");       // rinse and repeat
        __asm(" LDR R5, [R1]");         // restore R5
        __asm(" SUB R1, R1, #4");
        __asm(" LDR R6, [R1]");         // restore R6
        __asm(" SUB R1, R1, #4");
        __asm(" LDR R7, [R1]");         // restore R7
        __asm(" SUB R1, R1, #4");
        __asm(" LDR R8, [R1]");         // restore R8
        __asm(" SUB R1, R1, #4");
        __asm(" LDR R9, [R1]");         // restore R9
        __asm(" SUB R1, R1, #4");
        __asm(" LDR R10, [R1]");        // restore R10
        __asm(" SUB R1, R1, #4");
        __asm(" LDR R11, [R1]");        // restore R11
        __asm(" SUB R1, R1, #4");
        __asm(" LDR R13, [R1]");        // restore R13
    }

    // task switch
    __asm(" BX LR");
}

// REQUIRED: modify this function to add support for the service call
// REQUIRED: in preemptive code, add code to handle synchronization primitives
void svCallIsr(void) {
    // call pendSV (function call = 0)
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}

// sets the bit in APINT register to reset entire microcontroller
void rebootuC() {
    NVIC_APINT_R |= NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
}


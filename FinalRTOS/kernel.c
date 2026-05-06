// Kernel functions
// J Losh

// Sean Slater

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
#include "mm.h"
#include "kernel.h"

// student added includes
#include "asm.h"

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
#define STATE_INVALID           0 // no task
#define STATE_UNRUN             1 // task has never been run
#define STATE_READY             2 // has run, can resume at any time
#define STATE_DELAYED           3 // has run, but now awaiting timer
#define STATE_BLOCKED_SEMAPHORE 4 // has run, but now blocked by semaphore
#define STATE_BLOCKED_MUTEX     5 // has run, but now blocked by mutex
#define STATE_KILLED            6 // task has been killed

// task
uint8_t taskCurrent = 0;          // index of last dispatched task
uint8_t taskCount = 0;            // total number of valid tasks

// control
bool priorityScheduler = true;    // priority (true) or round-robin (false)
bool priorityInheritance = false; // priority inheritance for mutexes
bool preemption = false;          // preemption (true) or cooperative (false)

// tcb
#define NUM_PRIORITIES   8
struct _tcb
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

// student added global variables
uint8_t prior_tasks[NUM_PRIORITIES];

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

        // this is added to save time in rtosScheduler later
        tcb[i].priority = 255;
    }

    // set reload count based on specified ms
    st_ctrl = (SYSTICK_TIMER_MS * 4000) - 1;    // PIOSC is 16Mhz / 4 = 4Mhz; 1ms -> 0.25us = 4000 per ms
    NVIC_ST_RELOAD_R |= NVIC_ST_RELOAD_M & st_ctrl;

    // enable systick and related interrupt
    st_ctrl = NVIC_ST_CTRL_R;
    st_ctrl &= ~NVIC_ST_CTRL_CLK_SRC;   // use the PIOSC
    NVIC_ST_CTRL_R = st_ctrl | NVIC_ST_CTRL_INTEN | NVIC_ST_CTRL_ENABLE;

    // enable timer for calculating cpu time
    SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER0;
    TIMER0_CTL_R &= ~TIMER_CTL_TAEN;    // make sure enable bit is off before making changes
    TIMER0_CTL_R |= TIMER_CTL_TASTALL;  // enable stall during debug breakpoints
    TIMER0_CFG_R = TIMER_CFG_32_BIT_TIMER;
    TIMER0_TAMR_R |= TIMER_TAMR_TACDIR | TIMER_TAMR_TAMR_1_SHOT | TIMER_TAMR_TASNAPS;
    TIMER0_TAILR_R = 0xFFFFFFFF;        // set load register (stop point for counting up!)
    TIMER0_CTL_R |= TIMER_CTL_TAEN;     // set running
}

uint8_t rtosScheduler(void)
{
    uint8_t state;                      // local variable of state
    uint8_t priority;                   // local variable of priority
    uint8_t h_p = NUM_PRIORITIES;       // highest priority found ready to run, default is 8 (not 7!)
    uint8_t t_hp[MAX_TASKS];            // array of highest priority tasks
    uint8_t n_hp;                       // number of tasks of highest priority
    int i;

    // priority scheduler
    if (priorityScheduler) {
        // iterate over all valid tasks
        for (i=0; i<taskCount; i++) {
            // dereference state only once
            state = tcb[i].state;

            // check state
            if (state == STATE_READY || state == STATE_UNRUN) {
                // dereference priority from array
                priority = tcb[i].priority;

                // check if it is a higher priority
                if (h_p > priority) {
                    // set new highest priority
                    h_p = priority;

                    // add to array
                    n_hp = 0;
                }

                // check if it's the same priority
                if (h_p == priority) {
                    t_hp[n_hp] = i;         // add to array
                    n_hp++;                 // increment size
                }
            }
        }

        // iterate over tasks of highest priority
        for (i=0; i<n_hp; i++) {
            // compare this task with previously running task
            if (t_hp[i] > prior_tasks[h_p]) {
                prior_tasks[h_p] = t_hp[i];
                return t_hp[i];         // return task to run
            }
        }

        // if we hit here, no tasks further in queue of highest priority
        prior_tasks[h_p] = t_hp[0];     // set previously run
        return t_hp[0];                 // return task to run
    }

    // round robin scheduling
    t_hp[0] = taskCurrent;              // start at currently running task
    while (true) {
        t_hp[0]++;                      // increment to next task

        if (t_hp[0] >= MAX_TASKS) {     // detect overflow
            t_hp[0] = 0;
        }

        state = tcb[t_hp[0]].state;     // dereference state only once
        if (state == STATE_READY || state == STATE_UNRUN) {
            return t_hp[0];             // return task to run
        }
    }
}

void startRtos(void)
{
    // call scheduler
    taskCurrent = rtosScheduler();

    // apply srd bits
    applySramAccessMask(tcb[taskCurrent].srd);

    // update task state to READY
    tcb[taskCurrent].state = STATE_READY;

    // set PSP, set ASP, call function
    coldStart(tcb[taskCurrent].sp, tcb[taskCurrent].pid);
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
            tcb[i].currentPriority = priority;

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
            taskCurrent = i;
            stackBytes = roundSizeUp(stackBytes);
            tcb[i].sp = (void*)((uint32_t)mallocHeap(stackBytes) + stackBytes);

            // increment task count
            taskCount++;
            ok = true;
        }
    }
    return ok;
}

void killThread(_fn fn)
{
    __asm(" SVC #8");
}
void privKillThread(void* pid) {
    // this function is only to be called by privileged functions!
    int i, j;
    uint8_t taskCurrentSave;

    // iterate over every task
    for (i=0; i<MAX_TASKS; i++) {
        // compare function pointer with pid
        if (tcb[i].pid == pid) {
            // unlock any mutexes
            for (j=0; j<mutexes[tcb[i].mutex].queueSize; j++) {
                // remove from process queue
                if (mutexes[tcb[i].mutex].processQueue[j] == i) {
                    mutexes[tcb[i].mutex].processQueue[j] = mutexes[tcb[i].mutex].processQueue[j+1];
                    mutexes[tcb[i].mutex].queueSize--;
                }
            }
            // remove locked / locked by
            if (mutexes[tcb[i].mutex].lockedBy == i) {
                mutexes[tcb[i].mutex].lock = false;
            }

            // remove any semaphores
            for (j=0; j<semaphores[tcb[i].semaphore].queueSize; j++) {
                if (semaphores[tcb[i].semaphore].processQueue[j] == i) {
                    semaphores[tcb[i].semaphore].processQueue[j] = semaphores[tcb[i].semaphore].processQueue[j+1];
                    semaphores[tcb[i].semaphore].queueSize--;
                }
            }

            // there isn't a way implemented to see if this function had a semaphore assigned to it?

            // kill this task
            tcb[i].state = STATE_KILLED;

            // save current task for restoration later
            taskCurrentSave = taskCurrent;

            // overwrite current task so free will always work
            taskCurrent = i;

            // clear space on the heap
            freeHeap(tcb[i].sp);

            // restore current task
            taskCurrent = taskCurrentSave;

            // delete timer info
            tcb[i].cpu_time = 0;
            tcb[i].kernel_time = 0;

            return;
        }
    }

    return;
}

void restartThread(_fn fn)
{
    __asm(" SVC #7");
}
void privRestartThread(void* pid) {
    // should only be called by priv functions!
    int i, j;
    uint8_t taskCurrentSave;
    uint32_t offset;

    // save current task
    taskCurrentSave = taskCurrent;

    // iterate over every task
    for (i=0; i<MAX_TASKS; i++) {
        // compare function pointer with pid
        if (tcb[i].pid == pid) {
            // overwrite current task with the one we want to run
            taskCurrent = i;

            // set this task to unrun, will restart PC
            tcb[i].state = STATE_UNRUN;

            // iterate over all malloc entries
            for (j=0; j<NUM_SRAM_SUBREGS*NUM_SRAM_REGIONS; j++) {
                // compare pid with function pointer
                if (heap_alloc[j].pid == pid) {
                    // reset stack pointer; this is just restarting a running task
                    offset = (uint32_t)heap_alloc[j].n * SUBREGION_SIZE;
                    tcb[i].sp = (void*)((uint32_t)heap_alloc[j].p + offset);

                    // restore current task before returning!
                    taskCurrent = taskCurrentSave;
                    return;
                }
            }
            // if no region alloc'd in malloc, return a new sp with 1024 bytes (size unknown as it isn't stored!)
            tcb[i].sp = (void*)((uint32_t)mallocHeap(SUBREGION_SIZE) + 1024);

            break;
        }
    }

    // restore current task
    taskCurrent = taskCurrentSave;
    return;
}

void setThreadPriority(_fn fn, uint8_t priority)
{
    __asm(" SVC #6");
}

void yield(void)
{
    __asm(" SVC #0");
}

void sleep(uint32_t tick)
{
    __asm(" SVC #1");
}

void wait(int8_t semaphore)
{
    __asm(" SVC #2");
}

void post(int8_t semaphore)
{
    __asm(" SVC #3");
}

void lock(int8_t mutex)
{
    __asm(" SVC #4");
}

void unlock(int8_t mutex)
{
    __asm(" SVC #5");
}

// student added SVC calls:
void requestData(uint8_t req, p_data* res) {
    __asm(" SVC #9");
}
void offerData(uint8_t type, p_data* response) {
    __asm(" SVC #10");
}
void rebootuC() {
    __asm(" SVC #11");
}

// student added functions:
uint32_t getCurrentTimeus() {
    return NVIC_ST_CURRENT_R & NVIC_ST_CURRENT_M;
}


// ISR stuff
void systickIsr(void)
{
    int i, j;
    uint8_t using;
    uint8_t waiting;

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

    // priority inheritance handling, if enabled
    if (priorityInheritance) {
        // iterate over all mutexes
        for (i=0; i<MAX_MUTEXES; i++) {
            using = mutexes[i].lockedBy;
            // iterate over all of the processQueue
            for (j=0; j<mutexes[i].queueSize; j++) {
                waiting = mutexes[i].processQueue[j];
                // if process in queue is lesser priority, elevate it to match using's
                if (tcb[using].currentPriority >= tcb[waiting].priority) {
                    tcb[using].currentPriority = tcb[waiting].currentPriority;
                } else {
                    // else, reset the current priority
                    tcb[waiting].currentPriority = tcb[waiting].priority;
                }
            }
        }

        // iterate over all semaphores
        for (i=0; i<MAX_SEMAPHORES; i++) {

            // there is no way to get which processes are currently using the semaphores!!!

            for (j=0; j<semaphores[i].queueSize; j++) {
                waiting = semaphores[i].processQueue[j];
            }
        }
    } else {
        for (i=0; i<taskCount; i++) {
            tcb[i].currentPriority = tcb[i].priority;
        }
    }

    // preemption handling, if enabled
    if (preemption) {
        // call pendsv to switch tasks
        NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
    }
}

void pendSvIsr(void)
{
    uint8_t taskCurrentSave;

    // unset pendsv (set unpendsv)
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_UNPEND_SV;

    // store per-task timer
    tcb[taskCurrent].cpu_time = TIMER0_TAR_R;

    // save current context
    storeRegs(&(tcb[taskCurrent].sp));
    taskCurrentSave = taskCurrent;

    // if the MPU DERR or IERR bits are set, clear them, kill task
    if (NVIC_FAULT_STAT_R) {
        if (NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_DERR)
            NVIC_FAULT_STAT_R |= NVIC_FAULT_STAT_DERR;
        if (NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_IERR)
            NVIC_FAULT_STAT_R |= NVIC_FAULT_STAT_IERR;
        privKillThread(tcb[taskCurrent].pid);
    }

    // get which task to run, should only return a task which is unrun or ready!
    taskCurrent = rtosScheduler();

    // apply SRAM access mask
    applySramAccessMask(tcb[taskCurrent].srd);

    // restore new context
    if (tcb[taskCurrent].state == STATE_UNRUN) {
        setupUnrun(tcb[taskCurrent].pid, tcb[taskCurrent].sp);
        tcb[taskCurrent].sp = getPsp();
    }
    restoreRegs(&(tcb[taskCurrent].sp));    // restore all the other regs, update PSP

    // run task
    tcb[taskCurrent].state = STATE_READY;

    // get kernel time
    tcb[taskCurrentSave].kernel_time = TIMER0_TAR_R;

    // zero timers for per-task and kernel
    TIMER0_TAV_R = 0;

}

void svCallIsr(void)
{
    bool is_good;
    uint8_t i;
    uint32_t m, avg_t, total_t;
    p_data* p;
    void* v;

    m = getSVCa();                  // get SVC argument
    v = getSVCb();                  // get SVC data pass thru

    switch(getSVCn()) {             // switch on SVC #N; pendSV is called at end! [return to skip, break to task switch]
    case 0:                         // yield
        break;
    case 1:                         // sleep
        tcb[taskCurrent].state = STATE_DELAYED;
        tcb[taskCurrent].ticks = m;
        break;
    case 2:                         // wait
        // is count zero?
        if (semaphores[m].count == 0) {
            // yes, task switch
        } else {
            // no, return execution to task
            semaphores[m].count--;
            return;
        }

        // is task already in the queue?
        for (i=0; i<semaphores[m].queueSize; i++) {
            if (semaphores[m].processQueue[i] == taskCurrent) {
                // task switch away; can't use break here
                NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
                return;
            }
        }

        // check queue size
        if (semaphores[m].queueSize < MAX_SEMAPHORE_QUEUE_SIZE) {
            // task can be fit into queue; add it
            semaphores[m].processQueue[semaphores[m].queueSize] = taskCurrent;
            semaphores[m].queueSize++;

            // update state
            tcb[taskCurrent].state = STATE_BLOCKED_SEMAPHORE;
            tcb[taskCurrent].semaphore = m;
        } else {
            // no room left in queue; kill it
            privKillThread(tcb[taskCurrent].pid);
        }

        // task switch away
        break;
    case 3:                         // post
        // check if anyone waiting in queue
        if (semaphores[m].queueSize) {
            // set the first-in to ready
            tcb[semaphores[m].processQueue[0]].state = STATE_READY;

            // iterate over rest of queue; shift them over
            for (i=1; i<semaphores[m].queueSize; i++) {
                semaphores[m].processQueue[i-1] = semaphores[m].processQueue[i];
            }
            semaphores[m].queueSize--;
        } else {
            // increment count
            semaphores[m].count++;
        }

        // return to execution
        return;
    case 4:                         // lock
        if (!mutexes[m].lock) {
            // mutex is unlocked; lock it for this process
            mutexes[m].lock = true;
            mutexes[m].lockedBy = taskCurrent;

            // return without task switching
            return;
        } else {
            // mutex is locked; check for task in queue
            for (i=0; i<mutexes[m].queueSize; i++) {
                // is this process already waiting?
                if (mutexes[m].processQueue[i] == taskCurrent) {
                    // yes, task switch
                    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
                    return;
                }
            }

            // is there space in the queue?
            if (mutexes[m].queueSize < MAX_MUTEX_QUEUE_SIZE) {
                // yes, add it
                mutexes[m].processQueue[mutexes[0].queueSize] = taskCurrent;
                mutexes[m].queueSize++;
                tcb[taskCurrent].state = STATE_BLOCKED_MUTEX;
                tcb[taskCurrent].mutex = m;
            } else {
                // no room in queue, kill them and task switch
                privKillThread(tcb[taskCurrent].pid);
            }
        }

        // task switch
        break;
    case 5:                         // unlock
        // check if calling function has ownership of this mutex, if not kill it
        if (mutexes[m].lockedBy != taskCurrent) {
            privKillThread(tcb[taskCurrent].pid);
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
            return;
        }
        mutexes[m].lockedBy = 0;

        // check if mutex is locked, if so unlock
        if (!mutexes[m].lock) {
            privKillThread(tcb[taskCurrent].pid);
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
            return;
        }
        mutexes[m].lock = false;

        // check if anyone waiting on mutex, if so set them to ready
        if (mutexes[m].queueSize) {
            // set the first in to ready
            tcb[mutexes[m].processQueue[0]].state = STATE_READY;

            // lock the mutex for this application
            mutexes[m].lock = true;
            mutexes[m].lockedBy = mutexes[m].processQueue[0];

            // iterate over rest of queue and move them down a slot
            for (i=1; i<mutexes[m].queueSize; i++) {
                mutexes[m].processQueue[i-1] = mutexes[m].processQueue[i];
            }

            // decrement queue size
            mutexes[m].queueSize--;
        }

        // return without switching
        return;
    case 6:                         // set priority
        // iterate over every task
        for (i=0; i<MAX_TASKS; i++) {
            // compare function pointer with pid
            if (tcb[i].pid == (void*)m) {
                // update priority
                tcb[i].priority = (uint8_t)v;
                return;
            }
        }

        // no task found, should not pendsv
        return;
    case 7:                         // restart task
        privRestartThread((void*)m);
        return;
    case 8:                         // kill task
        privKillThread((void*)m);
        return;
    case 9:                         // request for privileged information
        // check that the task calling this has access to this memory!
        is_good = false;
        for (i=0; i<NUM_SRAM_SUBREGS*NUM_SRAM_REGIONS; i++) {
            // find which subregion the pointer v is in (upper and lower bounds)
            if ((void*)(i*SUBREGION_SIZE+START_ADDR) < v) {
                if ((void*)((i+1)*SUBREGION_SIZE+START_ADDR) >= v) {
                    // is this subregion owned by the current task?
                    if (heap_alloc[i].pid == tcb[taskCurrent].pid) {
                        // flag is good and break out of loop
                        is_good = true;
                        break;
                    }
                }
            }
        }

        // check end of memory for p_data as well
        if ((void*)(i*SUBREGION_SIZE+START_ADDR+sizeof(p_data)) > v) {
            is_good = false;
        }
        if ((void*)((i+1)*SUBREGION_SIZE+START_ADDR+sizeof(p_data)) < v) {
            is_good = false;
        }

        if (!is_good) {
            // current task doesn't own all of the memory space needed
            privKillThread(tcb[taskCurrent].pid);
            return;
        }

        // cast to p_data struct pointer
        p = (p_data*)v;

        // return task info
        if (m < MAX_TASKS) {
            // global variables
            p->ps = priorityScheduler;
            p->pi = priorityInheritance;
            p->pr = preemption;
            p->ct = taskCurrent;
            p->tc = taskCount;

            // per task
            p->s = tcb[m].state;
            p->p = tcb[m].priority;
            p->cp = tcb[m].currentPriority;
            p->pid = tcb[m].pid;

            // copy name over
            i = 0;
            while (tcb[m].name[i] != '\0') {
                p->n[i] = tcb[m].name[i];
                i++;

                if (i >= 16) {
                    p->n[15] = '\0';
                    break;
                }
            }
            p->n[i] = '\0';

            // calculate CPU time so as to not be preempted!
            total_t = 0;
            avg_t = 0;
            for (i=0; i<taskCount; i++) {           // sum up difference of os time and task time; end at 0
                total_t += tcb[i].cpu_time;         // calculate total task time
                avg_t += tcb[i].kernel_time - tcb[i].cpu_time;  // calculate total kernel time
            }
            avg_t = avg_t / (taskCount * 40);       // divide by number of tasks

            // cpu time
            p->pq[0] = (void*)tcb[m].cpu_time;
            p->pq[1] = (void*)total_t;
            p->pq[2] = (void*)avg_t;

            // don't care
            p->qs = 0;
            p->lc = 0;

            return;
        }

        // return mutex info
        if (m < MAX_TASKS+MAX_MUTEXES) {
            // copy mutex stuff over
            p->qs = mutexes[m-MAX_TASKS].queueSize;
            p->lc = mutexes[m-MAX_TASKS].lock;
            p->pid = tcb[mutexes[m-MAX_TASKS].lockedBy].pid;

            // copy over each task in process queue
            for (i=0; i<mutexes[m-MAX_TASKS].queueSize; i++) {
                p->pq[i] = tcb[mutexes[m-MAX_TASKS].processQueue[i]].pid;
            }

            // don't care
            p->ps = 0;
            p->pi = 0;
            p->pr = 0;
            p->ct = 0;
            p->tc = 0;
            p->s = 0;
            p->p = 0;
            p->cp = 0;
            p->n[0] = '\0';

            return;
        }

        // return semaphore info
        if (m < MAX_TASKS+MAX_MUTEXES+MAX_SEMAPHORES) {
            // copy semaphore stuff over
            p->qs = semaphores[m-MAX_TASKS].queueSize;
            p->lc = semaphores[m-MAX_TASKS].count;

            // copy over each task in process queue
            for (i=0; i<semaphores[m-MAX_TASKS].queueSize; i++) {
                p->pq[i] = tcb[semaphores[m-MAX_TASKS].processQueue[i]].pid;
            }

            // don't care
            p->ps = 0;
            p->pi = 0;
            p->pr = 0;
            p->ct = 0;
            p->tc = 0;
            p->s = 0;
            p->p = 0;
            p->cp = 0;
            p->n[0] = '\0';
        }

        // needed so that semaphore info return doesn't call pendsv
        return;
    case 10:                        // update rtos settings
        priorityScheduler = p->ps;
        priorityInheritance = p->pi;
        preemption = p->pr;

        // no need to pendsv task switch here
        return;
    case 11:                        // sets the bit in APINT register to reset uC
        NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
    }

    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}


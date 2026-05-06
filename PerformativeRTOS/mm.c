// Memory manager functions
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

// student added includes
#include "kernel.h"

// student added structs and globals
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

void * mallocHeap(uint32_t size_in_bytes) {
    // get current pid
    void* current_pid = tcb[taskCurrent].pid;
    void* p;
    int num_of_subregions;

    // round up bytes to next multiple of 1024
    size_in_bytes = roundSizeUp(size_in_bytes);

    // check if we have space available
    int i, is_free = 0;
    int first_free_ID = -1;
    int num_subregions = NUM_SRAM_SUBREGS*NUM_SRAM_REGIONS;
    for (i=0; i<num_subregions; i++) {      // iterate over all subregions
        if (!heap_alloc[i].is_alloc) {      // is the subregion free?
            is_free++;                      // yes, add it to our counter
            if (first_free_ID == -1) {      // if this is the first free subregion
                first_free_ID = i;          // store it to save time later
            }
        }
    }
    if ((size_in_bytes / SUBREGION_SIZE) > is_free) {  // are we asking for more than we have available?
        return (void*)0;                    // return 0 pointer, we don't have enough space left!
    }
    if (first_free_ID == -1) {
        return (void*)0;                    // never found the first free space?
    }

    // set srdbits in regions to match what is required, update heap_alloc
    tcb[taskCurrent].srd = 0;               // reset srd bits
    tcb[taskCurrent].srd--;
    p = heap_alloc[first_free_ID].p;
    num_of_subregions = size_in_bytes / SUBREGION_SIZE;
    for (i=first_free_ID; i<num_of_subregions+first_free_ID; i++) {
        heap_alloc[i].is_alloc = 1;         // indicate that this memory region is allocated
        heap_alloc[i].pid = current_pid;    // set PID to currently running PID
        heap_alloc[i].n = num_of_subregions;// set number of subregions

        // create srdBitMasks
        tcb[taskCurrent].srd &= ~(0x100 << i);  // flip bit to 1 at the correct index; centering number!
    }

    // return pointer to memory
    return p;
}

void freeHeap(void *address_from_malloc) {
    // get current pid
    void* current_pid = tcb[taskCurrent].pid;

    // find pointer in heap_alloc
    int i;
    int num_subregions = NUM_SRAM_SUBREGS*NUM_SRAM_REGIONS;
    for (i=0; i<num_subregions; i++) {
        if (heap_alloc[i].pid == current_pid) {
            heap_alloc[i].is_alloc = 0;     // set not alloc'd
            heap_alloc[i].pid = 0;          // clear pid
            heap_alloc[i].n = 0;            // clear n subregs
            tcb[taskCurrent].srd = 0xFFFFFFFFFFFFFFFF;  // reset srd bits
        }
    }
}

void initMemoryManager(void) {
    int num_subregions = NUM_SRAM_SUBREGS*NUM_SRAM_REGIONS;
    int i;                                  // set up subregions

    for (i=0; i<4; i++) {
        heap_alloc[i].p = (void*)(START_ADDR + i*SUBREGION_SIZE);
        heap_alloc[i].is_alloc = 1;         // set OS stack as allocated already
        heap_alloc[i].pid = 0;              // default PID will be 0
        heap_alloc[i].n = 4;                // set 4 sections allocated
    }
    for (i=4; i<num_subregions; i++) {      // iterate over every subregion
        heap_alloc[i].p = (void*)(START_ADDR + i*SUBREGION_SIZE);
        heap_alloc[i].is_alloc = 0;         // allocate memory in sections of 8192, set is_alloc false
        heap_alloc[i].pid = 0;              // default PID will be 0
        heap_alloc[i].n = 0;                // default number of sections allocated is 0
    }
}

void initMpu(void) {
    // enable flash access
    NVIC_MPU_NUMBER_R = MPU_REGNUM_FLASH;   // select MPU region for flash
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SIZE_M & (MPU_REGSIZE_FLASH << 1); // set size
    NVIC_MPU_BASE_R |= NVIC_MPU_BASE_ADDR_M & MPU_REGADR_FLASH;         // set base address
    NVIC_MPU_ATTR_R |= MPU_AP_OSRWURO;      // set OS: +R+W+X   user: +R+X
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_ENABLE;// enable region

    // enable peripheral access
    NVIC_MPU_NUMBER_R = MPU_REGNUM_PERI;  // select MPU region for peripherals
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SIZE_M & (MPU_REGSIZE_PERI << 1);// set size
    NVIC_MPU_BASE_R |= NVIC_MPU_BASE_ADDR_M & MPU_REGADR_PERI;        // set base address
    NVIC_MPU_ATTR_R |= MPU_AP_OSRWURW;      // set OS: +R+W     user: +R+W
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN;    // disable executable
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_ENABLE;// enable region

    // enable SRAM access
    NVIC_MPU_NUMBER_R = MPU_REGNUM_SRAM_0;  // select MPU region for SRAM 0
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SIZE_M & (MPU_REGSIZE_SRAM << 1);  // set size
    NVIC_MPU_BASE_R |= NVIC_MPU_BASE_ADDR_M & MPU_REGADR_SRAM_0;   // set base address
    NVIC_MPU_ATTR_R |= MPU_AP_OSRWURW;      // set OS: +R+W     user: +R+W
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN;    // disable executable
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_ENABLE;// enable region

    NVIC_MPU_NUMBER_R = MPU_REGNUM_SRAM_1;  // select MPU region for SRAM 1
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SIZE_M & (MPU_REGSIZE_SRAM << 1);  // set size
    NVIC_MPU_BASE_R |= NVIC_MPU_BASE_ADDR_M & MPU_REGADR_SRAM_1;        // set base address
    NVIC_MPU_ATTR_R |= MPU_AP_OSRWURW;      // set OS: +R+W     user: +R+W
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN;    // disable executable
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_ENABLE;// enable region

    NVIC_MPU_NUMBER_R = MPU_REGNUM_SRAM_2;  // select MPU region for SRAM 2
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SIZE_M & (MPU_REGSIZE_SRAM << 1);  // set size
    NVIC_MPU_BASE_R |= NVIC_MPU_BASE_ADDR_M & MPU_REGADR_SRAM_2;        // set base address
    NVIC_MPU_ATTR_R |= MPU_AP_OSRWURW;      // set OS: +R+W     user: +R+W
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN;    // disable executable
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_ENABLE;// enable region

    NVIC_MPU_NUMBER_R = MPU_REGNUM_SRAM_3;  // select MPU region for SRAM 3
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SIZE_M & (MPU_REGSIZE_SRAM << 1);  // set size
    NVIC_MPU_BASE_R |= NVIC_MPU_BASE_ADDR_M & MPU_REGADR_SRAM_3;        // set base address
    NVIC_MPU_ATTR_R |= MPU_AP_OSRWURW;      // set OS: +R+W     user: +R+W
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN;    // disable executable
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_ENABLE;// enable region

    // enable faults
    NVIC_SYS_HND_CTRL_R |= NVIC_SYS_HND_CTRL_USAGE;
    NVIC_SYS_HND_CTRL_R |= NVIC_SYS_HND_CTRL_BUS;
    NVIC_SYS_HND_CTRL_R |= NVIC_SYS_HND_CTRL_MEM;
    NVIC_CFG_CTRL_R |= NVIC_CFG_CTRL_DIV0;

    // MPU enable background rule and enable
    NVIC_MPU_CTRL_R |= NVIC_MPU_CTRL_PRIVDEFEN | NVIC_MPU_CTRL_ENABLE;
}

// custom functions here:
// helper function to round up to multiple of 1024
uint32_t roundSizeUp(uint32_t size_in) {
    // round up to next 1024B; this could be sped up using shifting...
    if (size_in % SUBREGION_SIZE == 0) {
        // do nothing, already a multiple of 1024
    } else {
        // round 'em up, YEEHAW
        size_in = (size_in / SUBREGION_SIZE) + 1;
        size_in = size_in * SUBREGION_SIZE;
    }

    return size_in;
}

// updates srd bits in each region of sram
void applySramAccessMask(uint64_t srd) {
    uint32_t attr_state;
    // 11111111 11111111 XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX 11111111 11111111
    //                   |<------ srd bits are here ------>|

    NVIC_MPU_NUMBER_R = MPU_REGNUM_SRAM_0;              // set region to SRAM0
    attr_state = NVIC_MPU_ATTR_R & ~NVIC_MPU_ATTR_SRD_M;// get current ATTR state, remove all srd bits
    attr_state |= NVIC_MPU_ATTR_SRD_M & srd;            // apply srd bit mask
    NVIC_MPU_ATTR_R = attr_state;                       // copy to ATTR register

    NVIC_MPU_NUMBER_R = MPU_REGNUM_SRAM_1;              // set region to SRAM1
    attr_state = NVIC_MPU_ATTR_R & ~NVIC_MPU_ATTR_SRD_M;
    attr_state |= NVIC_MPU_ATTR_SRD_M & (srd >> 8);
    NVIC_MPU_ATTR_R = attr_state;

    NVIC_MPU_NUMBER_R = MPU_REGNUM_SRAM_2;              // set region to SRAM2
    attr_state = NVIC_MPU_ATTR_R & ~NVIC_MPU_ATTR_SRD_M;
    attr_state |= NVIC_MPU_ATTR_SRD_M & (srd >> 16);
    NVIC_MPU_ATTR_R = attr_state;

    NVIC_MPU_NUMBER_R = MPU_REGNUM_SRAM_3;              // set region to SRAM3
    attr_state = NVIC_MPU_ATTR_R & ~NVIC_MPU_ATTR_SRD_M;
    attr_state |= NVIC_MPU_ATTR_SRD_M & (srd >> 32);
    NVIC_MPU_ATTR_R = attr_state;
}

// Memory manager functions
// J Losh

// Sean Slater

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef MM_H_
#define MM_H_

#define NUM_SRAM_REGIONS 4                      // first project files reference this...
                                                // second project files are missing kernel.c / kernel.h
                                                // new mm.c / mm.h missing this definition
                                                // changed to 4

// student added defines
#define NUM_SRAM_SUBREGS 8
#define REGION_SIZE 8192
#define SUBREGION_SIZE 1024
#define START_ADDR 0x20000000
#define END_ADDR 0x20008000

#define MPU_REGADR_FLASH 0x00000000             // define base address of regions
#define MPU_REGADR_SRAM_0 0x20000000            // increments of 8kib = 0x2000
#define MPU_REGADR_SRAM_1 0x20002000
#define MPU_REGADR_SRAM_2 0x20004000
#define MPU_REGADR_SRAM_3 0x20006000
#define MPU_REGADR_PERI 0x40000000

#define MPU_AP_NOACCESS 0x00000000              // os: no access    user: no access
#define MPU_AP_OSRW 0x01000000                  // os: read/write   user: no access
#define MPU_AP_OSRWURO 0x02000000               // os: read/write   user: read only
#define MPU_AP_OSRWURW 0x03000000               // os: read/write   user: read/write
#define MPU_AP_OSRO 0x04000000                  // os: read only    user: no access
#define MPU_AP_OSROURO 0x06000000               // os: read only    user: read only

#define MPU_REGNUM_FLASH 0 & NVIC_MPU_NUMBER_M; // creates a uint32_t for MPU number
#define MPU_REGNUM_SRAM_0 1 & NVIC_MPU_NUMBER_M;
#define MPU_REGNUM_SRAM_1 2 & NVIC_MPU_NUMBER_M;
#define MPU_REGNUM_SRAM_2 3 & NVIC_MPU_NUMBER_M;
#define MPU_REGNUM_SRAM_3 4 & NVIC_MPU_NUMBER_M;
#define MPU_REGNUM_PERI 6 & NVIC_MPU_NUMBER_M;

#define MPU_REGSIZE_FLASH 28                    // size as defined in datasheet for passing to MPU ATTR
#define MPU_REGSIZE_SRAM 12
#define MPU_REGSIZE_PERI 25

// structs
typedef struct _memory_alloc {
    void* p;                                    // base memory address
    uint8_t is_alloc;                           // is subregion alloc'd?, 1 or 0 for TRUE / FALSE
    void* pid;                                  // owner PID for free_heap
    uint8_t n;                                  // number of subregions in this alloc
} memory_alloc;

// externs
extern char* g_heap;

// global variable for heap allocation
memory_alloc heap_alloc[NUM_SRAM_SUBREGS*NUM_SRAM_REGIONS];

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void * mallocHeap(uint32_t size_in_bytes);
void freeHeap(void *address_from_malloc);
void initMemoryManager(void);
void initMpu(void);

// student added functions
uint32_t roundSizeUp(uint32_t size_in);
void applySramAccessMask(uint64_t srd);

#endif

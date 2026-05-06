// RTOS header file
// functions, defines, etc.

// includes
#include <stdint.h>
#include <stdbool.h>

#include "asm.h"
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "gpio.h"
#include "uart0.h"
#include "nvic.h"

// defines
#define ENDLESS_HARD                            // determines if handler enters endless loop or not
#define ENDLESS_BUS
#define ENDLESS_USAGE
//#define ENDLESS_PENDSV

#define UART0_BAUD_RATE 115200

#define UFAULT_STAT_MASK 0xFFFF0000
#define BFAULT_STAT_MASK 0x0000FF00
#define MFAULT_STAT_MASK 0x000000FF

#define MPU_REGNUM_FLASH 0 & NVIC_MPU_NUMBER_M; // creates a uint32_t for MPU number
#define MPU_REGNUM_SRAM_0 1 & NVIC_MPU_NUMBER_M;
#define MPU_REGNUM_SRAM_1 2 & NVIC_MPU_NUMBER_M;
#define MPU_REGNUM_SRAM_2 3 & NVIC_MPU_NUMBER_M;
#define MPU_REGNUM_SRAM_3 4 & NVIC_MPU_NUMBER_M;
#define MPU_REGNUM_PERI 6 & NVIC_MPU_NUMBER_M;

#define MPU_REGSIZE_FLASH 28                    // size as defined in datasheet for passing to MPU ATTR
#define MPU_REGSIZE_SRAM 12
#define MPU_REGSIZE_PERI 25

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

#define ONB_RED_LED 1                           // PORT F
#define ONB_BLUE_LED 2
#define ONB_GREEN_LED 3

#define GPIO_BUTTON_0 0                         // PORT B
#define GPIO_BUTTON_1 1
#define GPIO_BUTTON_2 2
#define GPIO_BUTTON_3 3
#define GPIO_BUTTON_4 4
#define GPIO_BUTTON_5 5
#define GPIO_BUTTON_0_MASK 0x01                 // 0000 0001
#define GPIO_BUTTON_1_MASK 0x02                 // 0000 0010
#define GPIO_BUTTON_2_MASK 0x04                 // 0000 0100
#define GPIO_BUTTON_3_MASK 0x08                 // 0000 1000
#define GPIO_BUTTON_4_MASK 0x10                 // 0001 0000
#define GPIO_BUTTON_5_MASK 0x20                 // 0010 0000

#define GPIO_GREEN_LED 4                        // PORT A
#define GPIO_YELLOW_LED 3
#define GPIO_ORANGE_LED 2
#define GPIO_RED_LED 0                          // PORT E
#define GPIO_BLUE_LED 2                         // PORT F

#define MALLOC_SUBREGION_SIZE 1024              // size of sram0-3/0-8 subregions in bytes
#define MALLOC_REGION_SIZE 8192                 // size of sram0-3 regions in bytes
#define MALLOC_REGION_NUM 4                     // number of regions
#define MALLOC_SUBREGION_NUM 8                  // number of subregions per region
#define MALLOC_START_ADDR 0x20000000            // start address of SRAM0 region

// structs
typedef struct _memory_alloc {
    void* p;                                    // base memory address
    bool is_alloc;                              // is subregion alloc'd?
    uint8_t PID;                                // owner PID for free_heap
    uint8_t n;                                  // number of subregions in this alloc
} memory_alloc;

// externs
extern char* g_heap;

// handlers
void hardFaultHandler();
void busFaultHandler();
void usageFaultHandler();
void mpuFaultHandler();
void pendsvHandler();
void nmiHandler();
void svcHandler();

// causers (troublemakers!)
void causeBusFault();
void causeHardFault();
int causeUsageFault();
void causeMPUFault();
void causePendSVFault();

// output
void printFaultAddr();
void print32h(uint32_t x);
void print32b(uint32_t x);
void print16h(uint32_t x);
void print8h(uint8_t x);
void print8d(uint8_t x);
void print8b(uint8_t x);
void print4h(uint8_t x);

// functions
int init();
int initMalloc();
void endlessLoop();
void yield();
void rebootuC();
void getInput();

// required functions for project
void* malloc_heap(int size_in_bytes);
int free_heap(void* p);
void allowFlashAccess();
void allowPeripheralAccess();
void setupSramAccess();
uint32_t createNoSramAccessMask();
uint32_t createSramAccessMask();
void applySramAccessMask(uint32_t srdBitMask);
void addSramAccessWindow(uint32_t *baseAdd, uint32_t size_in_bytes);

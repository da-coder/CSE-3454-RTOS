// Sean Slater
// RTOS CSE 4354
// Fall 2025
// Dr. Losh

// Rewritten 15 Sep 2025
// Switching from CCS Cloud to CCS 12 due to instability
// CCS 20 / cloud appear to both be ElectronJS and are not very stable
// CCS 12 appears to be Eclipse and is very stable (98%)

// must-have
#include "main.h"

uint8_t __pid = 69;                         // temporary pid, global, __ to remind me to remove this!
memory_alloc heap_alloc[MALLOC_SUBREGION_NUM*MALLOC_REGION_NUM];    // global variable to keep track of heap for malloc_heap

// handlers
void hardFaultHandler() {
    uint32_t* psp = getPsp();               // get PSP first

    putsUart0("\n\r ! HARD FAULT - PID:");  // warn user
    print8d(__pid);                         // print out PID
    putsUart0(" ! \n\r");

    // dump registers
    uint32_t r0, r1, r2, r3, r12;
    uint32_t lr, pc, xPSR, msp_, psp_;      // define registers for storage

    uint32_t* msp = getMsp();               // get MSP (always the current stack pointer)
    msp_ = (uint32_t)msp;                   // save for later, if needed
    psp_ = (uint32_t)psp;                   // save pointer for later
    r0 = *(psp++);                          // get all the register values off the stack
    r1 = *(psp++);
    r2 = *(psp++);
    r3 = *(psp++);
    r12 = *(psp++);
    lr = *(psp++);
    pc = *(psp++);
    xPSR = *(psp++);                        // psp now points to program stack

    putsUart0("\tPSP:\t");                  // print out all values
    print32h(psp_);
    putsUart0("\n\r");
    putsUart0("\tMSP:\t");
    print32h(msp_);
    putsUart0("\n\r");
    putsUart0("\toff. inst.:\n\r\t\t");
    print32h(*((uint32_t*)pc));
    putsUart0("\n\r");

    putsUart0("\tprocess stack dump:\n\r");
    putsUart0("\txPSR:\t");
    print32h(xPSR);
    putsUart0("\n\r");
    putsUart0("\tPC:\t");
    print32h(pc);
    putsUart0("\n\r");
    putsUart0("\tLR:\t");
    print32h(lr);
    putsUart0("\n\r");
    putsUart0("\tR0:\t");
    print32h(r0);
    putsUart0("\n\r");
    putsUart0("\tR1:\t");
    print32h(r1);
    putsUart0("\n\r");
    putsUart0("\tR2:\t");
    print32h(r2);
    putsUart0("\n\r");
    putsUart0("\tR3:\t");
    print32h(r3);
    putsUart0("\n\r");
    putsUart0("\tR12:\t");
    print32h(r12);
    putsUart0("\n\r");

    // print h-fault stat register
    uint32_t hfaultstat = NVIC_HFAULT_STAT_R; // read in hfault status register

    putsUart0("\thfaulstat register:\n\r\t\t");
    print16h(hfaultstat);
    putsUart0("\n\r");

#ifdef ENDLESS_HARD
    endlessLoop();
#endif
}
void busFaultHandler() {
    uint32_t* psp = getPsp();

    putsUart0("\n\r ! BUS FAULT - PID:");
    print8d(__pid);
    putsUart0(" ! \n\r");
    printFaultAddr();

    // clear bus pending bit
    NVIC_SYS_HND_CTRL_R &= ~NVIC_SYS_HND_CTRL_BUSP;

#ifdef ENDLESS_BUS
    endlessLoop();
#endif
}
void usageFaultHandler() {
    putsUart0("\n\r ! USAGE FAULT - PID:");
    print8d(__pid);
    putsUart0(" ! \n\r");

    // clear usage pending bit
    NVIC_SYS_HND_CTRL_R &= ~NVIC_SYS_HND_CTRL_USAGEP;

#ifdef ENDLESS_USAGE
    endlessLoop();
#endif
}
void mpuFaultHandler() {
    uint32_t* psp = getPsp();

    putsUart0("\n\r ! MPU FAULT - PID:");
    print8d(__pid);
    putsUart0(" ! \n\r");

    // dump registers
    uint32_t r0, r1, r2, r3, r12;
    uint32_t lr, pc, xPSR;                  // define registers for storage

    uint32_t* msp = getMsp();               // get MSP (always the current stack pointer)
    r0 = *(psp);                            // get all the register values off the stack
    r1 = *(psp+1);
    r2 = *(psp+2);
    r3 = *(psp+3);
    r12 = *(psp+4);
    lr = *(psp+5);
    pc = *(psp+6);
    xPSR = *(psp+7);                        // psp now points to program stack

    putsUart0("\tPSP:\t");                  // print out all values
    print32h((uint32_t)psp);
    putsUart0("\n\r");
    putsUart0("\tMSP:\t");
    print32h((uint32_t)msp);
    putsUart0("\n\r");
    putsUart0("\toff. inst.:\n\r\t\t");
    print32h(*((uint32_t*)pc));
    putsUart0("\n\r");

    putsUart0("\tprocess stack dump:\n\r");
    putsUart0("\txPSR:\t");
    print32h(xPSR);
    putsUart0("\n\r");
    putsUart0("\tPC:\t");
    print32h(pc);
    putsUart0("\n\r");
    putsUart0("\tLR:\t");
    print32h(lr);
    putsUart0("\n\r");
    putsUart0("\tR0:\t");
    print32h(r0);
    putsUart0("\n\r");
    putsUart0("\tR1:\t");
    print32h(r1);
    putsUart0("\n\r");
    putsUart0("\tR2:\t");
    print32h(r2);
    putsUart0("\n\r");
    putsUart0("\tR3:\t");
    print32h(r3);
    putsUart0("\n\r");
    putsUart0("\tR12:\t");
    print32h(r12);
    putsUart0("\n\r");

    // print fault status register
    uint32_t faultstat = NVIC_FAULT_STAT_R; // read in fault status register
    uint16_t ufaultstat = (faultstat & UFAULT_STAT_MASK) >> 16;
    uint8_t bfaultstat = (faultstat & BFAULT_STAT_MASK) >> 8;
    uint8_t mfaultstat = faultstat & MFAULT_STAT_MASK;

    putsUart0("\tfaulstat register:\n\r");
    putsUart0("\tu:\t");
    print16h(ufaultstat);
    putsUart0("\n\r");
    putsUart0("\tb:\t");
    print8h(bfaultstat);
    putsUart0("\n\r");
    putsUart0("\tm:\t");
    print8h(mfaultstat);
    putsUart0("\n\r");

    // print MPU memory address
    uint32_t mem_addr = NVIC_MM_ADDR_R;

    // check BFARV bit, make sure addr is valid, if not, print it out
    if (NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_MMARV) {

    } else {
        putsUart0("\tMMARV: m_addr not valid!\n\r");
    }
    putsUart0("\tm_addr:\t");
    print32h(mem_addr);
    putsUart0("\n\r");

#ifndef ENDLESS_PENDSV
    // skip current instruction, continue execution
    *(psp+6) = pc + 4;
#endif

    // clear mem pending bit
    NVIC_SYS_HND_CTRL_R &= ~NVIC_SYS_HND_CTRL_MEMP;

    // set pendsv bit
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}
void pendsvHandler() {
    putsUart0("\n\r ! PENDSV - PID:");
    print8d(__pid);
    putsUart0(" ! \n\r");

    // if the MPU DERR or IERR bits are set, clear them and display the message “called from MPU”
    if (NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_DERR) {
        NVIC_FAULT_STAT_R != NVIC_FAULT_STAT_DERR;
        putsUart0("\tcalled from MPU\n\r");
    }
    if (NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_IERR) {
        NVIC_FAULT_STAT_R != NVIC_FAULT_STAT_IERR;
        putsUart0("\tcalled from MPU\n\r");
    }

    // unset pendsv (set unpendsv)
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_UNPEND_SV;

#ifdef ENDLESS_PENDSV
    endlessLoop();
#endif
}
void nmiHandler() {}

void svcHandler() {
    setTmpl(0);                             // set TMPL back to 0, back to priv
}

// causers (read: troublemakers!)
void causeBusFault() {
    uint32_t* dont_exist = (uint32_t*)0x40014000;
    *dont_exist = 1;                        // attempts to address memory which wont respond
}
void causeHardFault() {
    callSVC(0);                             // call SVC handler
    NVIC_SYS_HND_CTRL_R &= ~NVIC_SYS_HND_CTRL_USAGE;    // disables usage fault trapping
    causeUsageFault();                      // cause usage fault, will not be handled
}
int causeUsageFault() {
    uint8_t not_zero = 0;                   // definitely DOESN'T try to cause a DIV0 fault
    uint8_t zero = 10;                      // if you are a compiler, look away!
    return zero / not_zero;                 // whoops!
}
void causeMPUFault() {
    uint32_t* outside_memory = (uint32_t*)0x30008000;
    *outside_memory = 1;                    // attempts to write to memory outside of MPU regions
}
void danceToIt() {
    uint32_t tracker = 0;
    uint8_t weeeee = 0;
    uint8_t mask = 0x1;
    while(1) {
        tracker++;
        if (tracker < 0x000AFFFF) {
            continue;
        } else {
            tracker = 0;
        }
        weeeee++;
        if (weeeee > 0xFF)
            weeeee = 0;
        if (weeeee & mask)
            togglePinValue(PORTA, GPIO_GREEN_LED);
        if (weeeee & (mask << 2))
            togglePinValue(PORTA, GPIO_YELLOW_LED);
        if (weeeee & (mask << 4))
            togglePinValue(PORTA, GPIO_ORANGE_LED);
        if (weeeee & (mask << 3))
            togglePinValue(PORTE, GPIO_RED_LED);
        if (weeeee & (mask << 1))
            togglePinValue(PORTF, GPIO_BLUE_LED);
    }
}

// output stuff
void printFaultAddr() {
    uint32_t fault_addr = NVIC_FAULT_ADDR_R;

    // check BFARV bit, make sure addr is valid, if so, print it out
    if (NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_BFARV) {

    } else {
        putsUart0("\tBFARV: f_addr not valid!\n\r");
    }
    putsUart0("\tf_addr:\t");
    print32h(fault_addr);
    putsUart0("\n\r");
}
void print32h(uint32_t x) {
    int i;
    uint32_t mask = 0x000000FF;             // 0000 0000 0000 0000 0000 0000 1111 1111
    uint8_t subsect = 0;
    for (i=3; i>=0; i--) {                  // iterate over 4 bytes
        subsect = mask & (x >> (i*8));      // shift x right by i*8, apply mask
        print8h(subsect);                   // print byte in hex
        putcUart0(' ');                     // print space between bytes
    }
}
void print32b(uint32_t x) {
    int i;
    uint32_t mask = 0x000000FF;             // 0000 0000 0000 0000 0000 0000 1111 1111
    uint8_t subsect = 0;

    for (i=3; i>=0; i--) {                  // iterate over 4 bytes
        subsect = mask & (x >> (i*8));      // apply mask after shifting to the right
        print8b(subsect);                   // print the byte out
        putcUart0(' ');                     // print out a space for separation
    }
}
void print16h(uint32_t x) {
    int i;
    uint32_t mask = 0x00FF;                 // 0000 0000 1111 1111
    uint8_t subsect = 0;
    for (i=1; i>=0; i--) {                  // iterate over 2 bytes
        subsect = mask & (x >> (i*8));      // shift x right by i*8, apply mask
        print8h(subsect);                   // print byte in hex
        putcUart0(' ');                     // print space between bytes
    }
}
void print8h(uint8_t x) {
    uint8_t mask = 0xF0;                    // 1111 0000
    uint8_t y = (x & mask) >> 4;            // get first nibble
    print4h(y);                             // print first nibble
    mask = mask >> 4;                       // move mask to get second nibble
    y = (x & mask);                         // get second nibble
    print4h(y);                             // print second nibble
}
void print8d(uint8_t x) {
    uint8_t y[3];                           // max value of 255, 3 digits
    int i = 0;
    while (x) {                             // while x is not zero
        y[i] = x % 10;                      // mod x, store in array
        x = x / 10;                         // new x, div by ten
        i++;
    }

    for (i=i-1; i>=0; i--) {                // i-1 is current number of digits
        putcUart0((char)(y[i] + 48));       // print in reverse order
    }
}
void print8b(uint8_t x) {
    int i;
    char c;
    uint8_t mask = 0x80;                    // 1000 0000
    for (i=0; i<8; i++) {                   // iterate over each bit
        c = '0';                            // assume it's a 0
        if (x & (mask >> i)) {              // test if it's a 1
            c = '1';                        // it is a 1
        }
        putcUart0(c);                       // output 0 or 1
    }
}
void print4h(uint8_t x) {
    char c;
    switch (x) {
    case 0:                                 // handle all numbers with same function
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        c = (char)(x + 48);                 // all ASCII numbers are +48 from dec value
        putcUart0(c);                       // print out char
        break;
    case 10:                                // alphas are handled separately
        putcUart0('A');
        break;
    case 11:
        putcUart0('B');
        break;
    case 12:
        putcUart0('C');
        break;
    case 13:
        putcUart0('D');
        break;
    case 14:
        putcUart0('E');
        break;
    case 15:
        putcUart0('F');
        break;
    }
}

// functions
int init() {
    // initialize hardware
    initSystemClockTo40Mhz();
    initUart0();

    // set UART0 baud rate
    setUart0BaudRate(UART0_BAUD_RATE, 40e6);

    // enable GPIO ports
    enablePort(PORTA);
    enablePort(PORTB);
    enablePort(PORTE);
    enablePort(PORTF);

    // set GPIO inputs
    selectPinDigitalInput(PORTB, GPIO_BUTTON_0);
    selectPinDigitalInput(PORTB, GPIO_BUTTON_1);
    selectPinDigitalInput(PORTB, GPIO_BUTTON_2);
    selectPinDigitalInput(PORTB, GPIO_BUTTON_3);
    selectPinDigitalInput(PORTB, GPIO_BUTTON_4);
    selectPinDigitalInput(PORTB, GPIO_BUTTON_5);
    enablePinPullup(PORTB, GPIO_BUTTON_0);
    enablePinPullup(PORTB, GPIO_BUTTON_1);
    enablePinPullup(PORTB, GPIO_BUTTON_2);
    enablePinPullup(PORTB, GPIO_BUTTON_3);
    enablePinPullup(PORTB, GPIO_BUTTON_4);
    enablePinPullup(PORTB, GPIO_BUTTON_5);

    // set GPIO outputs
    selectPinPushPullOutput(PORTA, GPIO_GREEN_LED);
    selectPinPushPullOutput(PORTA, GPIO_YELLOW_LED);
    selectPinPushPullOutput(PORTA, GPIO_ORANGE_LED);
    selectPinPushPullOutput(PORTE, GPIO_RED_LED);
    selectPinPushPullOutput(PORTF, GPIO_BLUE_LED);

    // enable faults
    NVIC_SYS_HND_CTRL_R |= NVIC_SYS_HND_CTRL_USAGE;
    NVIC_SYS_HND_CTRL_R |= NVIC_SYS_HND_CTRL_BUS;
    NVIC_SYS_HND_CTRL_R |= NVIC_SYS_HND_CTRL_MEM;
    NVIC_CFG_CTRL_R |= NVIC_CFG_CTRL_DIV0;

    // MPU enable background rule and enable
    NVIC_MPU_CTRL_R |= NVIC_MPU_CTRL_PRIVDEFEN | NVIC_MPU_CTRL_ENABLE;

    // exit, return success
    return 0;
}
int initMalloc() {
    int num_subregions = MALLOC_SUBREGION_NUM*MALLOC_REGION_NUM;
    int i;                                  // set up subregions
    for (i=0; i<4; i++) {
        heap_alloc[i].p = (void*)(MALLOC_START_ADDR + i*MALLOC_SUBREGION_SIZE);
        heap_alloc[i].is_alloc = true;      // set OS stack as allocated already
        heap_alloc[i].PID = 0;              // default PID will be 0
        heap_alloc[i].n = 4;                // set 4 sections allocated
    }
    for (i=4; i<num_subregions-1; i++) {    // iterate over every subregion
        heap_alloc[i].p = (void*)(MALLOC_START_ADDR + i*MALLOC_SUBREGION_SIZE);
        heap_alloc[i].is_alloc = false;     // allocate memory in sections of 8192, set is_alloc false
        heap_alloc[i].PID = 0;              // default PID will be 0
        heap_alloc[i].n = 0;                // default number of sections allocated is 0
    }
    heap_alloc[num_subregions-1].p = (void*)(MALLOC_START_ADDR + (num_subregions-1)*MALLOC_SUBREGION_SIZE);
    heap_alloc[num_subregions-1].is_alloc = true;
    heap_alloc[num_subregions-1].PID = 0;   // allocates space for stack...
    heap_alloc[num_subregions-1].n = 1;

    return 0;
}
void endlessLoop() {
    while(1) {}                             // trapping function for if we have a whoopsie
}
void yield() {};                            // empty function for later
void rebootuC() {                           // sets the bit in APINT register to reset entire microcontroller
    NVIC_APINT_R |= NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
}
void getInput() {
    uint8_t user_input = 0;
    uint8_t prev_input = 0;
    while (1) {
        user_input = getPortValue(PORTB);   // get user input at present
        user_input ^= 0xFF;                 // flip all bits
        user_input &= 0x3F;                 // remove first two bits, not used

        if (user_input == prev_input) {     // primitive debounce
            continue;                       // skip the rest of this loop
        }

        switch (user_input) {               // switch on user input
        case GPIO_BUTTON_0_MASK:
            causeBusFault();
            break;
        case GPIO_BUTTON_1_MASK:
            causeHardFault();
            break;
        case GPIO_BUTTON_2_MASK:
            causeUsageFault();
            break;
        case GPIO_BUTTON_3_MASK:
            causeMPUFault();
            break;
        case GPIO_BUTTON_4_MASK:
            // do nothing
            break;
        case GPIO_BUTTON_5_MASK:
            danceToIt();
            break;
        default:
            // do nothing
            break;
        }
        prev_input = user_input;            // primitive debounce
    }
}

// functions required in project
void* malloc_heap(int size_in_bytes) {
    // check if we are in priv mode
    int in_priv = getTmpl();

    // enter priv mode if not
    callSVC(0);

    // round up to next 1024B; this could be sped up using shifting...
    if (size_in_bytes % MALLOC_SUBREGION_SIZE == 0) {
        // do nothing, already a multiple of 1024
    } else {
        size_in_bytes = (size_in_bytes / MALLOC_SUBREGION_SIZE) + 1;
        size_in_bytes = size_in_bytes * MALLOC_SUBREGION_SIZE;
    }

    // check if we have space available
    int i, is_free = 0;
    int first_free_ID = -1;
    int num_subregions = MALLOC_SUBREGION_NUM*MALLOC_REGION_NUM;
    for (i=0; i<num_subregions; i++) {      // iterate over all subregions
        if (!heap_alloc[i].is_alloc) {      // is the subregion free?
            is_free++;                      // yes, add it to our counter
            if (first_free_ID == -1) {      // if this is the first free subregion
                first_free_ID = i;          // store it to save time later
            }
        }
    }
    if ((size_in_bytes / MALLOC_SUBREGION_SIZE) > is_free) {  // are we asking for more than we have available?
        return (void*)0;                    // return 0 pointer, we don't have enough space left!
    }
    if (first_free_ID == -1) {
        return (void*)0;                    // never found the first free space?
    }

    // set srdbits in regions to match what is required, update heap_alloc
    void* p = heap_alloc[first_free_ID].p;
    int num_of_subregions = size_in_bytes / MALLOC_SUBREGION_SIZE;
    for (i=first_free_ID; i<num_of_subregions+first_free_ID; i++) {
        heap_alloc[i].is_alloc = true;      // indicate that this memory region is allocated
        heap_alloc[i].PID = __pid;          // set PID to be the global (currently running PID)
        heap_alloc[i].n = num_of_subregions;// set number of subregions
    }

    addSramAccessWindow(p, size_in_bytes);   // set access window

    // leave priv mode, if we were not in priv mode
    setTmpl(in_priv);

    // return pointer to memory
    return p;
}
int free_heap(void* p) {
    // check if we are in priv mode
    int in_priv = getTmpl();

    // enter priv mode if not
    callSVC(0);

    uint8_t pid = __pid;                    // grab currently running PID (temporarily set to global variable)

    // find pointer in heap_alloc
    int i;
    int is_found = -1;
    int num_subregions = MALLOC_SUBREGION_NUM*MALLOC_REGION_NUM;
    for (i=0; i<num_subregions; i++) {
        if (heap_alloc[i].p == p) {         // iterate over all subregions, check if pointer matches
            is_found = i;                   // save index
            break;                          // break out of loop
        }
    }

    if (is_found == -1) {                   // did we find the pointer?
        return -1;                          // no, give up and go home
    }

    if (heap_alloc[is_found].PID != pid) {  // does the PID calling this own it?
        return -1;                          // return failure
    }

    int n_subregions = heap_alloc[is_found].n;
    for (i=is_found; i<n_subregions+is_found; i++) {  // iterate over all subregions for this alloc
        heap_alloc[i].is_alloc = false;
        heap_alloc[i].PID = 0;
        heap_alloc[i].n = 0;
    }

    // apply srdBitMask
    uint32_t srdBitMask = createSramAccessMask();   // generate new srdBitMask
    applySramAccessMask(srdBitMask);        // apply new mask

    // leave priv mode, if we were not in priv mode
    setTmpl(in_priv);

    return 0;                               // return success
}
void allowFlashAccess() {
    NVIC_MPU_NUMBER_R = MPU_REGNUM_FLASH;   // select MPU region for flash
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SIZE_M & (MPU_REGSIZE_FLASH << 1); // set size
    NVIC_MPU_BASE_R |= NVIC_MPU_BASE_ADDR_M & MPU_REGADR_FLASH;         // set base address
    NVIC_MPU_ATTR_R |= MPU_AP_OSRWURO;      // set OS: +R+W+X   user: +R+X
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_ENABLE;// enable region
}
void allowPeripheralAccess() {
    NVIC_MPU_NUMBER_R = MPU_REGNUM_PERI;  // select MPU region for peripherals
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SIZE_M & (MPU_REGSIZE_PERI << 1);// set size
    NVIC_MPU_BASE_R |= NVIC_MPU_BASE_ADDR_M & MPU_REGADR_PERI;        // set base address
    NVIC_MPU_ATTR_R |= MPU_AP_OSRWURW;      // set OS: +R+W     user: +R+W
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN;    // disable executable
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_ENABLE;// enable region
}
void setupSramAccess() {
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
}
uint32_t createNoSramAccessMask() {
    return (uint32_t)0xFFFFFFFF;            // not sure why this was requested to be 64b ???
}
uint32_t createSramAccessMask() {
    uint32_t one;
    uint32_t srdBitMask = 0;                // calcuate new srdBitMask
    int num_subregions = MALLOC_REGION_NUM*MALLOC_SUBREGION_NUM;
    int i;
    for (i=0; i<num_subregions; i++) {      // iterate over each subregion
        if (heap_alloc[i].is_alloc) {       // is this block allocated?
            one = 1 << i;                   // shift 1 into right spot
            srdBitMask |= one;              // add result to srdBitMask
        }
    }

    srdBitMask = ~srdBitMask;               // flip result
    return srdBitMask;                      // return result
}
void applySramAccessMask(uint32_t srdBitMask) {
    uint32_t attr_state = 0;

    NVIC_MPU_NUMBER_R = MPU_REGNUM_SRAM_3;  // set region to SRAM3
    attr_state = NVIC_MPU_ATTR_R;           // get current state
    attr_state &= ~NVIC_MPU_ATTR_SRD_M;     // remove SRD bits from state
    attr_state |= NVIC_MPU_ATTR_SRD_M & (srdBitMask >> 16); // write bit mask
    NVIC_MPU_ATTR_R = attr_state;           // write all at once

    NVIC_MPU_NUMBER_R = MPU_REGNUM_SRAM_2;  // set region to SRAM2
    attr_state = NVIC_MPU_ATTR_R;           // get current state
    attr_state &= ~NVIC_MPU_ATTR_SRD_M;     // remove SRD bits from state
    attr_state |= NVIC_MPU_ATTR_SRD_M & (srdBitMask >> 8);  // write bit mask
    NVIC_MPU_ATTR_R = attr_state;           // write all at once

    NVIC_MPU_NUMBER_R = MPU_REGNUM_SRAM_1;  // set region to SRAM1
    attr_state = NVIC_MPU_ATTR_R;           // get current state
    attr_state &= ~NVIC_MPU_ATTR_SRD_M;     // remove SRD bits from state
    attr_state |= NVIC_MPU_ATTR_SRD_M & srdBitMask;         // write bit mask
    NVIC_MPU_ATTR_R = attr_state;           // write all at once

    NVIC_MPU_NUMBER_R = MPU_REGNUM_SRAM_0;  // set region to SRAM0
    attr_state = NVIC_MPU_ATTR_R;           // get current state
    attr_state &= ~NVIC_MPU_ATTR_SRD_M;     // remove SRD bits from state
    attr_state |= NVIC_MPU_ATTR_SRD_M & (srdBitMask << 8);  // write bit mask
    NVIC_MPU_ATTR_R = attr_state;           // write all at once
}
void addSramAccessWindow(uint32_t* baseAdd, uint32_t size_in_bytes) {
    if ((uint32_t)baseAdd < 0x20000000) {   // check if baseAdd is within range
        putsUart0(" !!! base address out of range\n\r");
        return;
    }
    if (((uint32_t)baseAdd)+size_in_bytes > 0x20008000) {   // check if baseAdd is within range
        putsUart0(" !!! size requested will not fit\n\r");
        return;
    }

    // this is only called after heap_alloc is updated, so we can just generate srdBitMask from it...
    uint32_t srdBitMask = createSramAccessMask();           // generate new srdBitMask from heap_alloc
    applySramAccessMask(srdBitMask);                        // apply result
}
void miniProjectTest() {
    putsUart0("executing miniproject test routine\n\r");

/*
    putsUart0(" > setting flash to RWX for user and OS\n\r");
    allowFlashAccess();                     // set flash region to RWX for user and OS

    putsUart0("\tgoing into unpriv. mode\n\r");
    setTmpl(1);                             // switch to unpriv mode

    putsUart0("\tattempting to read from flash as unpriv.\n\r");
    uint32_t test = *((uint32_t*)0x1FFFFFFF);   // attempt to read flash
    putsUart0("\t\tread ");
    print32h(test);
    putsUart0(" at 0x1FFFFFFF\n\r");

    putsUart0("\tswitching back to priv. mode\n\r");
    callSVC(0);                             // switch back to priv mode

    putsUart0(" > setting peripherals to RW for user and OS\n\r");
    allowPeripheralAccess();                // enable peripheral access

    putsUart0("\tgoing into unpriv. mode\n\r");
    setTmpl(1);                             // switch to unpriv mode

    putsUart0("\tattempting to access a peripheral\n\r");
    setPinValue(PORTA, GPIO_GREEN_LED, 1);  // attempt to write to peripheral
    setPinValue(PORTA, GPIO_GREEN_LED, 0);

    putsUart0("\tswitching back to priv. mode\n\r");
    callSVC(0);                             // switch back to priv mode

    putsUart0(" > setting up regions and subregions on SRAM\n\r");
    setupSramAccess();                      // set up 4 regions x 8 subregions across SRAM

    putsUart0("\tcreating srd bit mask (32b instead of 64b)...\n\r");
    uint32_t srdBitMask = createNoSramAccessMask(); // create mask for no SRAM access

    putsUart0("\tapplying srd bit mask to all 4 regions of SRAM\n\r");
    applySramAccessMask(srdBitMask);        // apply mask to all 4 regions

    putsUart0("\tadding SRAM window from 0x20000000 to 0x20008000\n\r");
    // should this have the uint64_t* or not? instructions conflict with themselves...
    addSramAccessWindow(unit32_t *)0x20000000, 32768);

    putsUart0("\tattempting to access SRAM while in priv. mode\n\r");
    test = *((uint32_t*)0x20001FFF);
    putsUart0("\t\tread ");
    print32h(test);
    putsUart0(" at 0x20001FFF\n\r");

    putsUart0("\tgoing into unpriv. mode\n\r");
    setTmpl(1);                             // switch to unpriv mode

    putsUart0("\tattempting to access SRAM while in unpriv. mode\n\r");
    test = *((uint32_t*)0x20001FFF);
    putsUart0("\t\tread ");
    print32h(test);
    putsUart0(" at 0x20001FFF\n\r");

    putsUart0("\tswitching back to priv. mode\n\r");
    callSVC(0);                             // switch back to priv mode
*/
    putsUart0(" > SRAM access test (restricted)\n\r");

    putsUart0("\tsetting up access as requested...\n\r");
    allowFlashAccess();                     // set up MPU region for flash
    allowPeripheralAccess();                // set up MPU region for peripherals
    setupSramAccess();                      // set up MPU regions for SRAM
    uint32_t srdBitMask = createNoSramAccessMask(); // create a no-access srdBitMask
    applySramAccessMask(srdBitMask);        // apply the srdBitMask to the SRAM regions
    uint32_t* p = malloc_heap(1024);        // malloc space for a byte

    putsUart0("\twriting 0xF1 F1 F0 F0 to address 'p' = 0x");
    print32h((uint32_t)p);
    putsUart0("\n\r");
    *p = 0xF1F1F0F0;

    putsUart0("\treading 1 byte from SRAM\n\r");
    print8h(*p);                            // print out byte
    putsUart0("\n\r");

    putsUart0("\tgoing into unpriv. mode\n\r");
    setTmpl(1);                             // switch to unpriv mode

    putsUart0("\tattempting to read from flash\n\r");
    uint32_t* p_f = (uint32_t*)0x00030000;
    uint32_t test32 = *p_f;                 // attempt to read from flash
    putsUart0("\t\t*p_f = 0x");
    print32h(test32);
    putsUart0("\n\r");

    putsUart0("\tattempting to access a peripheral\n\r");
    setPinValue(PORTA, GPIO_GREEN_LED, 1);  // attempt to write to peripheral

    putsUart0("\tattempting to free 'p'\n\r");
    int result = free_heap(p);
    if (result == -1) {
        putsUart0(" !!! unable to free 'p'\n\r");
    }

    putsUart0("\tattempting to write to 'p'\n\r");
    *p = 0x0F0F1F1F;

    putsUart0("\tswitching back to priv. mode\n\r");
    callSVC(0);                             // switch back to priv mode

    putsUart0("\tattempting to read SRAM in priv. mode\n\r");
    test32 = *p;                            // attempt to read 0xF1F1F0F0 back
    putsUart0("\t\t'p' = 0x");
    print32h((uint32_t)p);
    putsUart0("\n\r");
    putsUart0("\t\t'*p' = 0x");
    print32h(test32);
    putsUart0("\n\r");

    putsUart0("\tgoing into unpriv. mode\n\r");
    setTmpl(1);                             // switch to unpriv mode

    putsUart0("\tattempting to read from flash, again\n\r");
    p_f = (uint32_t*)0x00030000;
    test32 = *p_f;                          // attempt to read flash, again?
    putsUart0("\t\t*p_f = 0x");
    print32h(test32);
    putsUart0("\n\r");

    putsUart0("\tattempting to access a peripheral\n\r");
    setPinValue(PORTA, GPIO_GREEN_LED, 0);  // attempt to write to peripheral

    putsUart0("\tattempting to read from wrong SRAM region in unpriv. mode\n\r");
    test32 = *((uint32_t*)0x20001FFF);
    putsUart0("\t\tread 0x");
    print32h(test32);
    putsUart0(" at 0x20 00 1F FF\n\r");

    putsUart0(" > tests all passed!\n\rnow starting button handler...\n\r");
}

// main
int main(void)
{
    // initial setup
    init();                                 // initialize hardware and GPIO and UART0
    initMalloc();                           // initialize malloc array
    setPsp((uint32_t*)0x20008000);          // sets up PSP to 0x20008000, top of SRAM stack
    setAsp();                               // set ASP bit to 1 in CONTROL register

    // functions required for project are in this function:
    miniProjectTest();                     // allow flash access, allow peripheral access, set up SRAM access, etc.

    // get user input from buttons
    getInput();

    // exit, return 0
    return 0;
}

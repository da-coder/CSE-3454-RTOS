// Shell functions
// J Losh

#include "tm4c123gh6pm.h"
#include "asm.h"
#include "uart0.h"
#include "kernel.h"

#include "shell.h"

// structs
typedef struct {
    uint8_t n_arg;                                      // number of arguments
    uint8_t t_arg;                                      // type of argument; 0=n/a, 1=int, 2=string
    char token[MAX_TOKEN];                              // token string, max characters
    char desc[MAX_CHARS];                               // description
    _fn fp;                                             // function pointer
} COMMAND;

// commands in form of: number of arguments, command string, help text, function pointer
const COMMAND commands[NUM_OF_CMDS] = {
    {0, 0,  "reboot",   "reboot\t...\t...\treboots uC",                         rebootuC},
    {0, 0,  "ps",       "ps\t...\t...\tlist processes",                         listProcess},
    {0, 0,  "ipcs",     "ipcs\t...\t...\tlist inter-process comms",             listInterProcess},
    {1, 1,  "kill",     "kill <PID>\t...\tkills process via PID",               (_fn)killProcessByPID},
    {1, 2,  "pkill",    "pkill <proc_name>\tkills process via proc_name",       (_fn)killProcessByName},
    {1, 2,  "pi",       "pi <on/off>\t...\tsets priority inheritance on/off",   (_fn)setPrioInherit},
    {1, 2,  "preempt",  "preempt <on/off>\tsets preemption on/off",             (_fn)setPreempt},
    {1, 2,  "sched",    "sched <prio/rr>\t...\tsets scheduling method prio/rr", (_fn)setScheduling},
    {1, 2,  "pidof",    "pidof <proc_name>\tgets PID via proc_name",            (_fn)getProcess},
    {1, 2,  "run",      "run <proc_name>\t...\tstarts process via proc_name",   (_fn)runProcess},
    {0, 0,  "?",        "?\t...\t...\tlists all commands",                      printCmds}
};

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void shell(void) {
    int cmd;
    char line[MAX_CHARS];
    char* argu;
    bool command_is_good;

    putsUart0("\n\rSeanSoft (c) 2025\n\rCSE4354 RTOS\n\rshell starting up...\n\r\n\r");
    putsUart0("\t\thelp menu --> ?\t\t\t<(^v^<) pengu\n\r");

    while (true) {
        // request user input
        command_is_good = true;
        putsUart0("\n\r(>^v^)> ");

        // wait for / get user input
        if (getsUart0(line)) {
            putsUart0("\n\r ! unable to get user input\n\r");
            continue;
        }

        // check user input
        if (getLength(line) < MIN_CHARS) {
            continue;                       // skip to next user input request
        }

        // decode command
        cmd = decodeCommand(line);
        if (cmd == -1) {
            putsUart0("\n\r<(?v?<) unknown command\n\r\tavailable commands --> ?\n\r");
            continue;                       // skip to next user input request
        }

        // get argument
        argu = getArgu(line);               // extract argument, if any

        // check if argument specified
        if (argu != (char*)0) {             // was an argument specified?
            if (!commands[cmd].n_arg) {     // yes, was it supposed to be?
                command_is_good = false;    // no, bad command
            }
        } else {
            if (commands[cmd].n_arg) {      // no, was it supposed to be?
                command_is_good = false;    // yes, bad command
            }
        }

        // check argument types
        command_is_good &= (commands[cmd].t_arg == isNumeric(argu));

        // is our command good to run?
        if (!command_is_good) {
            putsUart0("\n\r<(*v*<) ");
            printHelp(cmd);                 // print help for user
            continue;                       // no good, skip to next input
        }

        // call the function with arguments specified
        (commands[cmd].fp)();
    }
}

// generic shell helper functions
int decodeCommand(char* line) {             // decodes command; -1 = none found, X = index in CMDs of command
    int i=0;
    for (i=0; i<NUM_OF_CMDS; i++) {         // iterate over commands
        if (compareStr(commands[i].token, line)) {
            return i;                       // break out of loop, we found our man.
        }
    }

    return -1;                              // return -1 on fail
}
void printHelp(uint8_t which) {
    putsUart0((char*)commands[which].desc);
}
void listProcess() {
    int i;
    char state[NUM_STATE][MAX_STATE] = {"invalid", "unrun", "ready", "delayad", "blkd[s]", "blkd[m]", "killed"};

    putsUart0("\n\r<(*v*<) currently running tasks:\n\r\tPID:\t\tNAME:\t\tSTATE:\n\r");
    for (i=0; i<MAX_TASKS; i++) {
        putsUart0("\t ");
        print32h((uint32_t)tcb[i].pid);
        putsUart0("\t ");
        putsUart0(tcb[i].name);
        printBlanks(16 - getLength(tcb[i].name));
        print8d(tcb[i].state);
        putsUart0(" [");
        putsUart0(state[tcb[i].state]);
        putsUart0("]\n\r");
    }
}
void listInterProcess() {}
void killProcessByPID(uint8_t pid) {}
void killProcessByName(char* p_name) {}
void setPrioInherit(uint8_t value) {}
void setPreempt(uint8_t value) {}
void setScheduling(uint8_t choice) {}
void getProcess(char* p_name) {}
void runProcess(char* p_name) {}
void printCmds() {
    int i;
    for(i=0; i<NUM_OF_CMDS; i++) {
        putsUart0("\n\r\t");
        putsUart0((char*)commands[i].desc);
    }
}

uint8_t __pid = 69;                         // nice; temporary global variable

// prints fault messages based on type of fault; all my putsUart0 code will be here for ease of access
// 0: MPU fault         1: hard fault       2: bus fault        3: usage fault
void printFaultMessage(uint8_t which) {
    switch (which) {                        // which switch? switch which.
    case 0:
        putsUart0("\n\r ! MPU FAULT - PID:");
        print8d(__pid);                     // print out PID
        putsUart0(" ! \n\r");
        break;
    case 1:
        putsUart0("\n\r ! HARD FAULT - PID:");
        print8d(__pid);
        putsUart0(" ! \n\r");
        break;
    case 2:
        putsUart0("\n\r ! BUS FAULT - PID:");
        print8d(__pid);
        putsUart0(" ! \n\r");
        break;
    case 3:
        putsUart0("\n\r ! USAGE FAULT - PID:");
        print8d(__pid);
        putsUart0(" ! \n\r");
        break;
    default:
        break;
    }
}

// dumps core registers during faults, for cleanliness
void printCoreRegisters() {
    uint32_t* psp = getPsp();               // get program stack pointer
    uint8_t is_unpriv = getTmpl();          // get if we are privileged or not

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
    putsUart0("\n\r\tMSP:\t");
    print32h((uint32_t)msp);
    putsUart0("\n\r");
    if (is_unpriv) {
        putsUart0("\tnot privileged!\n\r\t\tunable to print instruction\n\r");
    } else {
        putsUart0("\toff. inst.:\n\r\t\t");
        print32h(*((uint32_t*)pc));
        putsUart0("\n\r");
    }

    putsUart0("\tprocess stack dump:\n\r\txPSR:\t");
    print32h(xPSR);
    putsUart0("\n\r\tPC:\t");
    print32h(pc);
    putsUart0("\n\r\tLR:\t");
    print32h(lr);
    putsUart0("\n\r\tR0:\t");
    print32h(r0);
    putsUart0("\n\r\tR1:\t");
    print32h(r1);
    putsUart0("\n\r\tR2:\t");
    print32h(r2);
    putsUart0("\n\r\tR3:\t");
    print32h(r3);
    putsUart0("\n\r\tR12:\t");
    print32h(r12);
    putsUart0("\n\r");
}

// print fault stat register
void printFaultStatRegister() {
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
}

// print hfault status register
void printHFaultStatRegister() {
    uint32_t hfaultstat = NVIC_HFAULT_STAT_R; // read in hfault status register

    putsUart0("\thfaulstat register:\n\r\t\t");
    print16h(hfaultstat);
    putsUart0("\n\r");
}

// print MPU fault address, if valid
void printMPUFaultAddress() {
    uint32_t mem_addr = NVIC_MM_ADDR_R;

    // check BFARV bit, make sure addr is valid, if not, print it out
    if (NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_MMARV) {
        putsUart0("\tm_addr:\t");
        print32h(mem_addr);
        putsUart0("\n\r");
    } else {
        putsUart0("\tMMARV: m_addr not valid!\n\r");
    }
}

// print bus fault address
void printBusFaultAddress() {
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

// general-use print functions
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
    if (x == 0) {                           // special case, trap for 0
        putcUart0('0');
        return;
    }

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
void printBlanks(uint8_t number_of) {
    while (number_of > 0) {
        putcUart0(' ');
        number_of--;
    }
}

// general-use string functions
int getLength(char* str) {                  // returns length of string, not including terminating char
    int i = 0;
    while (*str != '\0') {
        str++;                              // next char
        i++;                                // increment

        if (i > MAX_CHARS-1) {              // did we overflow?
            i = -1;                         // signal and exit
            break;
        }
    }

    return i;
}
int compareStr(const char* key, char* tok) {// case insensitive string compare (alphanumeric)
    while(*key != '\0') {                   // iterate over key until we hit end
        if (*key != *tok) {                 // check if they are exactly the same
            if (*tok > (char)65 && *tok < (char)90) {   // is it a capital letter
                if (*key != (char)(*tok+32)) {          // check ignoring case
                    return 0;               // not matching lower or upper, return failure
                }
            } else {
                return 0;                   // not a capital letter, so return failure
            }
        }

        key++;                              // next char
        tok++;
    }

    if (*tok == '\0' || *tok == ' ') {      // check if token is at end as well
        return 1;                           // token is at the end, return success
    }

    return 0;                               // substring, return failure
}
char* getArgu(char* line) {                 // get pointer to start of arguments, return 0 if failure
    int i=0;
    while(*line != '\0') {                  // iterate over line
        if (*line == ' ') {                 // find the first space
            line++;
            return line;                    // return the next character after space
        }
        line++;                             // next char, please
        i++;                                // keep track of number of chars
        if (i>MAX_CHARS) {
            break;                          // too many chars, break out of loop
        }
    }

    return 0;                               // failed to find space, or overflow; return failure
}
int isNumeric(char* line) {                 // stripped down atoi which only tells us if it's numeric
    if (line == (char*)0) {
        return 0;                           // return failure
    }

    while (*line != '\0') {
        if (*line > 57 || *line < 48) {     // is character outside of the numeric ASCII range?
            return 0;                       // return failure
        }
        line++;
    }

    return 1;                               // return success
}

// get user input
int getsUart0(char* buffer) {
    int count = 0;                          // init local variable for count set to 0

    while (true) {
        if (kbhitUart0()) {                 // any characters in receive buffer?
            yield();                        // no, yield our time
        }

        char a = getcUart0();               // get a character from uart
        switch (a) {                        // switch:case superior to if:else for char (e.g. 127 is also >= 32)
            case 8:                         // if char is backspace
            case 127:                       // if char is backspace (127)
                if (count > 0) {
                    count--;                // if count > 0; decrement count
                } else {
                    continue;               // avoid echoing and saving to buffer
                }
                break;
            case 13:                        // if char is return
                buffer[count] = '\0';       // add null to end
                return 0;                   // return success
            default:
                if (a >= 32) {              // is char printable?
                    buffer[count] = a;      // save char in buffer
                    count++;                // increment count
                } else {
                    continue;               // skip echo
                }
                if (count == MAX_CHARS-1) { // is count == max chars-1?
                    buffer[count] = '\0';   // add null to end
                    return 0;               // return success
                }
                break;
        }
        #ifdef ECHO
            putcUart0(a);                   // if ECHO is enabled, echo back
        #endif
    }
}

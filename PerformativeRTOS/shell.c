// Shell functions
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
#include "shell.h"

// student added includes
#include <stdbool.h>
#include "uart0.h"
#include "kernel.h"
#include "mm.h"

// student added structs
typedef struct {
    uint8_t t_arg;                  // type of argument out; 0=none, 1=int, 2=string
    char token[MAX_TOKEN];          // token string, max characters
    char desc[MAX_CHARS];           // description
    _fn fp;                         // function pointer
} COMMAND;
const COMMAND commands[NUM_OF_CMDS] = {
    {0, "reboot",   "reboot\t...\t...\treboots uC",                         requestRebootuC},
    {0, "ps",       "ps\t...\t...\tlist processes",                         listProcess},
    {0, "ipcs",     "ipcs\t...\t...\tlist inter-process comms",             listInterProcess},
    {1, "kill",     "kill <PID>\t...\tkills process via PID",               (_fn)killProcessByPID},
    {2, "pkill",    "pkill <proc_name>\tkills process via proc_name",       killProcessByName},
    {2, "pi",       "pi <on/off>\t...\tsets priority inheritance on/off",   (_fn)setPrioInherit},
    {2, "preempt",  "preempt <on/off>\tsets preemption on/off",             (_fn)setPreempt},
    {2, "sched",    "sched <prio/rr>\t...\tsets scheduling method prio/rr", (_fn)setScheduling},
    {2, "pidof",    "pidof <proc_name>\tgets PID via proc_name",            getProcess},
    {2, "run",      "run <proc_name>\t...\tstarts process via proc_name",   runProcess},
    {0, "?",        "?\t...\t...\tlists all commands",                      printCmds}
};

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void shell(void) {
    int cmd;
    char line[MAX_CHARS];
    char* argu;
    uint8_t command_is_good;
    int argu_int;

    putsUart0("\n\r\n\rSeanSoft (c) 2025\n\rCSE4354 RTOS\n\r");
    putsUart0("\t\thelp menu --> ?\t\t\t<(^v^)> ask pengu!");

    while (true) {
        // request user input
        command_is_good = true;
        printPengu(0);

        // wait for / get user input
        if (getsUart0(line)) {
            printPengu(4);
            putsUart0("unable to get user input\n\r");
            continue;
        }

        // check user input
        if (getLength(line) < MIN_CHARS) {
            continue;                       // skip to next user input request
        }

        // decode command
        cmd = decodeCommand(line);
        if (cmd == -1) {
            printPengu(4);
            putsUart0("unknown command\n\r\tavailable commands --> ?");
            continue;                       // skip to next user input request
        }

        // get argument
        argu = getArgu(line);               // extract argument, if any

        // does this command require an argument?
        if (commands[cmd].t_arg != 0) {
            // was there no argument specified?
            if (argu == (char*)0) {
                command_is_good = 0;        // no argu given when required, mark command as bad
            }
        }

        // do we need a numerical argument?
        if (commands[cmd].t_arg == 1) {
            argu_int = isNumeric(argu);     // get numerical value
            // check if argument is numerical
            if (argu_int == -1) {
                command_is_good = 0;        // argument needed to be a number, was not, mark as bad
            }
        }

        // is our command good to run?
        if (!command_is_good) {
            printPengu(2);
            printHelp(cmd);                 // print help for user
            continue;                       // no good, skip to next input
        }

        // call the function with correct outgoing arguments
        if (commands[cmd].t_arg == 1) {
            (commands[cmd].fp)(argu_int);   // run with int argument
            continue;                       // next command
        }
        (commands[cmd].fp)(argu);           // run with string argument or no argument
    }
}

// call-able functions
void requestRebootuC() {
    // save anything?
    printPengu(4);
    putsUart0("system will reboot...");     // inform user of pending reboot
    sleep(3);                               // wait for message to be printed out...

    rebootuC();                             // rebootuC function from kernel.c
}
void listProcess() {
    int i;
    char state[NUM_STATE][MAX_STATE] = {"invalid", "unrun", "ready", "delayed", "blkd[s]", "blkd[m]", "killed"};
    p_data privData;                        // create a copy of privileged data on local stack

    // process data for average time
    requestData(0, &privData);              // request data from kernel

    // output to uart
    printPengu(1);
    putsUart0("current state:\tsched: ");
    if (privData.ps) putsUart0("prio\t"); else putsUart0("rr\t");
    putsUart0("preempt: ");
    if (privData.pr) putsUart0("on\t"); else putsUart0("off\t");
    putsUart0("pi: ");
    if (privData.pi) putsUart0("on\t"); else putsUart0("off\t");
    putsUart0("  avg_pendsv_t = ");
    print32d((uint32_t)privData.pq[2]);
    putsUart0("us\n\r\tPID:\t\tNAME:\t\tSTATE:\t\tPRIORITY:");
    if (privData.pi) {
        putsUart0("\tCURR_PRIOR:");
    }
    putsUart0("\tCPU_t:");
    for (i=0; i<privData.tc; i++) {
        requestData(i, &privData);          // request data, wastes one read cycle (reads 0 twice!)

        putsUart0("\n\r\t ");
        print32d((uint32_t)privData.pid);
        putsUart0("\t\t ");
        putsUart0(privData.n);
        printBlanks(16 - getLength(privData.n));
        print8d(privData.s);
        putsUart0(" [");
        putsUart0(state[privData.s]);
        putsUart0("]\t ");
        print8d(privData.p);
        if (privData.pi) {
            putsUart0("\t\t ");
            print8d(privData.cp);
        }
        putsUart0("\t\t ");
        printTime((uint32_t)privData.pq[0], (uint32_t)privData.pq[1]);
    }
}
void listInterProcess() {
    int i, j;

    p_data privData;                        // create local copy of priv data struct

    // print out title
    printPengu(1);
    putsUart0("current state of inter-process comms:\n\r");

    // print mutexes
    putsUart0("\tMUTEX_n:\tLOCKED:\t\tLOCKED_BY:\tQ_SIZE:\t\tQUEUE:");
    for (i=0; i<MAX_MUTEXES; i++) {
        requestData(i+MAX_TASKS, &privData);// request priv information from kernel

        putsUart0("\n\r\t ");
        print8d(i);
        putsUart0("\t\t ");
        if (privData.lc) putsUart0("true\t\t "); else putsUart0("false\t\t ");
        if (privData.lc) print32d((uint32_t)privData.pid); else putsUart0("\t\t");
        putsUart0("\t\t ");
        print8d(privData.qs);
        putsUart0("\t\t ");
        for (j=0; j<privData.qs; j++) {
            print32d((uint32_t)privData.pq[j]);
            if (j+1 < privData.qs) putsUart0(", ");
        }
    }

    // print semaphores
    putsUart0("\n\r\tSEMAPHORE_n:\tCOUNT:\t\t\t\tQ_SIZE:\t\tQUEUE:");
    for (i=0; i<MAX_SEMAPHORES; i++) {
        requestData(i+MAX_TASKS+MAX_MUTEXES, &privData);

        putsUart0("\n\r\t ");
        print8d(i);
        putsUart0("\t\t ");
        print8d(privData.lc);
        putsUart0("\t\t\t\t ");
        print8d(privData.qs);
        putsUart0("\t\t ");
        for (j=0; j<privData.qs; j++) {
            print32d((uint32_t)privData.pq[j]);
            if (j+1 < privData.qs) putsUart0(", ");
        }
    }
}
void killProcessByPID(uint32_t pid) {
    uint8_t i;
    p_data privData;

    // iterate over all tasks
    for (i=0; i<MAX_TASKS; i++) {
        requestData(i, &privData);

        // check if task exists
        if (privData.pid == (void*)pid) {
            // signal kill to kernel (call killThread)
            killThread((_fn)pid);
            printPengu(3);
            putsUart0("killed pid [");
            print32d((uint32_t)pid);
            putsUart0("] \"");
            putsUart0(privData.n);
            putsUart0("\" just as you instructed...");

            return;
        }
    }
    printPengu(2);
    putsUart0("unable to find task");
}
void killProcessByName(char* p_name) {
    uint8_t i;
    p_data privData;

    // iterate over every task
    for (i=0; i<MAX_TASKS; i++) {
        requestData(i, &privData);

        // compare p_name with name
        if (compareStr(p_name, privData.n)) {
            // signal kill to kernel (call killThread)
            killThread((_fn)privData.pid);
            printPengu(3);
            putsUart0("killed pid_name \"");
            putsUart0(privData.n);
            putsUart0("\" [");
            print32d((uint32_t)privData.pid);
            putsUart0("] just as you instructed...");

            return;
        }
    }
    printPengu(2);
    putsUart0("unable to find task");
}
void setPrioInherit(char* argu) {
    p_data privData;

    // check argument for correct values
    if (!(compareStr(argu, "on") || compareStr(argu, "off"))) {
        // print help for user and return
        printUnknownArgu(argu);
        printHelp(5);
        return;
    }

    // handle data
    requestData(0, &privData);
    if (compareStr(argu, "on")) {
        privData.pi = true;
    } else {
        privData.pi = false;
    }
    offerData(0, &privData);

    // print results for user
    printPengu(1);
    putsUart0("priority inheritance is now [");
    putsUart0(argu);
    putsUart0("]");
}
void setPreempt(char* argu) {
    p_data privData;

    if (!(compareStr(argu, "on") || compareStr(argu, "off"))) {
        printUnknownArgu(argu);
        printHelp(6);
        return;
    }

    requestData(0, &privData);
    if (compareStr(argu, "on")) {
        privData.pr = true;
    } else {
        privData.pr = false;
    }
    offerData(0, &privData);

    printPengu(1);
    putsUart0("preemption is now [");
    putsUart0(argu);
    putsUart0("]");
}
void setScheduling(char* argu) {
    p_data privData;

    if (!(compareStr(argu, "prio") || compareStr(argu, "rr"))) {
        printUnknownArgu(argu);
        printHelp(7);
        return;
    }

    requestData(0, &privData);
    if (compareStr(argu, "prio")) {
        privData.ps = true;
    } else {
        privData.ps = false;
    }
    offerData(0, &privData);

    printPengu(1);
    putsUart0("scheduling is now [");
    putsUart0(argu);
    putsUart0("]");
}
void getProcess(char* p_name) {
    int i;
    p_data privData;

    // iterate over every task
    for (i=0; i<MAX_TASKS; i++) {
        requestData(i, &privData);

        // compare function p_name with name
        if (compareStr(p_name, privData.n)) {
            // set task to unrun via kernel.c function
            printPengu(1);
            putsUart0("process PID: ");
            print32d((uint32_t)privData.pid);

            return;
        }
    }
    printPengu(2);
    putsUart0("unable to find task");
}
void runProcess(char* p_name) {
    int i;
    p_data privData;

    // iterate over every task
    for (i=0; i<MAX_TASKS; i++) {
        requestData(i, &privData);

        // compare function p_name with name
        if (compareStr(p_name, privData.n)) {
            // set task to unrun via kernel.c function
            restartThread((_fn)privData.pid);
            printPengu(1);
            putsUart0("started process \"");
            putsUart0(privData.n);
            putsUart0("\"");

            return;
        }
    }
    printPengu(2);
    putsUart0("unable to find task");
}
void printCmds() {
    int i;

    printPengu(1);
    putsUart0("possible commands, and their arguments, are:");
    for(i=0; i<NUM_OF_CMDS; i++) {
        putsUart0("\n\r\t");
        putsUart0((char*)commands[i].desc);
    }
}

// generic shell helper functions
void printTime(uint32_t t_time, uint32_t total_time) {
    uint32_t percent_time;

    // check for div/0
    if (total_time == 0) {
        putsUart0("not run yet...");
        return;
    }

    // calculate percentage
    percent_time = (t_time * 100) / total_time;

    // print it out
    print32d(percent_time);
    putsUart0("%");
}
int decodeCommand(char* line) {             // decodes command; -1 = none found, X = index in CMDs of command
    int i=0;
    for (i=0; i<NUM_OF_CMDS; i++) {         // iterate over commands
        if (compareStr(commands[i].token, line)) {
            return i;                       // break out of loop, we found our man.
        }
    }

    return -1;                              // return -1 on fail
}
void printHelp(uint8_t which) {             // just prints out the help string of command
    putsUart0((char*)commands[which].desc);
}
void printUnknownArgu(char* argu) {         // informs user that argument specified is unknown
    printPengu(4);
    putsUart0("unknown argument specified: \"");
    putsUart0(argu);
    putsUart0("\"\n\r");
}
void printPengu(uint8_t which) {            // prints out a helpful, cute penguin; 0=asking for input, 1=handing out info, 2=error, 3=spooky, 4=confused, default=forbidden
    switch(which) {
    case 0:
        putsUart0("\n\r<(^v^)> ");
        break;
    case 1:
        putsUart0("\n\r(7'v')7 ");
        break;
    case 2:
        putsUart0("\n\r(>`v`)> ");
        break;
    case 3:
        putsUart0("\n\r(>*v*)>/` ");
        break;
    case 4:
        putsUart0("\n\r(>*v*<) ");
        break;
    default:
        putsUart0("\n\r<(.^.)> ");
        break;
    }
}

// general-use print functions
void print32d(uint32_t x) {
    if (x == 0) {                           // special case, trap for 0
        putcUart0('0');
        return;
    }

    uint8_t y[10];                          // max of 10 digits
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
int isNumeric(char* line) {                 // stripped down atoi
    int val = 0;
    int pow = 1;
    char* start = line;                     // store start pointer for later

    if (line == (char*)0) {
        return -1;                           // return failure
    }

    while (*line != '\0') {
        if (*line > 57 || *line < 48) {     // is character outside of the numeric ASCII range?
            return -1;                       // return failure
        }
        line++;
    }

    do {
        line--;
        val += pow*( ((int)*line)-48 );     // calculate value based on ASCII offset and power of digit
        pow = pow * 10;                     // next digit is x10 power
    } while (line != start);

    return val;                             // return value
}

// get user input
int getsUart0(char* buffer) {
    int count = 0;                          // init local variable for count set to 0

    while (true) {
        if (!kbhitUart0()) {                // any characters in receive buffer?
            yield();                        // no, yield our time
            continue;                       // will return to execution here, retest if buffer empty
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

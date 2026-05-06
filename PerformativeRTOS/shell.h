// Shell functions
// J Losh

// Sean Slater

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef SHELL_H_
#define SHELL_H_

// student created defines
#define MAX_CHARS 80
#define MIN_CHARS 1
#define MAX_TOKEN 8
#define NUM_STATE 7
#define MAX_STATE 8
#define NUM_OF_CMDS 11

// student created options
#define ECHO
#define MAX_TASKS 12

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void shell(void);

// call-able functions
void requestRebootuC();
void listProcess();
void listInterProcess();
void killProcessByPID(uint32_t pid);
void killProcessByName(char* p_name);
void setPrioInherit(char* argu);
void setPreempt(char* argu);
void setScheduling(char* argu);
void getProcess(char* p_name);
void runProcess(char* p_name);
void printCmds();

// generic shell functions
void printTime(uint32_t t_time, uint32_t total_time);
int decodeCommand(char* line);
void printHelp(uint8_t which);
void printUnknownArgu(char* argu);
void printPengu(uint8_t which);

// general-use print functions
void print32d(uint32_t x);
void print32h(uint32_t x);
void print32b(uint32_t x);
void print16h(uint32_t x);
void print8h(uint8_t x);
void print8d(uint8_t x);
void print8b(uint8_t x);
void print4h(uint8_t x);
void printBlanks(uint8_t number_of);

// general-use string functions
int getLength(char* str);
int compareStr(const char* key, char* tok);
char* getArgu(char* line);
int isNumeric(char* line);

// function to get user input
int getsUart0(char* buffer);

#endif

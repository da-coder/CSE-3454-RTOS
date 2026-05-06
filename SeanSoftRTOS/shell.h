// Shell functions
// J Losh

#ifndef SHELL_H_
#define SHELL_H_

#include <stdint.h>

// defines
#define UFAULT_STAT_MASK 0xFFFF0000
#define BFAULT_STAT_MASK 0x0000FF00
#define MFAULT_STAT_MASK 0x000000FF

#define MAX_CHARS 80
#define MIN_CHARS 1
#define MAX_TOKEN 8
#define NUM_STATE 7
#define MAX_STATE 8
#define NUM_OF_CMDS 11

#define ECHO

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// main shell program
void shell(void);

// generic shell functions
int decodeCommand(char* line);
void printHelp(uint8_t which);
void listProcess();
void listInterProcess();
void killProcessByPID(uint8_t pid);
void killProcessByName(char* p_name);
void setPrioInherit(uint8_t value);
void setPreempt(uint8_t value);
void setScheduling(uint8_t choice);
void getProcess(char* p_name);
void runProcess(char* p_name);
void printCmds();

// for fault handling
void printFaultMessage(uint8_t which);
void printCoreRegisters();
void printFaultStatRegister();
void printHFaultStatRegister();

// for address printing
void printMPUFaultAddress();
void printBusFaultAddress();

// general-use print functions
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

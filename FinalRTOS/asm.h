// Sean Slater

#ifndef __ASM_H
#define __ASM_H

#include <stdint.h>

// assembly functions for kernel.h
__attribute__((naked)) void coldStart(void* sp, void* fn);
void storeRegs(void* psp);
void restoreRegs(void* psp);
void* setupUnrun(void* fn, void* sp);
uint32_t getSVCn();
uint32_t getSVCa();
void* getSVCb();

// assembly functions for faults.h
void* getMsp();
void* getPsp();
int getTmpl();
void setTmpl(int);

#endif

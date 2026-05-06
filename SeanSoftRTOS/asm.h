#ifndef __ASM_H
#define __ASM_H

#include <stdint.h>

uint32_t* getPsp();
void setPsp(uint32_t* psp);
void setAsp();
uint32_t* getMsp();
void setTmpl(int val);
int getTmpl();
void startFunc(void* sp, void* fn);
void callFunc(void* fn);

#endif

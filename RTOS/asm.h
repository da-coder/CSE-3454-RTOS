#ifndef __ASM_H
#define __ASM_H
uint32_t* getPsp();
void setPsp(uint32_t* psp);
void setAsp();
uint32_t* getMsp();
void setTmpl(int val);
uint8_t getTmpl();
void callSVC(uint32_t func);
#endif

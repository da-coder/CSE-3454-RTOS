	.text
	.global getPsp
	.global setPsp
	.global setAsp
	.global getMsp
	.global setTmpl
	.global getTmpl
	.global startFunc
	.global callFunc

getPsp:
	MRS R0, PSP
	BX LR

setPsp:
	MSR PSP, R0
	ISB
	DSB
	BX LR

setAsp:
	MOV R1, #2
	MRS R0, CONTROL
	ORR R0, R0, R1
	MSR CONTROL, R0
	ISB
	DSB
	BX LR

getMsp:
	MOV R0, SP
	BX LR

setTmpl:
	MOV R1, R0
	MRS R0, CONTROL
	MVN R2, #0
	SUB R2, R2, #1
	AND R0, R0, R2
	ORR R0, R0, R1
	MSR CONTROL, R0
	ISB
	DSB
	BX LR

getTmpl:
	MOV R1, #1
	MRS R0, CONTROL
	AND R0, R0, R1
	BX LR

startFunc:
    MSR PSP, R0
    ISB
    DSB
    MRS R2, CONTROL
    MOV R3, #3
    ORR R2, R2, R3
    MSR CONTROL, R2
    ISB
    DSB
    MOV PC, R1

callFunc:
	MRS R1, PSP
	SUB R1, R1, #4
    MOV R2, #97
    LSL R2, R2, #24
    STR R2, [R1]
    SUB R1, R1, #4
    SUB R0, R0, #1
    STR R0, [R1]
	SUB R1, R1, #4
	MOV R2, #0x2229
    STR R2, [R1]
    SUB R1, R1, #20
    MSR PSP, R1
    ISB
    DSB
    BX LR

	.text
	.global coldStart
	.global storeRegs
	.global restoreRegs
	.global setupUnrun
	.global getSVCn
	.global getSVCa
	.global getSVCb
	.global getPsp
	.global getMsp
	.global getTmpl
	.global setTmpl
	.global getCurrentTime

coldStart:
	MSR PSP, R0
    MRS R2, CONTROL
    MOV R3, #3
    ORR R2, R2, R3
    MSR CONTROL, R2
    ISB
    DSB

    MOV PC, R1

storeRegs:
	MRS R2, PSP
	STR R2, [R0]
	LDR R1, [R13, #4]

	STR R1, [R2, #-4]
    STR R4, [R2, #-8]
    STR R5, [R2, #-12]
    STR R6, [R2, #-16]
    STR R7, [R2, #-20]
    STR R8, [R2, #-24]
    STR R9, [R2, #-28]
    STR R10, [R2, #-32]
    STR R11, [R2, #-36]

    BX LR

restoreRegs:
	LDR R2, [R0]
   	MSR PSP, R2

	LDR R1, [R2, #-4]
	STR R1, [R13, #4]
    LDR R4, [R2, #-8]
    LDR R5, [R2, #-12]
    LDR R6, [R2, #-16]
    LDR R7, [R2, #-20]
    LDR R8, [R2, #-24]
    LDR R9, [R2, #-28]
    LDR R10, [R2, #-32]
    LDR R11, [R2, #-36]

   	BX LR

setupUnrun:
    STR R0, [R1, #-8]
    MOV R0, #1

    LSL R0, R0, #24
    STR R0, [R1, #-4]
    MVN R0, #2
    STR R0, [R1, #-36]

    SUB R0, R1, #32
    MSR PSP, R0

    BX LR

getSVCn:
	MRS R2, PSP
	LDR R1, [R2, #24]
	LDR R0, [R1, #-2]
	MOV R1, #0xFF
	AND R0, R0, R1
	BX LR

getSVCa:
	MRS R1, PSP
	LDR R0, [R1]
	BX LR

getSVCb:
	MRS R1, PSP
	LDR R0, [R1, #4]
	BX LR

getPsp:
	MRS R0, PSP
	BX LR

getMsp:
	MRS R0, MSP
	BX LR

getTmpl:
	MOV R1, #1
	MRS R0, CONTROL
	AND R0, R0, R1
	BX LR

setTmpl:
	MRS R1, CONTROL
	MVN R2, #1
	AND R1, R1, R2
	AND R0, R0, #1
	ORR R0, R1
	MSR CONTROL, R0
	ISB
	DSB
	BX LR

	.text
	.global getPsp
	.global setPsp
	.global setAsp
	.global getMsp
	.global setTmpl
	.global getTmpl
	.global callSVC

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

callSVC:
	SVC #0
	BX LR


	.global movepsp
	.global setasp
	.global div0
	.global settmpl
	.global getpsp
	.global getmsp
	.global setpc
	.global pushreg
	.global popreg
	.global pushregfake

.thumb
.const

RETURN_VAL	.field 0xFFFFFFFD
XPSR_VAL    .field 0x41000000

.text
movepsp:
	MSR PSP, R0
	ISB
	BX LR

setasp:
	MOV R0, #2
	MSR CONTROL, R0
	ISB
	BX LR

div0:
	MOV R1, #0
	SDIV R0, #2, R1
	BX LR

settmpl:
	MOV R0, #3
	MSR CONTROL, R0
	ISB
	BX LR

getpsp:
	MRS R0, PSP
	ISB
	BX LR

getmsp:
	MRS R0, MSP
	ISB
	BX LR

setpc:
	MOV R1, #3
	MSR CONTROL, R1
	ISB
	BX R0

pushreg:
	MRS R0, PSP
	LDR R1, RETURN_VAL
	SUB R0, R0, #4
	STR R4, [R0]
	SUB R0, R0, #4
	STR R5, [R0]
	SUB R0, R0, #4
	STR R6, [R0]
	SUB R0, R0, #4
	STR R7, [R0]
	SUB R0, R0, #4
	STR R8, [R0]
	SUB R0, R0, #4
	STR R9, [R0]
	SUB R0, R0, #4
	STR R10, [R0]
	SUB R0, R0, #4
	STR R11, [R0]
	SUB R0, R0, #4
	STR R1, [R0]
	MSR PSP, R0
	;set msp to where it should be
	MRS R1, MSP
	ADD R1, R1, #8
	MSR MSP, R1
	BX LR

popreg:
	MRS R0, PSP
	LDR R14, [R0]
	ADD R0, R0, #4
	LDR R11, [R0]
	ADD R0, R0, #4
	LDR R10, [R0]
	ADD R0, R0, #4
	LDR R9, [R0]
	ADD R0, R0, #4
	LDR R8, [R0]
	ADD R0, R0, #4
	LDR R7, [R0]
	ADD R0, R0, #4
	LDR R6, [R0]
	ADD R0, R0, #4
	LDR R5, [R0]
	ADD R0, R0, #4
	LDR R4, [R0]
	ADD R0, R0, #4
	MSR PSP, R0
	ISB
	BX LR

pushregfake:
	MRS R1, PSP
    LDR R2, XPSR_VAL
    LDR LR, RETURN_VAL
	SUB R1, R1, #4
    STR R2, [R1]
	SUB R1, R1, #4
    STR R0, [R1]
    SUB R1, R1, #24
	MSR PSP, R1
	BX LR



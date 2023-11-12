// Faults functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef FAULTS_H_
#define FAULTS_H_

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void mpuFaultIsr(void);
void hardFaultIsr(void);
void busFaultIsr(void);
void usageFaultIsr(void);

extern void movepsp(void *psp);
extern void setasp();
extern void settmpl();
extern void* getpsp();
extern void* getmsp();
extern void setpc(void *pc);
extern void pushreg();
extern void popreg();
extern void pushregfake(void *ptr);
extern void popregfake();

#endif

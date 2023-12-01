// Shell functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "faults.h"
#include "uart0.h"
#include "commands.h"
#include "kernel.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// REQUIRED: If these were written in assembly
//           omit this file and add a faults.s file

// REQUIRED: code this function
void mpuFaultIsr(void)
{
    uint32_t *x;
    char str[10];
    NVIC_SYS_HND_CTRL_R &= ~NVIC_SYS_HND_CTRL_MEMA;
    NVIC_FAULT_STAT_R |= (NVIC_FAULT_STAT_IERR | NVIC_FAULT_STAT_DERR);

    putsUart0("MPU ISR\n");

    *x = getpsp();
    putsUart0("PSP: 0x");
    itoa(*x, str, 16);
    putsUart0(str);
    putsUart0("\n");

    *x = getmsp();
    putsUart0("MSP: 0x");
    itoa(*x, str, 16);
    putsUart0(str);
    putsUart0("\n");

    *x = NVIC_MM_ADDR_R;
    putsUart0("Fault Address: 0x");
    itoa(*x, str, 16);
    putsUart0(str);
    putsUart0("\n");

    *x = getpsp();
    putsUart0("Instruction Address: 0x");
    itoa(*x, str, 16);
    putsUart0(str);
    putsUart0("\n");

    *x = NVIC_FAULT_STAT_R & 0xFF;
    putsUart0("MFAULT REG: 0x");
    itoa(*x, str, 16);
    putsUart0(str);
    putsUart0("\n");

    x = getpsp();
    putsUart0("R0: 0x");
    itoa((uint32_t)*x, str, 16);
    putsUart0(str);
    putsUart0("\n");

    x++;
    putsUart0("R1: 0x");
    itoa((uint32_t)*x, str, 16);
    putsUart0(str);
    putsUart0("\n");

    x++;
    putsUart0("R2: 0x");
    itoa((uint32_t)*x, str, 16);
    putsUart0(str);
    putsUart0("\n");

    x++;
    putsUart0("R3: 0x");
    itoa((uint32_t)*x, str, 16);
    putsUart0(str);
    putsUart0("\n");

    x++;
    putsUart0("R12: 0x");
    itoa((uint32_t)*x, str, 16);
    putsUart0(str);
    putsUart0("\n");

    x++;
    putsUart0("LR: 0x");
    itoa((uint32_t)*x, str, 16);
    putsUart0(str);
    putsUart0("\n");

    x++;
    putsUart0("PC: 0x");
    itoa((uint32_t)*x, str, 16);
    putsUart0(str);
    putsUart0("\n");

    x++;
    putsUart0("xPSR: 0x");
    itoa(*x, str, 16);
    putsUart0(str);
    putsUart0("\n");
    stopThread(0);
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}

// REQUIRED: code this function
void hardFaultIsr(void)
{
    uint32_t *x;
    char str[10];
    putsUart0("HARD ISR\n");

    *x = getpsp();
    putsUart0("PSP: 0x");
    itoa(*x, str, 16);
    putsUart0(str);
    putsUart0("\n");

    *x = getmsp();
    putsUart0("MSP: 0x");
    itoa(*x, str, 16);
    putsUart0(str);
    putsUart0("\n");

    *x = NVIC_HFAULT_STAT_R;
    putsUart0("HFAULT REG: 0x");
    itoa(*x, str, 16);
    putsUart0(str);
    putsUart0("\n");

//    while(1);
}

// REQUIRED: code this function
void busFaultIsr(void)
{
    static uint32_t pid = 2;
    char str[10];
    putsUart0("Bus fault in process ");
    itoa(pid, str, 10);
    putsUart0(str);
    putsUart0("\n");
    while(1);
}

// REQUIRED: code this function
void usageFaultIsr(void)
{
    char str[10];
    static uint32_t pid = 3;
    putsUart0("Usage fault in process ");
    itoa(pid, str, 10);
    putsUart0(str);
    putsUart0("\n");
    while(1);
}


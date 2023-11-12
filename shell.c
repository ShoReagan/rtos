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
#include "shell.h"

// REQUIRED: Add header files here for your strings functions, ...
#include "commands.h"
#include "uart0.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// REQUIRED: add processing for the shell commands through the UART here
void shell(void)
{
    USER_DATA data;
    char *strTemp;

    if(kbhitUart0())
    {
        getsUart0(&data);
        parseFields(&data);

        if(isCommand(&data, "reboot", 1))
        {
            NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
        }
//        else if(isCommand(&data, "ps", 1))
//        {
//            psCalled();
//        }
//        else if(isCommand(&data, "ipcs", 1))
//        {
//            ipcsCalled();
//        }
//        else if(isCommand(&data, "kill", 2))
//        {
//            kill(getFieldInteger(&data, 1));
//        }
//        else if(isCommand(&data, "Pkill", 2))
//        {
//            strTemp = getFieldString(&data, 1);
//            pkill(strTemp);
//        }
//        else if(isCommand(&data, "preempt", 2))
//        {
//            strTemp = getFieldString(&data, 1);
//            if(strcmp1(strTemp, "ON"))
//                preempt(1);
//            else if(strcmp1(strTemp, "OFF"))
//                preempt(0);
//        }
//        else if(isCommand(&data, "sched", 2))
//        {
//            strTemp = getFieldString(&data, 1);
//            if(strcmp1(strTemp, "PRIO"))
//                sched(1);
//            else if(strcmp1(strTemp, "RR"))
//                sched(0);
//        }
//        else if(isCommand(&data, "pidof", 2))
//        {
//            strTemp = getFieldString(&data, 1);
//            pidof(strTemp);
//        }
//        else if(isCommand(&data, "run", 1))
//        {
//            strTemp = getFieldString(&data, 1);
//            run(strTemp);
//        }
    }
}

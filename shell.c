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
#include "kernel.h"

typedef struct _processes
{
    char name[12];
    char pid[8];
    uint32_t state;
    uint32_t stable;
} processes;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// REQUIRED: add processing for the shell commands through the UART here
void shell(void)
{
    USER_DATA data;
    char *strTemp;
    char strTemp1[33];
    uint32_t intTemp;
    uint8_t i, j;

    mutex tempMutexes[MAX_MUTEXES];
    semaphore tempSemaphores[MAX_SEMAPHORES];
    processes tempProcesses[12];

    while(true)
    {
        if(kbhitUart0())
        {
            getsUart0(&data);
            parseFields(&data);
            if(isCommand(&data, "reboot", 1))
            {
                reboot();
            }
            else if(isCommand(&data, "ps", 1))
            {
                ps(&tempProcesses);
                putsUart0("Task Pid State\n");
                for(i = 0; i < 10; i++)
                {
                    putsUart0(tempProcesses[i].name);
                    putsUart0(" ");
                    putsUart0(tempProcesses[i].pid);
                    putsUart0(" ");
                    itoa(tempProcesses[i].stable, strTemp1, 10);
                    putsUart0(strTemp1);
                    putsUart0(" ");
                    j = tempProcesses[i].state & 0xFF;
                    switch(j)
                    {
                    case 0:
                        putsUart0("STATE_INVALID");
                        break;
                    case 1:
                        putsUart0("STATE_UNRUN");
                        break;
                    case 2:
                        putsUart0("STATE_READY");
                        break;
                    case 3:
                        putsUart0("STATE_DELAYED");
                        break;
                    case 4:
                        putsUart0("STATE_BLOCKED_MUTEX");
                        break;
                    case 5:
                        putsUart0("STATE_BLOCKED_SEMAPHORE");
                        break;
                    }
                    putsUart0("\n");
                }
            }
            else if(isCommand(&data, "ipcs", 1))
            {
                ipcsMutexes(&tempMutexes);
                ipcsSemaphores(&tempSemaphores);

                putsUart0("---MUTEXES---\n\n");
                putsUart0("Locked: ");
                putsUart0(itoa(tempMutexes[0].lock, strTemp1, 10));
                putsUart0("\n");

                putsUart0("Queue Size: ");
                putsUart0(itoa(tempMutexes[0].queueSize, strTemp1, 10));
                putsUart0("\n");

                for(i = 0; i < tempMutexes[0].queueSize; i++)
                {
                    putsUart0("Process: ");
                    putsUart0(itoa(tempMutexes[0].processQueue[i], strTemp1, 10));
                    putsUart0("\n");
                }

                putsUart0("Locked by: ");
                putsUart0(itoa(tempMutexes[0].lockedBy, strTemp1, 10));
                putsUart0("\n\n");

                putsUart0("---SEMAPHORES---\n\n");
                for(i = 0; i < MAX_SEMAPHORES; i++)
                {
                    putsUart0("Count: ");
                    putsUart0(itoa(tempSemaphores[i].count, strTemp1, 10));
                    putsUart0("\n");

                    putsUart0("Queue Size: ");
                    putsUart0(itoa(tempSemaphores[i].queueSize, strTemp1, 10));
                    putsUart0("\n");

                    for(j = 0; j < tempSemaphores[i].queueSize; j++)
                    {
                        putsUart0("Process: ");
                        putsUart0(itoa(tempSemaphores[i].processQueue[j], strTemp1, 10));
                        putsUart0("\n");
                    }
                    putsUart0("\n");
                }
            }
            else if(isCommand(&data, "kill", 1))
            {
                intTemp = toDeci(getFieldString(&data, 1), 16);
                kill(intTemp);
            }
            else if(isCommand(&data, "Pkill", 1))
            {
                strTemp = getFieldString(&data, 1);
                intTemp = pidof(strTemp);
                kill(intTemp);
            }
            else if(isCommand(&data, "preempt", 1))
            {
                strTemp = getFieldString(&data, 1);
                if(strcmp(strTemp, "ON"))
                    preempt(1);
                else if(strcmp(strTemp, "OFF"))
                    preempt(0);
            }
            else if(isCommand(&data, "sched", 1))
            {
                strTemp = getFieldString(&data, 1);
                if(strcmp(strTemp, "PRIO"))
                    sched(1);
                else if(strcmp(strTemp, "RR"))
                    sched(0);
            }
            else if(isCommand(&data, "pidof", 1))
            {
                strTemp = getFieldString(&data, 1);
                intTemp = pidof(strTemp);
                itoa(intTemp, strTemp, 16);
                putsUart0("pid: 0x");
                putsUart0(strTemp);
                putsUart0("\n");
            }
            else if(isCommand(&data, "run", 1))
            {
                strTemp = getFieldString(&data, 1);
                intTemp = pidof(strTemp);
                run(intTemp);
            }
        }
        yield();
    }
}

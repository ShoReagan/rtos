// Kernel functions
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
#include "mm.h"
#include "kernel.h"
#include "faults.h"
#include "commands.h"
#include "shell.h"

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

#define YIELD 0
#define SLEEP 1
#define LOCK 2
#define UNLOCK 3
#define WAIT 4
#define POST 5
#define PRIO 6
#define PIDOF 7
#define PREEMPT 8
#define KILL 9
#define REBOOT 10
#define RUN 11
#define MUTEXES 12
#define SEMAPHORES 13
#define PS 14

#define MAX_TASKS 12              // maximum number of valid tasks
uint8_t taskCurrent = 0;          // index of last dispatched task
uint8_t taskCount = 0;            // total number of valid tasks

uint8_t priority_schedule = 1;
uint8_t preempt_on = 0;

#define NUM_SRAM_REGIONS 4
#define NUM_PRIORITIES   8
struct _tcb
{
    uint8_t state;                 // see STATE_ values above
    void *pid;                     // used to uniquely identify thread (add of task fn)
    void *spInit;                  // original top of stack
    void *sp;                      // current stack pointer
    int8_t priority;               // 0=highest
    uint32_t ticks;                // ticks until sleep complete
    uint8_t srd[NUM_SRAM_REGIONS]; // MPU subregion disable bits
    char name[16];                 // name of task used in ps command
    uint8_t mutex;                 // index of the mutex in use or blocking the thread
    uint8_t semaphore;             // index of the semaphore that is blocking the thread
    uint32_t stable;
    uint32_t changing;
} tcb[MAX_TASKS];

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

bool initMutex(uint8_t mutex)
{
    bool ok = (mutex < MAX_MUTEXES);
    return ok;
}

bool initSemaphore(uint8_t semaphore, uint8_t count)
{
    bool ok = (semaphore < MAX_SEMAPHORES);
    {
        semaphores[semaphore].count = count;
    }
    return ok;
}

// REQUIRED: initialize systick for 1ms system timer
void initRtos(void)
{
    uint8_t i;
    // no tasks running
    taskCount = 0;
    // clear out tcb records
    for (i = 0; i < MAX_TASKS; i++)
    {
        tcb[i].state = STATE_INVALID;
        tcb[i].pid = 0;
    }
}

// REQUIRED: modify this function to start the operating system
// by calling scheduler, set srd bits, setting PSP, ASP bit, TMPL bit, and PC
void startRtos(void)
{
    tcb[taskCurrent].state = STATE_UNRUN;
    taskCurrent = rtosScheduler();
    setSrdMask(tcb[taskCurrent].srd);
    movepsp((uint32_t)tcb[taskCurrent].spInit);
    setpc(tcb[taskCurrent].pid);
}

// REQUIRED:
// add task if room in task list
// store the thread name
// allocate stack space and store top of stack in sp and spInit
// set the srd bits based on the memory allocation
bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes)
{
    bool ok = false;
    uint8_t i = 0;
    bool found = false;
    if (taskCount < MAX_TASKS)
    {
        // make sure fn not already in list (prevent reentrancy)
        while (!found && (i < MAX_TASKS))
        {
            found = (tcb[i++].pid ==  fn);
        }
        if (!found)
        {
            // find first available tcb record
            i = 0;
            while (tcb[i].state != STATE_INVALID) {i++;}
            tcb[i].state = STATE_UNRUN;
            tcb[i].pid = fn;
            tcb[i].sp = mallocFromHeap(stackBytes);
            tcb[i].spInit = tcb[i].sp;
            tcb[i].priority = priority;
            getSrdMask(tcb[i].srd, tcb[i].spInit, stackBytes);
            strcpy(tcb[i].name, name);

            // increment task count
            taskCount++;
            ok = true;
        }
    }
    return ok;
}

// REQUIRED: modify this function to restart a thread
void restartThread(_fn fn)
{
    uint8_t i, taskNum;

    //get task number
    for(i = 0; i < taskCount; i++)
    {
        if(tcb[i].pid == (void*)fn)
        {
            taskNum = i;
            break;
        }
    }

    tcb[taskNum].sp = tcb[taskNum].spInit;
    tcb[taskNum].state = STATE_UNRUN;
}

// REQUIRED: modify this function to stop a thread
// REQUIRED: remove any pending semaphore waiting, unlock any mutexes
void stopThread(_fn fn)
{
    uint8_t i, j, taskNum;

    //get task number
    if(fn == 0)
    {
        i = taskCurrent;
    }
    else
    {
        for(i = 0; i < taskCount; i++)
        {
            if(tcb[i].pid == (void*)fn)
            {
                taskNum = i;
                break;
            }
        }
    }

    //remove from mutex queue
    if(mutexes[0].queueSize)
        tcb[mutexes[0].processQueue[0]].state = STATE_READY;

    for(i = 0; i < (mutexes[0].queueSize - 1); i++)
    {
        if(mutexes[0].processQueue[i] == taskNum)
        {
            mutexes[0].processQueue[i] = mutexes[0].processQueue[i + 1];
            mutexes[0].queueSize--;
        }
    }

    //remove if currently using mutex
    if(mutexes[0].lock == true && mutexes[0].lockedBy == taskNum)
        mutexes[0].lock = false;

    //remove from semaphore queue
    for(i = 0; i < MAX_SEMAPHORES; i++)
    {
        for(j = 0; j < (MAX_SEMAPHORE_QUEUE_SIZE - 1); j++)
        {
            if(semaphores[i].processQueue[j] == taskNum)
            {
                semaphores[i].processQueue[j] = semaphores[i].processQueue[j + 1];
            }
        }
    }
    tcb[taskNum].state = STATE_INVALID;
}

// REQUIRED: modify this function to set a thread priority
void setThreadPriority(_fn fn, uint8_t priority)
{
    uint8_t i, taskNum;

    //get task number
    for(i = 0; i < taskCount; i++)
    {
        if(tcb[i].pid == (void*)fn)
        {
            taskNum = i;
            break;
        }
    }

    tcb[taskNum].priority = priority;
}

uint8_t find_lowest_prio()
{
    uint8_t i, temp;
    temp = 0x7;
    for(i = 0; i < taskCount; i++)
    {
        if(tcb[i].priority < temp && (tcb[i].state == STATE_READY || tcb[i].state == STATE_UNRUN))
        {
            temp = tcb[i].priority;
        }
    }
    return temp;
}

// REQUIRED: Implement prioritization to NUM_PRIORITIES
int rtosScheduler(void)
{
    static uint8_t task = 0xFF;
    uint8_t i, lowestPrio;
    while(1)
    {
        lowestPrio = find_lowest_prio();
        for(i = 0; i < taskCount; i++)
        {
            task++;
            if (task >= taskCount)
                task = 0;

            if(priority_schedule)
            {
                if(tcb[task].priority == lowestPrio && (tcb[task].state == STATE_READY || tcb[task].state == STATE_UNRUN))
                    return task;
            }
            else
            {
                if(tcb[task].state == STATE_READY || tcb[task].state == STATE_UNRUN)
                    return task;
            }
        }
    }
}


// REQUIRED: modify this function to yield execution back to scheduler using pendsv
void yield(void)
{
    __asm("    SVC #0");
}

// REQUIRED: modify this function to support 1ms system timer
// execution yielded back to scheduler until time elapses using pendsv
void sleep(uint32_t tick)
{
    __asm("    SVC #1");
}

// REQUIRED: modify this function to lock a mutex using pendsv
void lock(int8_t mutex)
{
    __asm("    SVC #2");
}

// REQUIRED: modify this function to unlock a mutex using pendsv
void unlock(int8_t mutex)
{
    __asm("    SVC #3");
}

// REQUIRED: modify this function to wait a semaphore using pendsv
void wait(int8_t semaphore)
{
    __asm("    SVC #4");
}

// REQUIRED: modify this function to signal a semaphore is available using pendsv
void post(int8_t semaphore)
{
    __asm("    SVC #5");
}

void sched(int8_t prio)
{
    __asm("    SVC #6");
}

int pidof(char *str)
{
    __asm("    SVC #7");
}

void preempt(uint8_t on)
{
    __asm("    SVC #8");
}

void kill(uint32_t pid)
{
    __asm("    SVC #9");
}

void reboot()
{
    __asm("    SVC #10");
}

void run(uint32_t pid)
{
    __asm("    SVC #11");
}

void ipcsMutexes(void *ptr)
{
    __asm("    SVC #12");
}

void ipcsSemaphores(void *ptr)
{
    __asm("    SVC #13");
}

void ps(void *ptr)
{
    __asm("    SVC #14");
}

// REQUIRED: modify this function to add support for the system timer
// REQUIRED: in preemptive code, add code to request task switch
void systickIsr(void)
{
    uint8_t i;
    static uint16_t counter;
    if(counter >= 3000)
    {
        WTIMER1_TAV_R = 0;
        counter = 0;
        for(i = 0; i < taskCount; i++)
        {
            tcb[i].stable = tcb[i].changing;
        }
    }
    for(i = 0; i < taskCount; i++)
    {
        if(tcb[i].state == STATE_DELAYED)
        {
            if(tcb[i].ticks == 0)
                tcb[i].state = STATE_READY;
            else
                tcb[i].ticks--;
        }
    }
    counter++;
    if(preempt_on)
        NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}

// REQUIRED: in coop and preemptive, modify this function to add support for task switching
// REQUIRED: process UNRUN and READY tasks differently
__attribute__((naked))
void pendSvIsr(void)
{
    pushreg();
    tcb[taskCurrent].sp = getpsp();
    tcb[taskCurrent].changing = WTIMER1_TAV_R - tcb[taskCurrent].changing;
    taskCurrent = rtosScheduler();
    tcb[taskCurrent].changing = WTIMER1_TAV_R;
    movepsp(tcb[taskCurrent].sp);
    setSrdMask(tcb[taskCurrent].srd);
    if(tcb[taskCurrent].state == STATE_READY)
        popreg();
    else
    {
        tcb[taskCurrent].state = STATE_READY;
        pushregfake(tcb[taskCurrent].pid);
    }
    __asm("    BX LR");
}

// REQUIRED: modify this function to add support for the service call
// REQUIRED: in preemptive code, add code to handle synchronization primitives
void svCallIsr(void)
{
    uint32_t r0;
    uint32_t *psp_addr;
    uint16_t *pc_val;
    uint8_t svc_num, i;
    uint16_t *struct_ptr;
    char strTemp[8];

    //get svc number
    psp_addr = (uint32_t)(getpsp() + (24));
    pc_val = (*psp_addr) - 2;
    svc_num = (*pc_val) & 0xFF;

    //get first argument (R0)
    psp_addr = (uint32_t)(getpsp());
    r0 = *psp_addr;

    switch(svc_num)
    {
    case YIELD:
        NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
        break;
    case SLEEP:
        tcb[taskCurrent].state = STATE_DELAYED;
        tcb[taskCurrent].ticks = r0;
        NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
        break;
    case LOCK:
        if(mutexes[r0].lock == false)
        {
            mutexes[r0].lock = true;
            mutexes[r0].lockedBy = taskCurrent;
            break;
        }
        else
        {
            mutexes[r0].processQueue[mutexes[r0].queueSize] = taskCurrent;
            mutexes[r0].queueSize++;
            tcb[taskCurrent].state = STATE_BLOCKED_MUTEX;
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
            break;
        }
    case UNLOCK:
        if(mutexes[r0].lockedBy == taskCurrent)
        {
            mutexes[r0].lock = false;
            if(mutexes[r0].queueSize)
            {
                tcb[mutexes[r0].processQueue[0]].state = STATE_READY;
                mutexes[r0].queueSize--;
                mutexes[r0].lock = true;
                mutexes[r0].lockedBy = mutexes[r0].processQueue[0];
                for(i = 0; i < (MAX_MUTEX_QUEUE_SIZE - 1); i++)
                {
                    mutexes[r0].processQueue[i] = mutexes[r0].processQueue[i + 1];
                }
            }
        }
        break;
    case WAIT:
        if(semaphores[r0].count > 0)
        {
            semaphores[r0].count--;
            break;
        }
        else
        {
            semaphores[r0].processQueue[semaphores[r0].queueSize] = taskCurrent;
            semaphores[r0].queueSize++;
            tcb[taskCurrent].state = STATE_BLOCKED_SEMAPHORE;
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
            break;
        }
    case POST:
        semaphores[r0].count++;
        if(semaphores[r0].queueSize)
        {
            tcb[semaphores[r0].processQueue[0]].state = STATE_READY;
            semaphores[r0].queueSize--;
            for(i = 0; i < (MAX_SEMAPHORE_QUEUE_SIZE - 1); i++)
            {
                semaphores[r0].processQueue[i] = semaphores[r0].processQueue[i + 1];
            }
            semaphores[r0].count--;
        }
        break;
    case PRIO:
        priority_schedule = r0;
        break;
    case PIDOF:
        for(i = 0; i < taskCount; i++)
        {
            if(strcmp((char*)r0, tcb[i].name))
            {
                *psp_addr = (int)tcb[i].pid;
                break;
            }
        }
        break;
    case PREEMPT:
        preempt_on = r0;
        break;
    case KILL:
        stopThread(r0);
        break;
    case REBOOT:
        NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
        break;
    case RUN:
        restartThread(r0);
        break;
    case MUTEXES:
        struct_ptr = r0;
        for(i = 0; i < MAX_MUTEXES; i++)
        {
            *struct_ptr = mutexes[i].lock;
            struct_ptr += 1;

            *struct_ptr = mutexes[i].queueSize;
            struct_ptr += 1;

            *struct_ptr = mutexes[i].processQueue[0] & 0xFF;
            struct_ptr += 1;
            *struct_ptr = mutexes[i].processQueue[0] & 0xFF00;
            struct_ptr += 1;

            *struct_ptr = mutexes[i].processQueue[1] & 0xFF;
            struct_ptr += 1;
            *struct_ptr = mutexes[i].processQueue[1] & 0xFF00;
            struct_ptr += 1;

            *struct_ptr = mutexes[i].lockedBy & 0xFF;
            struct_ptr += 1;
            *struct_ptr = mutexes[i].lockedBy & 0xFF00;
        }
        break;
    case SEMAPHORES:
        struct_ptr = r0;
        for(i = 0; i < MAX_SEMAPHORES; i++)
        {
            *struct_ptr = semaphores[i].count;
            struct_ptr += 1;

            *struct_ptr = semaphores[i].queueSize;
            struct_ptr += 1;

            *struct_ptr = semaphores[i].processQueue[0] & 0xFF;
            struct_ptr += 1;
            *struct_ptr = semaphores[i].processQueue[0] & 0xFF00;
            struct_ptr += 1;

            *struct_ptr = semaphores[i].processQueue[1] & 0xFF;
            struct_ptr += 1;
            *struct_ptr = semaphores[i].processQueue[1] & 0xFF00;
            struct_ptr += 1;
        }
        break;
    case PS:
        struct_ptr = r0;
        for(i = 0; i < MAX_TASKS; i++)
        {
            strcpy((char*)struct_ptr, tcb[i].name);
            struct_ptr += 6;

            itoa((int)tcb[i].pid, strTemp, 16);
            strcpy((char*)struct_ptr, strTemp);
            struct_ptr += 4;

            *struct_ptr = tcb[i].state;
            struct_ptr += 2;

            *struct_ptr = tcb[i].stable & 0xFF;
            struct_ptr += 1;
            *struct_ptr = tcb[i].stable & 0xFF00;
            struct_ptr += 1;
        }
        break;
    }
}


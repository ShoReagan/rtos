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

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// task
#define STATE_INVALID           0 // no task
#define STATE_UNRUN             1 // task has never been run
#define STATE_READY             2 // has run, can resume at any time
#define STATE_DELAYED           3 // has run, but now awaiting timer
#define STATE_BLOCKED_MUTEX     4 // has run, but now blocked by semaphore
#define STATE_BLOCKED_SEMAPHORE 5 // has run, but now blocked by semaphore

#define YIELD 0
#define SLEEP 1
#define LOCK 2
#define UNLOCK 3

#define MAX_TASKS 12              // maximum number of valid tasks
uint8_t taskCurrent = 0;          // index of last dispatched task
uint8_t taskCount = 0;            // total number of valid tasks

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
    tcb[taskCurrent].state = STATE_READY;
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
}

// REQUIRED: modify this function to stop a thread
// REQUIRED: remove any pending semaphore waiting, unlock any mutexes
void stopThread(_fn fn)
{
}

// REQUIRED: modify this function to set a thread priority
void setThreadPriority(_fn fn, uint8_t priority)
{
}

// REQUIRED: Implement prioritization to NUM_PRIORITIES
int rtosScheduler(void)
{
    static bool ok;
    static uint8_t task = 0xFF;
    ok = false;
    while (!ok)
    {
        task++;
        if (task >= MAX_TASKS)
            task = 0;
        ok = (tcb[task].state == STATE_READY || tcb[task].state == STATE_UNRUN);
    }
    return task;
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
}

// REQUIRED: modify this function to signal a semaphore is available using pendsv
void post(int8_t semaphore)
{
}

// REQUIRED: modify this function to add support for the system timer
// REQUIRED: in preemptive code, add code to request task switch
void systickIsr(void)
{
    uint8_t i;
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
}

// REQUIRED: in coop and preemptive, modify this function to add support for task switching
// REQUIRED: process UNRUN and READY tasks differently
void pendSvIsr(void)
{
    pushreg();
    tcb[taskCurrent].sp = getpsp();
    taskCurrent = rtosScheduler();
    movepsp(tcb[taskCurrent].sp);
    setSrdMask(tcb[taskCurrent].srd);
    if(tcb[taskCurrent].state == STATE_READY)
        popreg();
    else
    {
        tcb[taskCurrent].state = STATE_READY;
        pushregfake(tcb[taskCurrent].pid);
    }

}

// REQUIRED: modify this function to add support for the service call
// REQUIRED: in preemptive code, add code to handle synchronization primitives
void svCallIsr(void)
{
    uint32_t r0;
    uint32_t *psp_addr;
    uint16_t *pc_val;
    uint8_t svc_num;

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
                    uint8_t i;
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
    }
}


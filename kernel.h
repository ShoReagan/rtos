// Kernel functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef KERNEL_H_
#define KERNEL_H_

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// function pointer
typedef void (*_fn)();

// mutex
#define MAX_MUTEXES 1
#define MAX_MUTEX_QUEUE_SIZE 2
typedef struct _mutex
{
    bool lock;
    uint16_t queueSize;
    uint32_t processQueue[MAX_MUTEX_QUEUE_SIZE];
    uint32_t lockedBy;
} mutex;
mutex mutexes[MAX_MUTEXES];
#define resource 0

// semaphore
#define MAX_SEMAPHORES 3
#define MAX_SEMAPHORE_QUEUE_SIZE 2
typedef struct _semaphore
{
    uint16_t count;
    uint16_t queueSize;
    uint32_t processQueue[MAX_SEMAPHORE_QUEUE_SIZE];
} semaphore;
semaphore semaphores[MAX_SEMAPHORES];
#define keyPressed 0
#define keyReleased 1
#define flashReq 2

// task
#define STATE_INVALID           0 // no task
#define STATE_UNRUN             1 // task has never been run
#define STATE_READY             2 // has run, can resume at any time
#define STATE_DELAYED           3 // has run, but now awaiting timer
#define STATE_BLOCKED_MUTEX     4 // has run, but now blocked by mutex
#define STATE_BLOCKED_SEMAPHORE 5 // has run, but now blocked by semaphore

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

bool initMutex(uint8_t mutex);
bool initSemaphore(uint8_t semaphore, uint8_t count);

void initRtos(void);
void startRtos(void);

bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes);
void restartThread(_fn fn);
void stopThread(_fn fn);
void setThreadPriority(_fn fn, uint8_t priority);

int rtosScheduler(void);

void yield(void);
void sleep(uint32_t tick);
void lock(int8_t mutex);
void unlock(int8_t mutex);
void wait(int8_t semaphore);
void post(int8_t semaphore);
void sched(int8_t prio);
int pidof(char *str);
void preempt(uint8_t on);
void kill(uint32_t pid);
void reboot();
void run(uint32_t pid);
void ipcsMutexes(void *ptr);
void ipcsSemaphores(void *ptr);
void ps(void *ptr);

void stopThread(_fn fn);

void systickIsr(void);
void pendSvIsr(void);
void svCallIsr(void);

#endif

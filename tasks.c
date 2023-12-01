// Tasks
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
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "wait.h"
#include "kernel.h"
#include "tasks.h"
#include "uart0.h"

#define BLUE_LED   PORTF,2 // on-board blue LED
#define RED_LED    PORTD,3 // off-board red LED
#define ORANGE_LED PORTD,6 // off-board orange LED
#define YELLOW_LED PORTC,6 // off-board yellow LED
#define GREEN_LED  PORTD,2 // off-board green LED

#define BUTTON1 PORTD, 1
#define BUTTON2 PORTE, 0
#define BUTTON3 PORTB, 7
#define BUTTON4 PORTA, 3
#define BUTTON5 PORTA, 2
#define BUTTON6 PORTD, 7

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
// REQUIRED: Add initialization for blue, orange, red, green, and yellow LEDs
//           Add initialization for 6 pushbuttons
void initHw(void)
{
    // Setup LEDs and pushbuttons
    enablePort(PORTA);
    enablePort(PORTB);
    enablePort(PORTC);
    enablePort(PORTD);
    enablePort(PORTE);
    enablePort(PORTF);

    // Unlock special pin D7
    setPinCommitControl(BUTTON6);

    // Button set up
    selectPinDigitalInput(BUTTON1);
    selectPinDigitalInput(BUTTON2);
    selectPinDigitalInput(BUTTON3);
    selectPinDigitalInput(BUTTON4);
    selectPinDigitalInput(BUTTON5);
    selectPinDigitalInput(BUTTON6);

    enablePinPullup(BUTTON1);
    enablePinPullup(BUTTON2);
    enablePinPullup(BUTTON3);
    enablePinPullup(BUTTON4);
    enablePinPullup(BUTTON5);
    enablePinPullup(BUTTON6);

    // External LED set up
    selectPinPushPullOutput(RED_LED);
    selectPinPushPullOutput(GREEN_LED);
    selectPinPushPullOutput(YELLOW_LED);
    selectPinPushPullOutput(ORANGE_LED);

//    // Configure Wide Timer 1 as counter of external events on CCP0 pin
    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R1;
    _delay_cycles(3);

    WTIMER1_CTL_R &= ~TIMER_CTL_TAEN;                // turn-off counter before reconfiguring
    WTIMER1_CFG_R = 4;                               // configure as 32-bit counter (A only)
    WTIMER1_TAMR_R = 1 | TIMER_TAMR_TACDIR; // configure for edge count mode, count up
    WTIMER1_CTL_R = 0;                               //
    WTIMER1_IMR_R = 0;                               // turn-off interrupts
    WTIMER1_TAV_R = 0;                               // zero counter for first period
    WTIMER1_CTL_R |= TIMER_CTL_TAEN;                 // turn-on counter

    // Board LED set up
    selectPinPushPullOutput(BLUE_LED);

    // Power-up flash
    setPinValue(GREEN_LED, 1);
    waitMicrosecond(250000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(250000);

    //enable usage, bus, mem fault, and svc call
    NVIC_CFG_CTRL_R |= NVIC_CFG_CTRL_DIV0;
    NVIC_SYS_HND_CTRL_R |= NVIC_SYS_HND_CTRL_BUS | NVIC_SYS_HND_CTRL_MEM | NVIC_SYS_HND_CTRL_USAGE;

    //configure systick timer to 1ms interrupt rate
    //9C40 = 40,000 (40 Mhz / 1,000)
    NVIC_ST_RELOAD_R = 0x9C40;
    //clear reg by writing a value to it
    NVIC_ST_CURRENT_R = 0xFFFF;
    //set ctrl register
    NVIC_ST_CTRL_R = 7;
}

// REQUIRED: add code to return a value from 0-63 indicating which of 6 PBs are pressed
uint8_t readPbs(void)
{
    uint8_t temp = 0;
    if(!getPinValue(BUTTON1))
    {
        temp += 1;
    }
    if(!getPinValue(BUTTON2))
    {
        temp += 2;
    }
    if(!getPinValue(BUTTON3))
    {
        temp += 4;
    }
    if(!getPinValue(BUTTON4))
    {
        temp += 8;
    }
    if(!getPinValue(BUTTON5))
    {
        temp += 16;
    }
    if(!getPinValue(BUTTON6))
    {
        temp += 32;
    }
    return temp;
}

// one task must be ready at all times or the scheduler will fail
// the idle task is implemented for this purpose
void idle(void)
{
    while(true)
    {
        setPinValue(ORANGE_LED, 1);
        waitMicrosecond(1000);
        setPinValue(ORANGE_LED, 0);
        yield();
    }
}

void idle2(void)
{
    while(true)
    {
        setPinValue(YELLOW_LED, 1);
        waitMicrosecond(1000);
        setPinValue(YELLOW_LED, 0);
        yield();
    }
}

void flash4Hz(void)
{
    while(true)
    {
        setPinValue(GREEN_LED, !getPinValue(GREEN_LED));
        sleep(125);
    }
}

void oneshot(void)
{
    while(true)
    {
        wait(flashReq);
        setPinValue(YELLOW_LED, 1);
        sleep(1000);
        setPinValue(YELLOW_LED, 0);
    }
}

void partOfLengthyFn(void)
{
    // represent some lengthy operation
    waitMicrosecond(990);
    // give another process a chance to run
    yield();
}

void lengthyFn(void)
{
    uint16_t i;
    while(true)
    {
        lock(resource);
        for (i = 0; i < 5000; i++)
        {
            partOfLengthyFn();
        }
        setPinValue(RED_LED, !getPinValue(RED_LED));
        unlock(resource);
    }
}

void readKeys(void)
{
    uint8_t buttons;
    while(true)
    {
        wait(keyReleased);
        buttons = 0;
        while (buttons == 0)
        {
            buttons = readPbs();
            yield();
        }
        post(keyPressed);
        if ((buttons & 1) != 0)
        {
            setPinValue(YELLOW_LED, !getPinValue(YELLOW_LED));
            setPinValue(RED_LED, 1);
        }
        if ((buttons & 2) != 0)
        {
            post(flashReq);
            setPinValue(RED_LED, 0);
        }
        if ((buttons & 4) != 0)
        {
            restartThread(flash4Hz);
        }
        if ((buttons & 8) != 0)
        {
            stopThread(flash4Hz);
        }
        if ((buttons & 16) != 0)
        {
            setThreadPriority(lengthyFn, 4);
        }
        yield();
    }
}

void debounce(void)
{
    uint8_t count;
    while(true)
    {
        wait(keyPressed);
        count = 10;
        while (count != 0)
        {
            sleep(10);
            if (readPbs() == 0)
                count--;
            else
                count = 10;
        }
        post(keyReleased);
    }
}

void uncooperative(void)
{
    while(true)
    {
        while (readPbs() == 8)
        {
        }
        yield();
    }
}

void errant(void)
{
    uint32_t* p = (uint32_t*)0x20007000;
    while(true)
    {
        while (readPbs() == 32)
        {
            *p = 1;
        }
        yield();
    }
}

void important(void)
{
    while(true)
    {
        lock(resource);
        setPinValue(BLUE_LED, 1);
        sleep(1000);
        setPinValue(BLUE_LED, 0);
        unlock(resource);
    }
}

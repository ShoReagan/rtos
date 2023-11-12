// Memory manager functions
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

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

uint8_t mem_map[32] = {};

uint8_t find_large_block(int size)
{
    static uint8_t i, j, blocks_needed, k;

    if(size % 1024)
        blocks_needed = (size / 1024) + 1;
    else
        blocks_needed = size / 1024;

    for(i = 1; i < 4; i++)
    {
        for(j = 0; j < 8; j++)
        {
            if((mem_map[(i * j) + 8] == 0) && ((8 - j) >= blocks_needed))
            {
                for(k = 0; k < blocks_needed; k++)
                    mem_map[(i * j) + k + 8] = blocks_needed;
                return ((i * j) + 8);
            }
        }
    }
    return 100;
}

uint8_t find_small_block(int size)
{
    static uint8_t i;
    for(i = 0; i < 8; i++)
    {
        if(mem_map[i] == 0)
        {
            mem_map[i] = 1;
            return i;
        }
    }
    return find_large_block(size);
}

// REQUIRED: add your malloc code here and update the SRD bits for the current thread
void * mallocFromHeap(uint32_t size_in_bytes)
{
    static uint8_t index;
    if(size_in_bytes <= 512)
    {
        index = find_small_block(size_in_bytes);
        void *ptr = (void*)(0x20001200 + (0x200 * index));
        return ptr;
    }
    else
    {
        index = find_large_block(size_in_bytes);
        return ((void*)(0x20002400 + (0x400 * (index - 8))));
    }
}

// REQUIRED: add your custom MPU functions here (eg to return the srd bits)
void allowOverallAccess()
{
    //set region and valid bit
    NVIC_MPU_NUMBER_R = 0x0;

    //set base address
    NVIC_MPU_BASE_R |= 0x00000000;

    //set size to max
    NVIC_MPU_ATTR_R |= (0x1F << 1);

    //set RW privileges
    NVIC_MPU_ATTR_R |= (0x3 << 24) | (1u << 28);

    //enable region
    NVIC_MPU_ATTR_R |= 1;
}

void allowFlashAccess()
{
    //set region and valid bit
    NVIC_MPU_NUMBER_R = 0x1;

    //set base address
    NVIC_MPU_BASE_R |= 0x00000000;

    //set SBC bits
    NVIC_MPU_ATTR_R |= (0x2 << 16);

    //set size to 0x3FFFF
    NVIC_MPU_ATTR_R |= (0x11 << 1);

    //set RWX privileges
    NVIC_MPU_ATTR_R |= (0x3 << 24);

    //enable region
    NVIC_MPU_ATTR_R |= 1;
}

void allowPeripheralAccess()
{
    //set region and valid bit
    NVIC_MPU_NUMBER_R = 0x2;

    //set base address
    NVIC_MPU_BASE_R |= 0x40000000;

    //set SBC bits
    NVIC_MPU_ATTR_R |= (0x5 << 16);

    //set size to 0x3FFFF
    NVIC_MPU_ATTR_R |= (0x19 << 1);

    //set RW privileges
    NVIC_MPU_ATTR_R |= (0x3 << 24) | (1u << 28);

    //enable region
    NVIC_MPU_ATTR_R |= 1;
}

void setupSramAccess()
{
    uint8_t i;
    //set region and valid bit
    NVIC_MPU_NUMBER_R = 0x3;

    //set base address
    NVIC_MPU_BASE_R |= 0x20001000;

    //set SBC bits
    NVIC_MPU_ATTR_R = (0x6 << 16);

    //set size to 0x4kB
    NVIC_MPU_ATTR_R |= (0xB << 1);

    //set RW privileges
    NVIC_MPU_ATTR_R |= (0x1 << 24) | (1u << 28);

    //enable region
    NVIC_MPU_ATTR_R |= 1;

    for(i = 0; i < 3; i++)
    {
        //set region and valid bit
        NVIC_MPU_NUMBER_R = (0x4 + i);

        //set base address
        NVIC_MPU_BASE_R |= (0x20002000 + (i * 0x2000));

        //set SBC bits
        NVIC_MPU_ATTR_R |= (0x6 << 16);

        //set size to 0x8kB
        NVIC_MPU_ATTR_R |= (0xC << 1);

        //set RW privileges
        NVIC_MPU_ATTR_R |= (0x1 << 24) | (1u << 28);

        //enable region
        NVIC_MPU_ATTR_R |= 1;
    }
}

void getSrdMask(uint8_t srdMask[4], void *p, uint32_t size_in_bytes)
{
    uint8_t i, startIndex, j, maskedBits;
    uint32_t tempPtr = 0x20001000;

    if((uint32_t)p > tempPtr && (uint32_t)p <= 0x20002000)
    {
        startIndex = ((uint32_t)p - 0x20001200) / 0x200;

        if(size_in_bytes % 0x200)
            maskedBits = (size_in_bytes / 0x200) + 1;
        else
            maskedBits = (size_in_bytes / 0x200);

        for(j = 0; j < maskedBits; j++)
        {
            srdMask[0] |= 1u << (startIndex + j);
        }
    }
    else
        srdMask[0] = 0;

    tempPtr += 0x1000;

    for(i = 0; i < 4 - 1; i++)
    {
        if((uint32_t)p > tempPtr && (uint32_t)p <= (tempPtr + 0x2000))
        {
            startIndex = ((uint32_t)p - (tempPtr + 0x2000)) / 0x400;

            if(size_in_bytes % 0x400)
                maskedBits = (size_in_bytes / 0x400) + 1;
            else
                maskedBits = (size_in_bytes / 0x400);

            for(j = 0; j < maskedBits; j++)
            {
                srdMask[i + 1] |= 1u << (startIndex + j);
            }
        }
        else
            srdMask[i + 1] = 0;

        tempPtr += 0x2000;
    }
}

void setSrdMask(uint8_t srdMask[4])
{
    uint8_t i;
    for(i = 0; i < 4; i++)
    {
        NVIC_MPU_NUMBER_R = (0x3 + i);
        NVIC_MPU_ATTR_R &= ~(0xFF << 8);
        NVIC_MPU_ATTR_R |= srdMask[i] << 8;
    }
}

// REQUIRED: initialize MPU here
void initMpu(void)
{
    // REQUIRED: call your MPU functions here
    allowOverallAccess();
    allowFlashAccess();
    allowPeripheralAccess();
    setupSramAccess();

    NVIC_MPU_CTRL_R |= NVIC_MPU_CTRL_ENABLE;
}


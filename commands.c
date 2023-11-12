#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "tm4c123gh6pm.h"
#include "commands.h"

void reverse(char str[], int length)
{
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        end--;
        start++;
    }
}

char* itoa(uint32_t num, char* str, int base)
{
    int i = 0;
    bool isNegative = false;

    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    if (num < 0 && base == 10) {
        isNegative = true;
        num = -num;
    }

    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    if (isNegative)
        str[i++] = '-';

    str[i] = '\0';

    reverse(str, i);

    return str;
}

int32_t atoi(char *str)
{
    int n = 0;
    int i;
    for (i = 0; str[i] != '\0'; ++i)
            n = n * 10 + str[i] - '0';
    return n;
}

void strcpy(char *s1, char *s2)
{
    uint8_t i;
    for(i = 0; s2[i] != '\0'; i++)
    {
        s1[i] = s2[i];
    }
    s1[i] = '\0';
}

int32_t strcmp(char *str1, char *str2)
{
    int i = 0;
    while(str1[i] != '\0' || str2[i] != '\0')
    {
        if(str1[i] != str2[i]) {
            return 0;
        }
        i++;
    }
    return 1;
}

void getsUart0(USER_DATA *data)
{
    int count = 0;
    char x;
    int i = 1;
    while(i) {
        x = getcUart0();
        if((x == 8 || x == 127) && count > 0)
        {
            count--;
        }
        else if(x == 13)
        {
            data->buffer[count] = '\0';
            i = 0;
        }
        else if(x >= 32)
        {
            data->buffer[count] = x;
            if(count == MAX_CHARS)
            {
                data->buffer[count] = '\0';
                i = 0;
            }
            count++;
        }
    }
}

void parseFields(USER_DATA *data)
{
    int index = 0;
    char temp;
    char delim = 'd';
    data->fieldCount = 0;
    while(data->buffer[index] != '\0')
    {
        if(delim == 'd') {
            temp = delim;
            if((data->buffer[index] >= 65 && data->buffer[index] <= 90) || (data->buffer[index] >= 97 && data->buffer[index] <= 122))
            {
                delim = 'a';
            }
            else if(data->buffer[index] >= 48 && data->buffer[index] <= 57)
            {
                delim = 'n';
            }
            else
                data->buffer[index] = '\0';
            index++;
        }
        else
            delim = 'd';
        if(temp != delim && delim != 'd' && (data->buffer[index-2] == '\0' || index == 0))
        {
            data->fieldPosition[data->fieldCount] = index - 1;
            data->fieldType[data->fieldCount] = delim;
            data->fieldCount++;
        }
    }

}

char* getFieldString(USER_DATA *data, uint8_t fieldNumber)
{
    if(data->fieldCount >= fieldNumber)
    {
        return (data->buffer + data->fieldPosition[fieldNumber]);
    }
    else
        return NULL;
}

int32_t getFieldInteger(USER_DATA *data, uint8_t fieldNumber)
{
    if(data->fieldCount >= fieldNumber && data->fieldType[fieldNumber] == 'n')
    {
        return atoi(data->buffer + data->fieldPosition[fieldNumber]);
    }
    else
        return 0;
}

bool isCommand(USER_DATA *data, char strCommand[], uint8_t minArguments)
{
    if(strcmp(strCommand, data->buffer) && (data->fieldCount >= minArguments))
    {
        return true;
    }
    else
        return false;
}

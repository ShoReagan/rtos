#ifndef COMMANDS_H_
#define COMMANDS_H_

#define MAX_FIELDS 5
#define MAX_CHARS 64

#include <stdint.h>
#include <stdbool.h>

typedef struct _USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
} USER_DATA;

void reverse(char str[], int length);
char* itoa(uint32_t num, char* str, int base);
int32_t atoi(char *str);
int32_t strcmp(char *str1, char *str2);
void strcpy(char *s1, char *s2);
void getsUart0(USER_DATA *data);
void parseFields(USER_DATA *data);
char* getFieldString(USER_DATA *data, uint8_t fieldNumber);
int32_t getFieldInteger(USER_DATA *data, uint8_t fieldNumber);
bool isCommand(USER_DATA *data, char strCommand[], uint8_t minArguments);
int toDeci(char *str, int base);

#endif

#ifndef VCD_H_
#define VCD_H_


#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// I will reorder these some day
#define ERR_FILE_ENDS -1
#define ERR_INVALID_COMMAND -2
#define ERR_NO_END -3
#define ERR_INVALID_TIMESCALE -4
#define ERR_INVALID_UNIT -5
#define ERR_INVALID_VAR_TYPE -6
#define ERR_INVALID_SIZE -7
#define ERR_INVALID_ID -8
#define ERR_INVALID_NAME -9
#define ERR_NO_VARS -10
#define ERR_IDK -11
#define ERR_INVALID_DUMP -12
#define ERR_INVALID_VAR_ID -13
#define ERR_NOT_YET_IMPLEMENTED -14
#define ERR_NO_ENDDEFINITIONS_BEFORE_VALUES -15
#define ERR_VAR_WITH_NO_VALUES -16
#define ERR_VARIABLE_IS_BIT_BUT_VALUE_IS_VECTOR -17
#define ERR_COMMAND_IN_COMMAND -18
#define ERR_NEW_TIME_LESS_THAN_MAX -19
#define ERR_VALUE_LENGTH_MISMATCH -20





#define CHAR_BUF_SIZE 1024
#define COMMAND_COUNT 12
#define MAX_ID_LENGTH 20
#define MAX_NAME_LENGTH 32 // This one is up for debate.

//! STOP BEING DUMB DUDE
//TODO: TOKENIZE THE FUCKING FILE OMFG
//TODO: That would honestly make this so much easier.
// todo: Or maybe!!! Make a function that grabs the next keyword.
// todo: You can even maybe make it with static memory hold on yeah that's possible.


#define TYPE_EVENT      0
#define TYPE_INTEGER    1
#define TYPE_PARAMETER  2
#define TYPE_REAL       3
#define TYPE_REG        4
#define TYPE_SUPPLY0    5
#define TYPE_SUPPLY1    6
#define TYPE_TIME       7
#define TYPE_TRI        8
#define TYPE_TRIAND     9
#define TYPE_TRIOR      10
#define TYPE_TRIREG     11
#define TYPE_TRI0       12
#define TYPE_TRI1       13
#define TYPE_WAND       14
#define TYPE_WIRE       15
#define TYPE_WOR        16


// These have to be dynamic unfortunately.
typedef struct {
    union {
        char *value_string;
        char value_char;
    };
    // If the time values exceed 2^64 something is wrong at the generation side.
    size_t time;
} value_pair_t;


typedef struct {
    char id[MAX_ID_LENGTH];
    char name[MAX_NAME_LENGTH];
    uint8_t type;
    size_t size;
    value_pair_t *values;
    size_t value_count;
    size_t value_index;
} var_t;


typedef struct {
    int8_t timescale_power; // s = 0, ms = -3, 10us = -5, etc
    var_t *vars;
    size_t var_count;
    size_t max_time;
} vcd_t;




vcd_t interpretVCD(FILE *file);
void freeVCD(vcd_t vcd);



#endif
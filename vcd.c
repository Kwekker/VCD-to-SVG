#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "vcd.h"

#define CHAR_BUF_SIZE 1024
#define COMMAND_COUNT 12


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

typedef struct {
    char *id;
    char *name;
    uint8_t type;
    size_t size;
    char *value;
} variable_t;


typedef struct {
    int8_t timescale_power; // s = 0, ms = -3, 10us = -5, etc
} vcd_t;


int handleComment(FILE *file, vcd_t* vcd);
int handleDate(FILE *file, vcd_t* vcd);
int handleEnddefinitions(FILE *file, vcd_t* vcd);
int handleScope(FILE *file, vcd_t* vcd);
int handleTimescale(FILE *file, vcd_t* vcd);
int handleUpscope(FILE *file, vcd_t* vcd);
int handleVar(FILE *file, vcd_t* vcd);
int handleVersion(FILE *file, vcd_t* vcd);
int handleDumpall(FILE *file, vcd_t* vcd);
int handleDumpoff(FILE *file, vcd_t* vcd);
int handleDumpon(FILE *file, vcd_t* vcd);
int handleDumpvars(FILE *file, vcd_t* vcd);


// Expects file to be at the first character of a $command, past the $
int interpretCommand(FILE *file, vcd_t* vcd);

int interpretVCD(char *file_name) {
    FILE* file = fopen(file_name, "r");
    if (file == NULL) return -1;

    vcd_t vcd = {0};

    while(1) {
        int new_char = getc(file);

        if (new_char == EOF) return 0;
        if (new_char == '$') {
            printf("Found a $\n");
            int ret = interpretCommand(file, &vcd);
            if (ret) return ret;
        }

    }

    return 0;
}


int interpretCommand(FILE *file, vcd_t* vcd) {

    // Largest is $enddefinitions so this should be enough.
    char command_buf[20] = {0};
    size_t i = 0;

    while (1) {
        int new_char = getc(file);
        if (isspace(new_char)) break;
        if (new_char == EOF) return ERR_FILE_ENDS;
        command_buf[i] = new_char;

        i++;
        if (i > 14) return ERR_INVALID_COMMAND;
    }


    printf("The command is %s\n", command_buf);


    static const struct {
        char* name;
        int (*handler)(FILE*, vcd_t*);
    } commands[] = {
        {"comment", handleComment},
        {"date", handleDate},
        {"enddefinitions", handleEnddefinitions},
        {"scope", handleScope},
        {"timescale", handleTimescale},
        {"upscope", handleUpscope},
        {"var", handleVar},
        {"version", handleVersion},
        {"dumpall", handleDumpall},
        {"dumpoff", handleDumpoff},
        {"dumpon", handleDumpon},
        {"dumpvars", handleDumpvars}
    };

    for (uint8_t i = 0; i < COMMAND_COUNT; i++) {
        if (strcmp(command_buf, commands[i].name) == 0) {
            printf("Running function %d: %s\n", i, commands[i].name);
            return commands[i].handler(file, vcd);
        }
    }


    return ERR_INVALID_COMMAND;
}


int seekEnd(FILE *file) {

    size_t i = 0;
    char buf[5] = {0};

    while(1) {
        int new_char = getc(file);
        if (new_char == EOF) return ERR_NO_END;

        if (new_char == '$') i = 0;
        if (i < 4) buf[i] = new_char;
        i++;
        if (i == 4 && strcmp(buf, "$end") == 0) return 0;
    }
}

int handleComment(FILE *file, vcd_t* vcd) {
    return seekEnd(file);
}
int handleDate(FILE *file, vcd_t* vcd) {
    return seekEnd(file);
}
int handleEnddefinitions(FILE *file, vcd_t* vcd) {
    return seekEnd(file);
}
int handleScope(FILE *file, vcd_t* vcd) {
    return seekEnd(file); // Not dealing with that yet
}
int handleTimescale(FILE *file, vcd_t* vcd) {
    int c = 0;
    // Seek the number
    do {c = getc(file);} while (isspace(c));
    if (c == EOF) return ERR_FILE_ENDS;


    char buf[5] = {0};
    for (size_t i = 0; i < 3; i++) {
        if (c == EOF) return ERR_FILE_ENDS;
        if (isspace(c)) break;
        buf[i] = c;
        c  = getc(file);
    }

    int base_power = -1;
    static const char values[3][4] = {"1", "10", "100"};
    for (uint8_t i = 0; i < 3; i++) {
        if (strcmp(buf, values[i]) == 0) {
            base_power = i; // 1 => 0, 10 => 1, 100 => 2;
            break;
        }
    }
    if (base_power == -1) return ERR_INVALID_TIMESCALE;

    // Seek the unit
    do {c = getc(file);} while (isspace(c));
    if (c == EOF) return ERR_FILE_ENDS;

    if (c == 's') {
        vcd->timescale_power = 0;
    }
    else {
        char units[] = "munpf";
        char *unit = strchr(units, c);
        if (unit == NULL) return ERR_INVALID_UNIT;
        vcd->timescale_power = 0 - 3 * (1 + unit - units);
    }
    vcd->timescale_power += base_power;

    return seekEnd(file);
}


int handleUpscope(FILE *file, vcd_t* vcd) {
    printf("Found Upscope\n");
    return seekEnd(file);
}


int handleVar(FILE *file, vcd_t* vcd) {
    printf("Found Var\n");
    return seekEnd(file);
}


int handleVersion(FILE *file, vcd_t* vcd) {
    printf("Found Version\n");
    return seekEnd(file);
}


int handleDumpall(FILE *file, vcd_t* vcd) {
    printf("Found Dumpall\n");
    return seekEnd(file);
}
int handleDumpoff(FILE *file, vcd_t* vcd) {
    printf("Found Dumpoff\n");
    return 0;
}
int handleDumpon(FILE *file, vcd_t* vcd) {
    printf("Found Dumpon\n");
    return 0;
}
int handleDumpvars(FILE *file, vcd_t* vcd) {
    printf("Found Dumpvars\n");
    return 0;
}
#include "handlers.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>



static inline int find_option(
    char *options, char *buf, int option_count, int option_length);


// I'm obsessed with using as little dynamic memory as possible.
#define NEXT_TOKEN(max_size, buf, invalid_err)  \
    /*Skip whitespace*/                         \
    do {c = getc(file);} while (isspace(c));    \
    if (c == EOF) return ERR_FILE_ENDS;         \
                                                \
    /*Get the value*/                           \
    char buf[max_size + 1] = {0};               \
    for (uint8_t i = 0; i < 11; i++) {          \
        if (c == EOF) return ERR_FILE_ENDS;     \
        if (isspace(c)) break;                  \
        if (i >= 10) return invalid_err;        \
        buf[i] = c;                             \
        c = getc(file);                         \
    }                                           \



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
    NEXT_TOKEN(3, buf, ERR_INVALID_TIMESCALE);

    char values[3][4] = {"1", "10", "100"};
    int base_power = find_option(values[0], buf, 3, 4);
    if (base_power == -1) return ERR_INVALID_TIMESCALE;

    NEXT_TOKEN(3, unit_buf, ERR_INVALID_UNIT);
    char units[6][3] = {"s", "ms", "us", "ns", "ps", "fs"};
    int unit_power = find_option(units[0], unit_buf, 6, 3);
    if (unit_power == -1) return ERR_INVALID_TIMESCALE;
    unit_power = 0 - unit_power * 3;

    vcd->timescale_power = unit_power + base_power;

    printf("Timescale is %d\n", vcd->timescale_power);

    return seekEnd(file);
}


int handleUpscope(FILE *file, vcd_t* vcd) {
    printf("Found Upscope\n");
    return seekEnd(file);
}


int handleVar(FILE *file, vcd_t* vcd) {
    variable_t var = {0};
    int c = 0;
    NEXT_TOKEN(10, type_buf, ERR_INVALID_VAR_TYPE);

    char types[17][10] = {
        "event","integer","parameter","real","reg","supply0","supply1","time",
        "tri","triand","trior","trireg","tri0","tri1","wand","wire","wor"
    };

    int type = find_option(types[0], type_buf, 17, 10);
    if (type == -1) return ERR_INVALID_VAR_TYPE;
    var.type = type;

    NEXT_TOKEN(20, size_buf, ERR_INVALID_SIZE);
    var.size = strtol(size_buf, NULL, 0);
    if (var.size == 0 || errno) return ERR_INVALID_SIZE;

    NEXT_TOKEN(MAX_ID_LENGTH, id_buf, ERR_INVALID_ID);
    strcpy(var.id, id_buf);

    NEXT_TOKEN(MAX_NAME_LENGTH, name_buf, ERR_INVALID_NAME);
    strcpy(var.name, name_buf);

    return seekEnd(file);
}


int handleVersion(FILE *file, vcd_t* vcd) {
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


static inline int find_option(
    char *options, char *buf, int option_count, int option_length
) {
    for (size_t i = 0; i < option_count; i++) {
        if (strcmp(buf, options + (i * option_length)) == 0) {
            return i;
        }
    }
    return -1;
}
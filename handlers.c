#include "handlers.h"
#include "vcd.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>



static inline int find_option(
    const char *options, char *buf, int option_count, int option_length);

static char *nextArbiLengthToken(FILE *file);
static var_t *getVarByName(vcd_t * restrict vcd, char * restrict name);

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
        if (i >= max_size) return invalid_err;        \
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

    printf("Buf is %s\n", buf);

    char values[3][4] = {"1", "10", "100"};
    int base_power = find_option(values[0], buf, 3, 4);
    if (base_power == -1) return ERR_INVALID_TIMESCALE;

    NEXT_TOKEN(3, unit_buf, ERR_INVALID_UNIT);
    printf("Unit buf is %s\n", buf);
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
    var_t var = {0};
    int c = 0;
    NEXT_TOKEN(10, type_buf, ERR_INVALID_VAR_TYPE);

    static const char types[17][10] = {
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

    vcd->vars[vcd->var_count] = var;
    vcd->var_count++;

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
    // Count the amount of value changes for each variable.

    size_t current_time = 0;

    while(1) {
        char *token = nextArbiLengthToken(file);
        int first_char = tolower(token[0]);
        if (first_char == '#') {
            current_time = strtol(token + 1, NULL, 0);
        }
        else if (first_char == 'b') {
            char *name = nextArbiLengthToken(file);
            if (name == NULL) return ERR_INVALID_VAR_NAME;
            var_t *var = getVarByName(vcd, name);
            if (var == NULL) return ERR_INVALID_VAR_NAME;

            var->values[var->value_count].value_string = token;
            var->values[var->value_count].time = current_time;
            var->value_count++;
            free(name);
        }
        else if (first_char == 'r') return ERR_NOT_YET_IMPLEMENTED;
        else if (strchr("01xz", first_char) != NULL) {
            var_t *var = getVarByName(vcd, token + 1);
            if (var == NULL) return ERR_INVALID_VAR_NAME;

            var->values[var->value_count].value_char = first_char;
            var->values[var->value_count].time = current_time;
            var->value_count++;
            free(var);
        }
        else {
            printf("idk what I'm supposed to do with %c\n", first_char);
            return ERR_INVALID_DUMP;
        }
    }

    return 0;
}


static char *nextArbiLengthToken(FILE *file) {
    int c = 0;
    //Skip whitespace
    do {c = getc(file);} while (isspace(c));
    if (c == EOF) return NULL;

    // Find the length of the token
    long file_pos = ftell(file);
    do {c = getc(file);}
    while(!isspace(c) && c != EOF);
    if (c == EOF) return NULL;

    long token_length = 1 + ftell(file) - file_pos;

    // Go back, create a buffer, and copy the token into it.
    fseek(file, file_pos, SEEK_SET);
    char *ret = malloc(token_length + 1); // +1 for terminating \0.
    fread(ret, 1, token_length, file);
    ret[token_length] = '\0';

    return ret;
}



static inline int find_option(
    const char *options, char *buf, int option_count, int option_length
) {
    for (size_t i = 0; i < option_count; i++) {
        if (strcmp(buf, options + (i * option_length)) == 0) {
            return i;
        }
    }
    return -1;
}


static var_t *getVarByName(vcd_t* restrict vcd, char* restrict name) {
    for (size_t i = 0; i < vcd->var_count; i++) {
        if (strcmp(name, vcd->vars[i].name) == 0)
            return &(vcd->vars[i]);
    }
    return NULL;
}
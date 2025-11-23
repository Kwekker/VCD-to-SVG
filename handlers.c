#include "handlers.h"
#include "vcd.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>



static inline int find_option(
    const char *options, char *buf, int option_count, int option_length
);

static int countAndAllocateValues(FILE *file, vcd_t *vcd);
static char *nextArbiLengthToken(FILE *file);
static int countValues(FILE *file, vcd_t *vcd);
static var_t *getVarById(vcd_t * restrict vcd, char * restrict id);
static int handleDump(FILE *file, vcd_t* vcd);


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

//! Warning: not completely comprehensive, but it is fast.
//  And I don't need a comprehensive one.
// It doesn't work when you're for example looking for
// "mimimum" in the string "mimimimum" but none of the $commands are like that.
int seekString(FILE *file, char* seek) {
    size_t match_count = 0;
    while(1) {
        int new_char = getc(file);
        if (new_char == EOF) return ERR_FILE_ENDS;

        if (new_char == seek[match_count]) {
            match_count++;
        }
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
int handleUpscope(FILE *file, vcd_t* vcd) {
    return seekEnd(file);
}


int handleTimescale(FILE *file, vcd_t* vcd) {
    int c = 0;
    NEXT_TOKEN(5, buf, ERR_INVALID_TIMESCALE);

    // Apparently this value can have a space in between the number and the unit
    // Both are valid, which is a bit of a headache.
    if(buf[0] != '1') return ERR_INVALID_TIMESCALE;
    uint8_t base_power = 0;
    while(buf[base_power + 1] == '0') base_power++;

    char *unit_buf;
    if (isspace(buf[base_power + 1])) {
        NEXT_TOKEN(3, temp_buf, ERR_INVALID_UNIT);
        unit_buf = temp_buf;
    }
    else {
        unit_buf = buf + base_power + 1;
    }

    printf("Unit buf is %s\n", buf);
    char units[6][3] = {"s", "ms", "us", "ns", "ps", "fs"};
    int unit_power = find_option(units[0], unit_buf, 6, 3);
    if (unit_power == -1) return ERR_INVALID_TIMESCALE;
    unit_power = 0 - unit_power * 3;

    vcd->timescale_power = unit_power + base_power;

    printf("Timescale is %d\n", vcd->timescale_power);

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
    int ret = countAndAllocateValues(file, vcd);
    if (ret) return ret;
    return handleDump(file, vcd);
}
int handleDumpoff(FILE *file, vcd_t* vcd) {
    int ret = countAndAllocateValues(file, vcd);
    if (ret) return ret;
    return handleDump(file, vcd);
}
int handleDumpon(FILE *file, vcd_t* vcd) {
    int ret = countAndAllocateValues(file, vcd);
    if (ret) return ret;
    return handleDump(file, vcd);
}

int handleDumpvars(FILE *file, vcd_t *vcd) {
    int ret = countAndAllocateValues(file, vcd);
    if (ret) return ret;
    return handleDump(file, vcd);
}


int handleDump(FILE *file, vcd_t* vcd) {
    size_t current_time = vcd->max_time;

    while(1) {
        char *token = nextArbiLengthToken(file);
        if (token == NULL) return 0;
        int first_char = tolower(token[0]);

        switch(first_char) {
            case '$':
                // $ should mean we found a $end.
                // We should check for this though, just to be sure.
                if (memcmp(token, "$end", 4) != 0) {
                    printf("Uhhh %s is not $end?? While handling.\n", token);
                    return ERR_COMMAND_IN_COMMAND;
                    free(token);
                }
                free(token);
                return 0;

            case '#':
                current_time = strtol(token + 1, NULL, 0);
                if (vcd->max_time > current_time)
                    return ERR_NEW_TIME_LESS_THAN_MAX;
                vcd->max_time = current_time; // Keep track of the max time.
                free(token);
                break;

            case 'b': {
                char *id = nextArbiLengthToken(file);
                if (id == NULL) {
                    free(token);
                    return ERR_INVALID_VAR_ID;
                }

                var_t *var = getVarById(vcd, id);
                if (var == NULL) {
                    free(id);
                    free(token);
                    return ERR_INVALID_VAR_ID;
                }

                if (var->size <= 1) {
                    free(token);
                    free(id);
                    return ERR_VARIABLE_IS_BIT_BUT_VALUE_IS_VECTOR;
                }

                size_t token_length = strlen(token);
                if (token_length != var->size) {
                    printf("Length %zu of value %s: "
                        "does not match expected length of %zu of var %s.\n",
                        token_length, token, var->size, var->id
                    );
                    free(token);
                    free(id);
                    return ERR_VALUE_LENGTH_MISMATCH;
                }

                // Remove the pesky 'b' in front of the value
                memmove(token, token + 1, token_length);
                // I honestly don't think the memory gain of 1 byte per value
                // is worth the performance loss of reallocing every single time

                var->values[var->value_index].value_string = token;
                var->values[var->value_index].time = current_time;
                var->value_index++;
                free(id);
                break;
            }
            case 'r':
                free(token);
                return ERR_NOT_YET_IMPLEMENTED;

            case '0':
            case '1':
            case 'x':
            case 'z': {
                var_t *var = getVarById(vcd, token + 1);
                if (var == NULL) return ERR_INVALID_VAR_ID;

                var->values[var->value_index].value_char = first_char;
                var->values[var->value_index].time = current_time;
                var->value_index++;
                free(token);
                break;
            }
            default:
                // Proper error messages will come one day.
                printf("idk what I'm supposed to do with %c while handling\n",
                    first_char);
                free(token);
                return ERR_INVALID_DUMP;
        }
    }

    return 0;
}


// Perform a counting pass over the variable part of the file to count how many
// value changes each value has to endure.
// This function is very similar to handleDumpVars. Still different enough
// to be its own function though. C is not versatile enough
// to efficiently and readably combine these two functions. (As far as I know.)
static int countValues(FILE *file, vcd_t *vcd) {

    while(1) {

        char *token = nextArbiLengthToken(file);
        if (token == NULL) return 0;
        int first_char = tolower(token[0]);

        switch (first_char) {
            // "mimimi using more than 3 layers of indentation is wrong!!!"
            // Is what people who don't use double indentation in a switch
            // statement like a civilized human being say.
            // K&R can suck it.
            // (Jk of course, K&R may not be disrespected, my apologies.)
            case '$':
                // $ should mean we found a $end.
                // We should check for this though, just to be sure.
                if (memcmp(token, "$end", 4) != 0) {
                    printf("Uhhh %s is not $end?? While counting.\n", token);
                    return ERR_COMMAND_IN_COMMAND;
                    free(token);
                }
                free(token);
                return 0;

            case '#':
                free(token);
                break;

            case 'b': {
                char *id = nextArbiLengthToken(file);
                if (id == NULL) {
                    free(token);
                    return ERR_INVALID_VAR_ID;
                }
                var_t *var = getVarById(vcd, id);
                if (var == NULL) {
                    free(id);
                    free(token);
                    return ERR_INVALID_VAR_ID;
                }
                var->value_count++;
                free(id);
                free(token);
                break;
            }
            case 'r':
                free(token);
                return ERR_NOT_YET_IMPLEMENTED;

            case '0':
            case '1':
            case 'x':
            case 'z': {
                var_t *var = getVarById(vcd, token + 1);
                if (var == NULL) {
                    free(token);
                    return ERR_INVALID_VAR_ID;
                }
                var->value_count++;
                free(token);
                break;
            }
            default:
                printf("idk what I'm supposed to do with %c while counting\n",
                    first_char);
                free(token);
                char *next_token = nextArbiLengthToken(file);
                printf("FYI the next token was %s\n", next_token);
                free(next_token);
                return ERR_INVALID_DUMP;
        }
    }
}

// Wrapper function that uses countValues
// and then allocates the arrays in vcd->vars->valus.
static int countAndAllocateValues(FILE *file, vcd_t *vcd) {
    // Count how many times each value changes.
    long file_pos = ftell(file);
    int ret = countValues(file, vcd);
    if (ret) return ret;
    fseek(file, file_pos, SEEK_SET);


    // Allocate or reallocate all value arrays.
    // This function can be run multiple times if there's multiple
    // dump commands in the file. That's why we need a realloc here.

    for (size_t i = 0; i < vcd->var_count; i++) {
        var_t *var = vcd->vars + i;
        if (var->value_count == 0) continue;

        if (var->values == NULL) {
            var->values = malloc(var->value_count * sizeof(value_pair_t));
        }
        else {
            var->values = realloc(
                var->values, var->value_count * sizeof(value_pair_t)
            );
        }

    }


    // We don't need to allocate the strings inside the value_pair_t's.
    // This is done by the tokenizer later on.
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

    long token_length = ftell(file) - file_pos;
    // ftell returns the position of the character after the one we just read.
    // That is why I'm not using any +1 here.
    // Also the reason why we need to do file_pos -1 in the upcoming fseek.

    // Go back, create a buffer, and copy the token into it.
    fseek(file, file_pos - 1, SEEK_SET);

    char *ret = malloc(token_length + 1); // +1 for terminating \0.
    memset(ret, 'E', token_length + 1);
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


static var_t *getVarById(vcd_t* restrict vcd, char* restrict id) {

    // printf("Looking for var id %s\n", id);

    for (size_t i = 0; i < vcd->var_count; i++) {
        if (strcmp(id, vcd->vars[i].id) == 0)
            return &(vcd->vars[i]);
    }
    return NULL;
}

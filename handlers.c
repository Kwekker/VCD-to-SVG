#include "handlers.h"
#include "vcd.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>



static inline int find_option(
    const char *options, char *buf, int option_count, int option_length
);

static char *nextArbiLengthToken(FILE *file);
static int countValues(FILE *file, vcd_t *vcd);
static var_t *getVarById(vcd_t * restrict vcd, char * restrict id);


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
    int ret = seekEnd(file);
    if (ret) return ret;

    // Count how many times each value changes.
    long file_pos = ftell(file);
    ret = countValues(file, vcd);
    if (ret) return ret;
    fseek(file, file_pos, SEEK_SET);


    // Allocate all value arrays.
    for (size_t i = 0; i < vcd->var_count; i++) {
        var_t *var = vcd->vars + i;
        // printf("\x1b[34mvalue count is %zu\x1b[0m\n", var->value_count);
        if (var->value_count == 0) {
            printf("Found a variable with out any value definitions!\n");
            printf("Var %3zu: %s, %s, %zu, %d\n",i,
                var->id, var->name, var->size, var->type
            );
            return ERR_VAR_WITH_NO_VALUES;
        }
        var->values = malloc(var->value_count * sizeof(value_pair_t));

        // We don't need to allocate the strings inside the value_pair_t's.
        // This is done by the tokenizer later on.

        // Set all variables' value count to 0 so they can be used as index.
        var->value_count = 0;
    }
    vcd->has_allocaded_values = 1;
    return 0;
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
    if (vcd->has_allocaded_values == 0) {
        printf("\x1b[31mNo $enddefinitions before the first value dump!! Aborting!!!\x1b[0m\n");
        return ERR_NO_ENDDEFINITIONS_BEFORE_VALUES;
    }
    printf("Found Dumpall\n");
    return seekEnd(file);
}
int handleDumpoff(FILE *file, vcd_t* vcd) {
    if (vcd->has_allocaded_values == 0) {
        printf("\x1b[31mNo $enddefinitions before the first value dump!! Aborting!!!\x1b[0m\n");
        return ERR_NO_ENDDEFINITIONS_BEFORE_VALUES;
    }
    printf("Found Dumpoff\n");
    return 0;
}
int handleDumpon(FILE *file, vcd_t* vcd) {
    if (vcd->has_allocaded_values == 0) {
        printf("\x1b[31mNo $enddefinitions before the first value dump!! Aborting!!!\x1b[0m\n");
        return ERR_NO_ENDDEFINITIONS_BEFORE_VALUES;
    }
    printf("Found Dumpon\n");
    return 0;
}


int handleDumpvars(FILE *file, vcd_t* vcd) {
    if (vcd->has_allocaded_values == 0) {
        printf("\x1b[31mNo $enddefinitions before the first value dump!! Aborting!!!\x1b[0m\n");
        return ERR_NO_ENDDEFINITIONS_BEFORE_VALUES;
    }
    // Count the amount of value changes for each variable.

    size_t current_time = 0;

    while(1) {
        char *token = nextArbiLengthToken(file);
        if (token == NULL) return 0;
        int first_char = tolower(token[0]);

        if (first_char == '#') {
            current_time = strtol(token + 1, NULL, 0);
            free(token);
        }
        else if (first_char == 'b') {
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

            // Remove the pesky 'b' in front of the value
            memmove(token, token + 1, var->size + 1);
            // I honestly don't think the memory gain of 1 byte per value
            // is worth the performance loss of reallocing every single time.

            // fprintf(stderr, "Setting value string to token. var is %zu\n", var->size);
            var->values[var->value_count].value_string = token;
            var->values[var->value_count].time = current_time;
            var->value_count++;
            free(id);
        }
        else if (first_char == 'r') {
            free(token);
            return ERR_NOT_YET_IMPLEMENTED;
        }
        else if (strchr("01xz", first_char) != NULL) {

            var_t *var = getVarById(vcd, token + 1);
            if (var == NULL) return ERR_INVALID_VAR_ID;

            var->values[var->value_count].value_char = first_char;
            var->values[var->value_count].time = current_time;
            var->value_count++;
            free(token);
        }
        else {
            printf("idk what I'm supposed to do with %c\n", first_char);
            free(token);
            return ERR_INVALID_DUMP;
        }
    }

    return 0;
}


// Perform a counting pass over the variable part of the file to count how many
// value changes each value has to endure.
// This function is very similar to handleDumpVars
static int countValues(FILE *file, vcd_t *vcd) {
    printf("Counting values!!\n");

    while(1) {

        char *token = nextArbiLengthToken(file);
        if (token == NULL) return 0;
        int first_char = tolower(token[0]);

        if (first_char == '#' || first_char == '$') {
            free(token);
        }
        else if (first_char == 'b') {
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
        }
        else if (first_char == 'r') {
            free(token);
            return ERR_NOT_YET_IMPLEMENTED;
        }
        else if (strchr("01xz", first_char) != NULL) {
            var_t *var = getVarById(vcd, token + 1);
            if (var == NULL) {
                free(token);
                return ERR_INVALID_VAR_ID;
            }
            var->value_count++;
            free(token);
        }
        else {
            printf("idk what I'm supposed to do with %c\n", first_char);
            free(token);
            return ERR_INVALID_DUMP;
        }
    }
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

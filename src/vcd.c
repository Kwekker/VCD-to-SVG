#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "vcd.h"
#include "handlers.h"


// Expects file to be at the first character of a $command, past the $
static int interpretCommand(FILE *file, vcd_t* vcd);
static size_t countVars(FILE *file);


vcd_t interpretVCD(FILE *file) {


    vcd_t vcd = {0};

    size_t var_count = countVars(file);
    printf("Counted %zu vars\n", var_count);
    if (var_count == 0) {
        vcd.var_count = ERR_NO_VARS;
        return vcd;
    }

    // Keep this at 0 so that it can be used as an index.
    vcd.var_count = 0;
    // No overflows should be possible because we're allocating enough here.
    vcd.vars = calloc(var_count, sizeof(var_t));

    while(1) {
        int new_char = getc(file);

        if (new_char == EOF) break;
        if (isspace(new_char)) continue;
        if (new_char == '$') {
            int ret = interpretCommand(file, &vcd);
            if (ret) {
                freeVCD(vcd);
                vcd.vars = NULL;
                vcd.var_count = ret;
                return vcd;
            }
        }
        else if (new_char == '#') {
            do {new_char = getc(file);} while(isspace(new_char));
            char token[64] = {0};
            char *c = token;
            while(!isspace(new_char)) {
                *c = new_char;
                new_char = getc(file);
                c++;
            }
            size_t time = strtol(token, NULL, 0);
            if (vcd.max_time > time) {
                printf("New time is %zu, but previous time was %zu.",
                    time, vcd.max_time);
                vcd.vars = NULL;
                vcd.var_count = ERR_NEW_TIME_LESS_THAN_MAX;
                return vcd;
            }
            vcd.max_time = time;
        }
        else {
            // Go back by one because handleValueChange
            // expects to be on the first character.
            fseek(file, -1, SEEK_CUR);
            int ret = handleValueChange(file, &vcd);
            if (ret) {
                vcd.vars = NULL;
                vcd.var_count = ret;
                return vcd;
            }
        }

    }

    printf("\n\n");
    for (size_t i = 0; i < vcd.var_count; i++) {
        var_t var = vcd.vars[i];

        printf("Var %3zu: %s, %s, %zu, %d\n", i, var.id, var.name, var.size, var.type);
        // for (size_t j = 0; j < var.value_count; j++) {
        //     if (var.size == 1)
        //         printf("\t %3zu: %c\n", j, var.values[j].value_char);
        //     else
        //         printf("\t %3zu: %s\n", j, var.values[j].value_string);
        // }
    }

    return vcd;
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
            return commands[i].handler(file, vcd);
        }
    }

    return ERR_INVALID_COMMAND;
}


static size_t countVars(FILE *file) {

    char buf[5] = {0};
    size_t i = 0;
    size_t var_count = 0;

    int c = getc(file);
    while(c != EOF) {
        if (c == '$') i = 0;
        if (i < 4) buf[i] = c;
        if (i == 4 && strcmp(buf, "$var") == 0) {
            var_count++;
        }
        c = getc(file);
        i++;
    }

    if (fseek(file, 0, SEEK_SET)) return ERR_IDK;

    return var_count;
}


void freeVCD(vcd_t vcd) {
    if (vcd.vars != NULL) {
        // Free its scope path.
        if (vcd.current_path != NULL) free(vcd.current_path);

        for (size_t i = 0; i < vcd.var_count; i++) {
            var_t *var = vcd.vars + i;
            if (var->name != NULL) free(var->name);
            if (var->values == NULL || var->copy_of != NULL) continue;
            if (var->size > 1) {
                for (size_t j = 0; j < var->value_count; j++) {
                    if (var->values[j].value_string != NULL) {
                        free(var->values[j].value_string);
                    }
                }
            }
            if (var->value_count) {
                free(var->values);
            }
        }
    }
    free(vcd.vars);
}



var_t *getVarById(vcd_t* restrict vcd, char* restrict id) {

    // printf("Looking for var id %s\n", id);

    for (size_t i = 0; i < vcd->var_count; i++) {
        if (strcmp(id, vcd->vars[i].id) == 0)
            return &(vcd->vars[i]);
    }
    return NULL;
}

var_t* getVarByPath(vcd_t* restrict vcd, char* restrict path) {

    // Pathless search (So "bar" matches "sim/dut/bar").
    if (strchr(path, '/') == NULL) {
        for (size_t i = 0; i < vcd->var_count; i++) {
            var_t *var = vcd->vars + i;
            char *cmp_me = strrchr(var->name, '/') + 1;
            if (strcmp(cmp_me, path) == 0) return var;
        }
    }
    // Search with path (Only match "sim/dut/bar" with "sim/dut/bar").
    else {
        for (size_t i = 0; i < vcd->var_count; i++) {
            var_t *var = vcd->vars + i;
            if (strcmp(var->name, path) == 0) return var;
        }
    }

    return NULL;
}
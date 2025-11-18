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


int interpretVCD(FILE *file) {


    vcd_t vcd = {0};

    size_t var_count = countVars(file);
    printf("Counted %zu vars\n", var_count);
    if (var_count == 0) return ERR_NO_VARS;

    // Keep this at 0 so that it can be used as an index.
    vcd.var_count = 0;
    // No overflows should be possible because we're allocating enough here.
    vcd.vars = malloc(sizeof(var_t) * var_count);

    while(1) {
        int new_char = getc(file);

        if (new_char == EOF) break;
        if (new_char == '$') {
            int ret = interpretCommand(file, &vcd);
            if (ret) {
                freeVCD(vcd);
                return ret;
            }
        }

    }

    printf("\n\n");
    for (size_t i = 0; i < vcd.var_count; i++) {
        var_t var = vcd.vars[i];

        printf("Var %3zu: %s, %s, %zu, %d\n", i, var.id, var.name, var.size, var.type);
        for (size_t j = 0; j < var.value_count; j++) {
            if (var.size == 1)
                printf("\t %3zu: %c\n", j, var.values[j].value_char);
            else
                printf("\t %3zu: %s\n", j, var.values[j].value_string);
        }
    }

    freeVCD(vcd);

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
    if (vcd.has_allocaded_values) {
        for (size_t i = 0; i < vcd.var_count; i++) {
            var_t *var = vcd.vars + i;
            for (size_t j = 0; j < var->value_count; j++) {
                if (var->size > 1 && var->values[j].value_string != NULL) {
                    free(var->values[j].value_string);
                }
            }

            free(var->values);
        }
    }
    free(vcd.vars);
}
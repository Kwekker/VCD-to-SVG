#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "vcd.h"
#include "svg.h"


void handleError(FILE *file, char *file_name, int ret);

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Please at least provide the vcd file.\n");
        return -1;
    }

    char *file_name = argv[1];
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        printf("Couldn't open file.\n");
        return -1;
    }

    vcd_t vcd = interpretVCD(file);
    int ret = 0;
    if (vcd.vars == NULL) ret = vcd.var_count;


    if (ret) {
        handleError(file, argv[1], ret);
        fclose(file);
        return -1;
    }
    fclose(file);

    svg_settings_t settings = initSvgSettings(vcd.var_count);



    FILE *out_file = fopen("out.svg", "w");


    printf("Outputting svg!!\n");
    writeSVG(out_file, vcd, settings);
    freeVCD(vcd);
    free(settings.signals);
    fclose(out_file);

    return 0;
}



void handleError(FILE* file, char* file_name, int ret) {
    printf("\nReturned with \x1b[31m%d\x1b[0m\n", ret);

    long file_pos = ftell(file);

    fseek(file, 0, SEEK_SET);

    // Search for newline before the error file position.
    long last_newline = 0;
    long file_seek_pos = 0;
    // Line counts always start at 1
    // (cringe, but let's follow it)
    long error_line_number = 1;
    int c = 0;
    while (file_seek_pos < file_pos) {
        c = getc(file);
        file_seek_pos++;
        if (c == '\n') {
            error_line_number++;
            last_newline = file_seek_pos;
        }
    }
    printf("\x1b[31mError happened at \x1b[0m%s:%ld:%ld\n",
        file_name, error_line_number, file_pos - last_newline);

#if 0 // This doesn't work and I'm kinda sick of it
    // Find the next newline or the end of the file
    long next_newline = last_newline;
    do {
        c = getc(file);
        next_newline++;
    }
    while (c != EOF && c != '\n');

    fseek(file, last_newline, SEEK_SET);
    long error_line_size = next_newline - last_newline;
    char error_line[error_line_size + 1]; // + 1 for terminating \0.
    fread(error_line, 1, error_line_size, file);
    error_line[error_line_size] = '\0';

    printf("\x1b[33mError happened here, in \x1b[0m%s:%ld\n\t\x1b[41m%s\x1b[0m\n\t\x1b[31m",
        argv[1],
        newline_line,
        error_line
    );
    for (size_t i = 0; i < file_pos - last_newline; i++) {
        printf(" ");
    }
    printf("^\n\n\x1b[0m");
#endif
}
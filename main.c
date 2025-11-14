#include <stdio.h>
#include <stdint.h>

#include "vcd.h"

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Please at least provide the vcd file.\n");
        return -1;
    }

    char *file_name = argv[1];

    int ret = interpretVCD(file_name);
    printf("Returned with %d\n", ret);

    return 0;
}


#include <stdio.h>
#include "operations.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <filename> <command>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    const char *command = argv[2];

    process_sed_command(filename, command);

    return 0;

}

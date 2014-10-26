#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <zlib.h>

int main(int argc, const char *argv[]) {

    if(argc != 2) {
        printf("missing argument !\n");
        exit(EXIT_FAILURE);
    }

    uLong crc = crc32(0L, Z_NULL, 0);

    crc = crc32(crc, (Bytef *) argv[1], strlen(argv[1]));
    printf("CRC: %lu\n", crc);

    return 0;
}

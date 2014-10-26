#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include "rtp.h"

int main(int argc, const char *argv[]) {

    packetstruct *test = (packetstruct*) calloc(1, sizeof(packetstruct));

    test->type = PTYPE_DATA;
    test->window = 5;
    test->seqnum = 8;
    test->length = 16;


    memcpy((void*) test->payload, (void*) "loremIpsum", strlen("loremIpsum"));

    uint32_t crc;

    if(compute_crc(test, &crc)) {
        printf("Error computig crc !\n");
        return -1;
    }

    test->crc = crc;

    printf("Calculated CRC: %u\n", crc);

    printf("First test: %u (expected true)\n", packet_valid(test));

    test->window = 2;

    printf("Second test: %u (expected false)\n", packet_valid(test));

    return 0;
}

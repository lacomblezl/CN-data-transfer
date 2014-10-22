#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
typedef struct __attribute__((packed)){
	uint8_t type : 3;
	uint8_t window : 5;
	uint8_t seqnum : 8;
	uint16_t length : 16;
	uint8_t payload[512];
	uint32_t crc;
}packetstruct;


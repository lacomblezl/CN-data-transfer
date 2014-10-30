/*
 *  rtp.c (Reliable Transfer Protocol)
 *
 *  INGI2141 - Computer Networks
 *
 *  AUTHORS :   Lacomblez Loic (2756-11-00)
 *              Laurent Quentin (...)
 *
 *  DATE : october 2014
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "rtp.h"


/* initializes a connection's resources */
int init_host(struct addrinfo *address, enum host_type type) {

    // instanciate the socket
    int sock_id = socket(address->ai_family, address->ai_socktype,
      address->ai_protocol);
    if(sock_id < 0) {
        return -1;
    }

    /* If type is sender, connect socket to the destination */
    if(type == sender) {
        if(connect(sock_id, address->ai_addr, address->ai_addrlen)) {
            close(sock_id);
            return -1;
        }
    }

    /* Else (type is receiver), bind socket to the address specified */
    else {
        if(bind(sock_id, address->ai_addr, address->ai_addrlen)) {
            close(sock_id);
            return -1;
        }
    }

    return sock_id;
}


/* Establishes the connection as specified by the protocol */
int connect_up(int sock_id) {
    //TODO: routine de negociation du sequence number...
    return 0;
}


/*
 * Computes the CRC given the following data and binds it to result
 * Returns 0, or -1 if an error was encountered
 */
int compute_crc(packetstruct* packet, uint32_t *result) {

    // The number of bytes on which the CRC must be applied
    size_t len = PAYLOADSIZE + 4;

    // Allocate space to store the buffer to wich CRC is applied
    Bytef *buffer = malloc(len*sizeof(Bytef));
    if(buffer == NULL) {
        return -1;
    }

    // Fill buffer with the frame content (not the CRC part) !
    memcpy((void*) buffer, (void*) packet, len);

    // Compute CRC
    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, buffer, len);

    // Bind the result
    *result = (uint32_t) crc;

    free(buffer);
    return 0;
}


/*
 * Checks the packet's CRC after doing an endianness correction
 * TODO: convert endianness !!
 */
int packet_valid(packetstruct* packet) {

    //Convert CRC endianness
    packet->crc = ntohl(packet->crc);

    // Compute crc and return the comparison result
    uint32_t crc;
    if(compute_crc(packet, &crc)) {
        return -1;
    }
    // Convert Length
    packet->length = ntohs(packet->length);

    return (crc == packet->crc);

    /*
    // The number of bytes on which the CRC must be applied
    size_t len = PAYLOADSIZE + 4;

    // Allocate space to store the buffer to wich CRC is applied
    Bytef *buffer = malloc(len*sizeof(Bytef));
    if(buffer == NULL) {
        return -1;
    }

    // Fill buffer with the frame content (not the CRC part) !
    memcpy((void*) buffer, (void*) packet, len);

    // Compute CRC
    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, buffer, len);

    free(buffer);

    // Check the result
    */
}

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
 * Computes the CRC given the following data
 */
int compute_crc(char* header, char* payload)
{
    //TODO implement crc...
    return 0;
}

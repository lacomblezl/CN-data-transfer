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

#include "rtp.h"


#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/socket.h>


struct addrinfo *src_address, *dest_address;

/* initializes a connection's resources */
int init_host(struct addrinfo *dest_addr, struct addrinfo *src_addr) {
    /*TODO: needed by sender...
    *   - a socket id (initialized !!!)
    *   - a sending port and address (using CONNECT !!)
    *   => ONLY A WELL CONFIGURED SOCKET NEEDED...
    */

    /*TODO: needed by receiver...
    *   - a socket id (initialized !!!)
    *       -> define hints in global scope
    *   - a listening port and address (using BIND !!)
    *   => ONLY A WELL CONFIGURED SOCKET NEEDED...
    */
}


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

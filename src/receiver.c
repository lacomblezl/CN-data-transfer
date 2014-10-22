/*
 *  receiver.c
 *
 *  INGI2141 - Computer Networks
 *
 *  AUTHORS :   Lacomblez Loic (2756-11-00)
 *              Laurent Quentin (4834-11-00)
 *
 *  DATE : october 2014
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "rtp.h"


#define IP_PROT PF_INET6            // defines the ip protocol used (IPv6)
#define BUFFSIZE 10                 // size of the receiving buffer


struct addrinfo *address = NULL;    // address & port we're listening to
struct addrinfo hints = {
    .ai_family = IP_PROT,
    .ai_socktype = SOCK_DGRAM,
    .ai_protocol = IPPROTO_UDP };

//FIXME: definition de window commune a send et receive non ?
typedef struct slot {
    uint8_t seqnum;
    bool received;
} window_slot;
window_slot *window;                // The sliding window

int sock_id;                        // The socket used by the program
packetstruct recv_buffer[BUFFSIZE]; // receiving buffer


// Prints the function usage
void usage() {
    printf("usage:\n");
    printf("\t./receiver [--file filename] hostname port\n\n");
}

// Cleanly exits the application in case of error
void die(char *error_msg) {
    perror(error_msg);
    freeaddrinfo(address);
    close(sock_id);
    //TODO: close(fd);
    exit(EXIT_FAILURE);
}


/*
 * Flushes the content of receiving buffer to file fd.
 * Updates the window accordingly to accept new sequence numbers.
 * Sets lastack to its new value.
 */
int flush_frames(int fd, uint8_t *lastack) {

    // iterates over the sliding window slots
    int i;
    uint8_t j=0;
    for(i = 0; i < BUFFSIZE; i++) {
        // We found the first unacked packet
        if(window[i].seqnum == (*lastack) + 1) {
            j = i;
        }
    }

    // iterates from the first unacked
    while(window[j % BUFFSIZE].received) {

        //TODO: write to file

        window[j % BUFFSIZE].seqnum = window[j % BUFFSIZE].seqnum + BUFFSIZE;
        window[j % BUFFSIZE].received = false;

        j++;
    }

    *lastack = j-1;

    return 0;
}


/*
 * Checks if the seq_numb is in the sliding window. If so, returns it's index
 * into the window. If not, returns -1.
 */
int idx_in_window(uint8_t seqnumb) {
    int i;
    for(i = 0; i < BUFFSIZE; i++) {

        // If seq number is in window and the packet wasn't received yet
        if((window[i].seqnum == seqnumb) && !(window[i].received)) {
            return i;
        }
    }
    return -1;
}



int main(int argc, const char** argv) {

    int sock_id;                        // socket descriptor
    int fd;                             // file descriptor for the output
    struct sockaddr_storage src_host;   // source host emitting the packets
    packetstruct tmp_packet;            // stores the just received packet
    uint8_t lastack;                    // last in-sequence acknowledge packet


    //TODO: getOpt !
    /* Provisoire... */
    const char *port = argv[1];


    /* Resolve the address passed to the program */
    //FIXME: faire dependre de argv !
    int result;
    if((result = getaddrinfo("::", port, &hints, &address)) < 0) {
      printf("Error resolving address %s - code %i", argv[1], result);
      freeaddrinfo(address);
      exit(EXIT_FAILURE);
    }


    /* initialize the socket used by the receiver */
    sock_id = init_host(address, receiver);
    if(sock_id == -1) {
        perror("Error creating socket");
        freeaddrinfo(address);
        exit(EXIT_FAILURE);
    }

    // FIXME: verbose print
    printf("Socket initialization passed !\n");

    //FIXME: open file or use standard ouput according to params...
    fd = STDOUT_FILENO;


    window = (window_slot *) calloc(BUFFSIZE, sizeof(window_slot));
    if(window == NULL) {
        die("Error assigning receive window");
    }
    //FIXME: memory leak possible !


    //TODO: establish connection ! (avec boucle while)
    // Quand on sort de la, la window est initialisee avec tous les seq numb
    // attendus ! Pour l'instant, on va tester en connectionless.


    int idx;            // index used serveral times in each iteration
    socklen_t src_len;     // size of the source address
    while(1) {

        /* blocking receive - we are waiting for a frame */
        src_len = sizeof(src_host);
        if(recvfrom(sock_id, (void *) &tmp_packet, sizeof(tmp_packet), 0,
            (struct sockaddr*) &src_host, &(src_len)) < 0) {
            free(window);
            die("Error while receiving packet");
        }

        //TODO: check packet !

        if((idx = idx_in_window(tmp_packet.seqnum)) != 1) {
            recv_buffer[idx] = tmp_packet;  // copy packet to rcv_buffer
            window[idx].received = true;    // mark the frame as received
        }

        //TODO: flush packets function call (updates file, buffer, window)
        flush_frames(fd, &lastack); 

        //TODO: send ack(lastack)
    }


    freeaddrinfo(address);
    close(sock_id);
    exit(EXIT_SUCCESS);
}

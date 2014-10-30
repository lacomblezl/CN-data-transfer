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
#include <getopt.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <zlib.h>

#include "rtp.h"


#define IP_PROT PF_INET6            // defines the ip protocol used (IPv6)
#define BUFFSIZE 31                 // size of the receiving buffer
#define SEQSPAN 256
#define PAYLOADSIZE 512
#define HEADERSIZE 4
#define CRCSIZE 4

struct addrinfo *address = NULL;    // address & port we're listening to
struct addrinfo hints = {
    .ai_family = IP_PROT,
    .ai_socktype = SOCK_DGRAM,
    .ai_protocol = IPPROTO_UDP };
struct sockaddr_storage src_host;   // source host emitting the packets
socklen_t src_len;                  // size of the source address

// definition d'un des slots constituant la window
typedef struct slot {
    uint8_t seqnum;
    bool received;
} window_slot;
window_slot window[BUFFSIZE];       // The sliding window

int sock_id;                        // The socket used by the program
packetstruct recv_buffer[BUFFSIZE]; // receiving buffer
int fd;                             // file descriptor for the output file
char *filename = NULL;              // file name for the output
bool verbose;                       // verbose flag to print debug messages


// Prints the function usage
void usage() {
    printf("usage:\n");
    printf("\t./receiver [--verbose] [--file filename] hostname port\n\n");
}

// Cleanly exits the application in case of error
void die(char *error_msg) {
    perror(error_msg);
    freeaddrinfo(address);
    close(sock_id);
    close(fd);
    exit(EXIT_FAILURE);
}


/*
 * Flushes the content of receiving buffer to file fd.
 * Updates the window accordingly to accept new sequence numbers.
 * Sets lastack to its new value.
 * TODO: le bufferFill enleve la necessite du received !!
 */
int flush_frames(int fd, int *lastack, int *bufferPos, int *bufferFill) {

    // iterates over the sliding window slots

	uint8_t i=0;
    int length;
	while(i < BUFFSIZE && window[*bufferPos].received) {

        //int length = ntohs(recv_buffer[*bufferPos].length);
        length = recv_buffer[*bufferPos].length;
        
        if(write(fd, &(recv_buffer[*bufferPos].payload), length) != length) {
            return -1;
        }
        
        // Free space to welcome another packet
        window[*bufferPos].received = 0;
        window[*bufferPos].seqnum = (window[*bufferPos].seqnum+BUFFSIZE)%SEQSPAN;
        
		*lastack = (*lastack + 1)%SEQSPAN;
		*bufferPos = (*bufferPos + 1)%BUFFSIZE;
		*bufferFill = *bufferFill - 1;
		i++;
    }
	return 0;
}


/*
 * Checks if the sequence number 'seq_numb' is in the sliding window.
 * If so, returns it's index into the actual list representing the window.
 * If not, returns -1.
 */
int idx_in_window(uint8_t seqnumb, int lastack, int bufferPos) {

    //TODO: modulo necessaire ? int diff = (seqnumb-lastack+SEQSPAN)%SEQSPAN;
    int diff = seqnumb-lastack;
	if(diff <= 0 || diff > BUFFSIZE) {
		return -1;
	}
	else {
		return (bufferPos+diff-1)%BUFFSIZE;
	}
}


/*
 * Generates an rtp packet acknowledging the sequence number seqnumb.
 * FIXME: window de taille fixe pour l'instant
 */
void acknowledge(int lastack) {
    
    packetstruct packet;

	packet.type = PTYPE_ACK;
	packet.window = BUFFSIZE;
    packet.seqnum = lastack + 1;    // idx of the next expected packet
	packet.length = htons(0);

    // set the payload to zero
    memset(packet.payload, 0, PAYLOADSIZE);

    uint32_t crc;
    if(compute_crc(&packet, &crc)) {
        die("Error computing CRC");
    }

	packet.crc = crc;

	ssize_t lensent = sendto(sock_id, &packet,sizeof(packetstruct),0,
                                    (struct sockaddr *) &src_host, src_len);
	if(lensent != sizeof(packetstruct)) {
		die("Mismatch in number of sent bytes");
	}
}


/*
 * Checks if all the file content has been received
 * RETURN:
 *      true if the file has entirely been received, false otherwise.
 *
 * ARGUMENTS:
 *      - last_received : true if a packet smaller than PAYLOAD has been recvd.
 *      - bufferFill : Number of received packet among the seq. numbers in the
                       the window.
 *      - bufferPos : Index of the sequence number following 'lastack' in
 *                    the receiving window.
 */
int isReceived(bool last_received, int bufferFill, int bufferPos) {

    if(!last_received) {
        return 0;
    }

    int i =0;
	int allackedwindow = 1;
	//printf("\nValeur de bufferFill : %d \n Valeur de size : %d \n Valeur de size!=PAYL : %d \n",bufferFill,(int)size,(size!=PAYLOADSIZE));

    while(i < bufferFill && allackedwindow) {
		allackedwindow = (window[(bufferPos-bufferFill+i+BUFFSIZE)%BUFFSIZE].received);
		i++;
		//printf("Valeur de allacked : %d\n",allackedwindow);
	}
	int istransm = allackedwindow;

    //printf("Istransmitted ? : %d\n",istransm);
    return istransm;
}


/*
 * Formate les options passees au programme
 */
void map_options(int argc, char **argv, int *opts) {

    fd = STDOUT_FILENO;
    verbose = false;

    /* options descriptor */
    static struct option longopts[] = {
        { "file",  required_argument,    NULL,     'f' },
        {"verbose",no_argument,          NULL,     'v'},
        { NULL,    0,                    NULL,     0 }
    };

    int ch;
    while ((ch = getopt_long(argc, argv, "vf:", longopts, NULL)) != -1) {
        switch (ch) {
        case 'v':
            verbose = true;
            break;

        case 'f':
            filename = optarg;
            fd = -1;
            break;

        default:
            usage();
        }
    }
    *opts = optind;
}



int main(int argc, char* argv[]) {

    // Checks the program parameters
    int opt;
    map_options(argc, argv, &opt);
    argc -= optind;
    argv += optind;
    if(argc != 2) {
        usage();
        exit(EXIT_FAILURE);
    }

    // Destination file opening
    if(fd == -1) {
        if((fd = open(filename, O_RDWR|O_CREAT, S_IRWXU|S_IRWXG)) < 0) {
            die("Error opening destination file");
        }
    }

    // Port and adresses string init
    char *addr_str = argv[0];
    char *port_str = argv[1];

    if(verbose) {
        printf("\nListenning address \'%s\' on port \'%s\'\n", addr_str,
        port_str);
    }

    // Resolve the address passed to the program
    int result;
    if((result = getaddrinfo(addr_str, port_str, &hints, &address)) < 0) {
        printf("Error resolving address %s - code %i", addr_str, result);
        freeaddrinfo(address);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // initialize the socket used by the receiver
    sock_id = init_host(address, receiver);
    if(sock_id == -1) {
        die("Error creating socket");
    }
    
    packetstruct tmp_packet;           // stores the just received packet
                                       // TODO: uint8_t lastack=SEQSPAN-1;         // last in-sequence acknowledge packet
    int lastack = -1;
    int bufferPos = 0;                 // Corresponding position in the buffer
                                       // TODO : changer en unsigned 8, voir si lastack doit avoir une valeur particulière
    bool lastPacketReceived = false;   // True if a packet with less than 512B
                                       // of paylod has been received.

    int bufferFill = 0;     //TODO: nb of real packets received ??
    int idx;                //TODO:index used serveral times in each iteration
    ssize_t size = PAYLOADSIZE;     // Size of the received payload
    int is_valid;

    while( !isReceived(lastPacketReceived, bufferFill, bufferPos) ) {

        //FIXME: virer printf("enter while\n");

        /* blocking receive - we are waiting for a frame */
        src_len = sizeof(src_host);
        if(recvfrom(sock_id, (void *) &tmp_packet, sizeof(tmp_packet), 0,
            (struct sockaddr*) &src_host, &(src_len)) != sizeof(packetstruct)) {
                //TODO: on serait pas plus soft ??
                die("Error while receiving packet");
        }

        is_valid = packet_valid(&tmp_packet);
        
        /* only if the packet is valid */
        if(is_valid == 1) {

            size = tmp_packet.length;

            if(verbose) {
                printf("Received a %zd-byte type %u packet (seq %u)\n", size,
                tmp_packet.type, tmp_packet.seqnum);
            }

            // Is the sequence number in the receive window ?
            if((idx = idx_in_window(tmp_packet.seqnum,lastack,bufferPos)) != -1) {
                recv_buffer[idx] = tmp_packet;  // copy packet to rcv_buffer
                window[idx].received = true;    // mark the frame as received
                bufferFill++;                   // add 1 more received packet

                /* If this is the last packet from the original file
                 * (ie, payload with a size smaller than 512 Bytes */
                 if(size < PAYLOADSIZE) {
                     //FIXME: debug print
                     printf("Smaller PAYLOADSIZE\n");
                     lastPacketReceived = true;
                 }

                // Try to empty the in-sequence received packets
                if(flush_frames(fd, &lastack, &bufferPos, &bufferFill)) {
                    die("Error writing packets to file");
                }

                // Send an acknowledgement
                acknowledge(lastack);
            }
            // Else, we do nothing and discard it...
        }
        
        // If the packet is not valid
        else if(verbose) {
            printf("Received a corrupted packet !\n");
        }

    }

    //sleep(10);// FIXME: Comment terminer l'envoi
    if(verbose) {
        printf("File successfully received\n");
    }

    freeaddrinfo(address);
    close(sock_id);
    exit(EXIT_SUCCESS);
}

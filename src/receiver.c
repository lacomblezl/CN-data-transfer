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
#include "rtp.h"


#define IP_PROT PF_INET6            // defines the ip protocol used (IPv6)
#define BUFFSIZE 10                 // size of the receiving buffer
#define MAXSEQ 20
#define PAYLOADSIZE 512
#define HEADERSIZE 4
#define CRCSIZE 4

struct addrinfo *address = NULL;    // address & port we're listening to
struct addrinfo hints = {
    .ai_family = IP_PROT,
    .ai_socktype = SOCK_DGRAM,
    .ai_protocol = IPPROTO_UDP };
struct sockaddr_storage src_host;   // source host emitting the packets
socklen_t src_len;      // size of the source address
//FIXME: definition de window commune a send et receive non ?
typedef struct slot {
    uint8_t seqnum;
    bool received;
} window_slot;
window_slot window[BUFFSIZE];                // The sliding window

int sock_id;                        // The socket used by the program
packetstruct recv_buffer[BUFFSIZE]; // receiving buffer
int fd;                             // file descriptor for the output file
char *filename;                     // file name for the output
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
    //TODO: close(fd);
    exit(EXIT_FAILURE);
}


/*
 * Flushes the content of receiving buffer to file fd.
 * Updates the window accordingly to accept new sequence numbers.
 * Sets lastack to its new value.
 */
int flush_frames(int fd, uint8_t *lastack, int *bufferPos,int *bufferFill) {

    // iterates over the sliding window slots
    
    uint8_t i=0;
	while(i<BUFFSIZE && window[(*bufferPos)%BUFFSIZE].received){
		int length = recv_buffer[*bufferPos].length;
        	if(write(fd, &(recv_buffer[*bufferPos].payload), length) != length) {
            		return -1;
        	}
		*lastack = (*lastack+1)%MAXSEQ;
		*bufferPos = (*bufferPos+1)%BUFFSIZE;
		*bufferFill = *bufferFill-1;
		window[*bufferPos].received = 0;
		window[*bufferPos].seqnum = (window[*bufferPos].seqnum+BUFFSIZE)%MAXSEQ;
		i++;
        }
    return 0;
}


/*
 * Checks if the seq_numb is in the sliding window. If so, returns it's index
 * into the window. If not, returns -1.
 */
int idx_in_window(uint8_t seqnumb, int lastack, int bufferPos) {
	int diff = (seqnumb-lastack+MAXSEQ)%MAXSEQ;
	//printf("seqnumb : %d , lastack : %d, bufferPos : %d, idx : %d",seqnumb,lastack,bufferPos,(bufferPos+diff)%BUFFSIZE);
	if(diff<=0 || diff>BUFFSIZE){
		return -1;
	}
	else{
		return (bufferPos+diff-1)%BUFFSIZE;
	}
}


/*
 * Generates an rtp packet acknowledging the sequence number seqnumb
 * TODO: je fais quoi de la partie window ?
 */
void acknowledge(int lastack, packetstruct *packet) {

    packet->type = PTYPE_ACK;
    packet->window = BUFFSIZE; //TODO: quoi mettre ?
    packet->seqnum = lastack;
    packet->length = 0;
    //FIXME: mettre le payload a zero !!
    packet->crc = 0; //TODO: compute CRC
	
	ssize_t lensent = sendto(sock_id,packet,sizeof(packetstruct),0,(struct sockaddr *)&src_host,src_len);
	if(lensent != sizeof(packetstruct)) {
		die("Mismatch in number of sent bytes");
	}
	//printf("\nAcknowledgement sent with seq number %d\n",lastack);
}
/*
* 1 si tout est reçu, 0 sinon
*/
int isReceived(ssize_t size, int bufferFill, int bufferPos){
int i =0;
	int allackedwindow = 1;
	printf("\nValeur de bufferFill : %d \n Valeur de size : %d \n Valeur de size!=PAYL : %d \n",bufferFill,(int)size,(size!=PAYLOADSIZE));
	while(i<bufferFill && allackedwindow){
		allackedwindow = (window[(bufferPos-bufferFill+i+BUFFSIZE)%BUFFSIZE].received);
		i++;
		//printf("Valeur de allacked : %d\n",allackedwindow);
	}
	int istransm = (size!=sizeof(packetstruct)) && allackedwindow;
	printf("Istransmitted ? : %d\n",istransm);
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

    
    packetstruct tmp_packet;            // stores the just received packet
    uint8_t lastack=MAXSEQ-1;                  // last in-sequence acknowledge packet
	int bufferPos = 0;		//Corresponding position in the buffer
	// TODO : changer en unsigned 8
    int opt;
    map_options(argc, argv, &opt);
    argc -= optind;
    argv += optind;

    if(argc != 2) {
        usage();
        exit(EXIT_FAILURE);
    }

    //TODO: getOpt !
    /* Provisoire... */
    char *addr_str = argv[0];
    char *port_str = argv[1];

    if(verbose) {
        printf("\nListenning address \'%s\' on port \'%s\'\n", addr_str,
                                                                port_str);
    }

    /* Resolve the address passed to the program */
    //FIXME: faire dependre de argv !
    int result;
    if((result = getaddrinfo(addr_str, port_str, &hints, &address)) < 0) {
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

    //TODO: open da fuckin' file !!


   /* A supprimer window = (window_slot *) calloc(BUFFSIZE, sizeof(window_slot));
    if(window == NULL) {
        die("Error assigning receive window");
    }*/
    //FIXME: memory leak possible !


    //TODO: establish connection ! (avec boucle while)
    // Quand on sort de la, la window est initialisee avec tous les seq numb
    // attendus ! Pour l'instant, on va tester en connectionless.

	int bufferFill=0;
    int idx;                // index used serveral times in each iteration
    ssize_t size =sizeof(packetstruct);
    while(!isReceived(size,bufferFill,bufferPos)) {
        /* blocking receive - we are waiting for a frame */
        src_len = sizeof(src_host);
        if((size=recvfrom(sock_id, (void *) &tmp_packet, sizeof(tmp_packet), 0,
            (struct sockaddr*) &src_host, &(src_len))) < 0) {
	// TODO : src_host a une valeur?
            //free(window);
            die("Error while receiving packet");
        }
        //TODO: check packet !

        /* FIXME: verbose print */
        if(verbose) {
            printf("Received a %u-byte data packet (seq %u)\n",
                                tmp_packet.length, tmp_packet.seqnum);
        }

        // Is the sequence number in the receive window ?
	//printf("Avant idx_in_window\n");
        if((idx = idx_in_window(tmp_packet.seqnum,lastack,bufferPos)) != -1) {
	//printf("Value of lastack before flush_frames : %d\n",lastack);
	// flush packets and update window/lastack
	recv_buffer[idx] = tmp_packet;  // copy packet to rcv_buffer
	//printf("%s\n",(char *)&(recv_buffer[idx].payload));
	// TODO : gérer un paquet en dehors de la fenêtre
            window[idx].received = true;    // mark the frame as received
	bufferFill++;
        if(flush_frames(fd, &lastack,&bufferPos,&bufferFill)) {
            die("Error writing packets to file");
        }

		acknowledge(lastack,&recv_buffer[idx]);
        }
        

        //TODO: send ack(lastack)
        /*if(sendto(sock_id,... )) {
            die("Error while sending acknowledgement");
        }*/
	}
	printf("File successfully received\n");

	freeaddrinfo(address);
	close(sock_id);
	exit(EXIT_SUCCESS);
}

/*
 *  sender.c
 *
 *  INGI2141 - Computer Networks
 *
 *  AUTHORS :   Lacomblez Loic (2756-11-00)
 *              Laurent Quentin (4834-11-00)
 *
 *  DATE : october 2014
 */

#include "rtp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdbool.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <zlib.h>

#define PAYLOADSIZE 512
#define HEADERSIZE 4
#define CRCSIZE 4
#define SEQSPAN 256
#define TIMEOUT 100                 // Timer de chaque paquet en msec
#define DEFAULTBUFFSIZE 31          // size of the buffer
#define MAXBUFFSIZE 31

#define IP_PROT PF_INET6            // defines the ip protocol used (IPv6)
struct sockaddr_storage src_host;   // source host emitting the packets
socklen_t src_len;                  // size of the source address

// address & port we're sending to
struct addrinfo *address = NULL;
struct addrinfo hints = {
    .ai_family = IP_PROT,
    .ai_socktype = SOCK_DGRAM,
    .ai_protocol = IPPROTO_UDP };

int BUFFSIZE = DEFAULTBUFFSIZE;
window_slot window[MAXBUFFSIZE];
packetstruct send_buffer[MAXBUFFSIZE];
packetstruct ackBuffer;
int sock_id;
int fileDescriptor;
char *filename;
bool lastseqsent = 0;

// Arguments passed to the program
struct options {
    char* filename;     // The file to send
    int sber;           // Byte Error Rate [per thousand]
    int splr;           // Packet Loss Ratio [%]
    int delay;          // Delay before packet transmission [ms]
    int verbose;
} opts;

/*
* Cleanly exits the application in case of error
*/
void Die(char* error_msg) {
    perror(error_msg);
    freeaddrinfo(address);
    close(sock_id);
    close(fileDescriptor);
    exit(EXIT_FAILURE);
}


/*
 * Inserts the data with header and crc in the sending_buffer at the right place
 * and returns the size of the data read in the file
 * Modifies 'seq' : increments of 1
 * Modifies 'bufferPos' : increments of 1 % BUFFSIZE
 * Modifies 'bufferFIll' : incremente de 1
 * The window is also updated (seqnum=seq and received=false)
 *
 */
int insert_in_buffer(int *seq, int *bufferPos, int *bufferFill) {
    
    if (*bufferFill >= BUFFSIZE) {
        Die("Da buffer is-a full");
    }

    if (window[*bufferPos].seqnum == *seq) {
        return ntohs(send_buffer[*bufferPos].length);
    }
    
    void *payloadaddress = &(send_buffer[*bufferPos].payload);
    
    ssize_t size = read(fileDescriptor, payloadaddress, PAYLOADSIZE);
    
    // Filling the buffer
    send_buffer[*bufferPos].type = PTYPE_DATA;
    send_buffer[*bufferPos].window = 0;
    send_buffer[*bufferPos].seqnum = *seq;
    send_buffer[*bufferPos].length = htons(size);
    
    // Compute CRC
    uint32_t crc;
    if(compute_crc(&send_buffer[*bufferPos], &crc)) {
        Die("Error computing crc");
    }
    send_buffer[*bufferPos].crc = htonl(crc);
    
    // Updates the window info
    window[*bufferPos].seqnum = *seq;
    window[*bufferPos].received = false;
    
    // Updates the description variables of the window and sequence
    *bufferPos = (*bufferPos+1)%MAXBUFFSIZE;
    *bufferFill = *bufferFill+1;
    *seq = (*seq+1)%SEQSPAN;
    return size;
}


/*
 * Fonction which sends the packet with paquetseq sequence number
 * (It has to be in the buffer already)
 */
int supersend(int bufferPos, int bufferFill, int seq, int paquetseq) {
    
	int diff = (seq-paquetseq+SEQSPAN)%SEQSPAN;
	if(diff > bufferFill) {
		Die("");
	}
	int packetbufferindex = (bufferPos-diff+MAXBUFFSIZE)%MAXBUFFSIZE;
	void *bufaddress = &send_buffer[packetbufferindex];
    
    
    // A local buffer to corrupt the packet if needed
    unsigned char* corrupted = malloc(sizeof(packetstruct));
    if(corrupted == NULL) {
        Die("Error allocating corrupted buffer");
    }
    
    memcpy(corrupted, bufaddress, sizeof(packetstruct));
    
    // Packet loss simulation(--splr)
    if (random()%100 < opts.splr) {
        window[packetbufferindex].timesent=clock();
        free(corrupted);
        return sizeof(packetstruct);
    }

    // Simulation of byte errors during transmission (--sber)
    if (random()%1000 < opts.sber) {
        corrupted[random()%(PAYLOADSIZE+8)] ^= 0x69;
    }
    
	//Delaying
    if(usleep(opts.delay*1000)) {
    	free(corrupted);
        Die("Error while waiting before send");
    }
    
    
    // Send the packet to the destination
	ssize_t lensent = send(sock_id, corrupted, sizeof(packetstruct), 0);
	window[packetbufferindex].timesent=clock();

	if(lensent != sizeof(packetstruct)) {
		free(corrupted);
		Die("Error sending packets");
	}
    
    free(corrupted);
	return lensent;
}


/*
 * Function that removes all the packets before ackedframe from the buffer
 */
int remv_from_buffer(int bufferPos, int *bufferFill, int seq, int *unack, int ackedframe) {
    
    int diff = (seq-ackedframe+1+SEQSPAN)%SEQSPAN;
    
    if(diff > *bufferFill) {
        if(opts.verbose){
        	printf("Ack is out of window : (diff=%d), (bufFill=%d), (acked=%d), (seq=%d)\n", diff, *bufferFill, ackedframe, seq);
        }
        return 1;
    }
    int i = 0;
    // Remove acked packets from the buffer
    while(i <= *bufferFill-diff) {
        window[(bufferPos-*bufferFill+i+MAXBUFFSIZE)%MAXBUFFSIZE].received = true;
        *unack=(*unack+1)%SEQSPAN;
        *bufferFill=*bufferFill-1;
        if(opts.verbose){
        	printf("Last unacked frame = %d\n",*unack);
        }
    }
    
    return 1;
}


/*
 * Checks the Ack validity and returns the seqnumb written in it.
 * Returns -1 if the packet is not valid.
 * Modifies 'seq', 'bufferFill' and 'bufferPos' only if a new window size is
 * received.
 *
 */
int processAck(int *seq, int *bufferFill,int *bufferPos) {
    
    // Checks the packet validity and convert packet.length with 'ntohs'
    if(!packet_valid(&ackBuffer)) {
        return -1;
    }
    int newWinSize = ackBuffer.window;
    if(newWinSize==0){ newWinSize = 1; }
    return ackBuffer.seqnum;
}


/*
 * Verifies the whole file has been transmitted
 */
int isTransmitted(ssize_t size, int bufferFill, int bufferPos) {
    if(!lastseqsent){
    	return 0;
    }
    int lastseqinbuffer = (bufferPos-1+BUFFSIZE)%BUFFSIZE;
    
    return window[lastseqinbuffer].received;
}



/*
* Returns the first sequence number for which the time is over or -1 if no timer is over
*/
int timeisover(int bufferFill, int bufferPos) {
	clock_t now = clock();
	int i;
	for(i=0;i<bufferFill;i++){
		int index = (bufferPos-bufferFill+i+MAXBUFFSIZE)%MAXBUFFSIZE;
		if(window[index].received==false){
			int diff = (now - window[index].timesent)*1000/CLOCKS_PER_SEC;//msecs
			if(diff>TIMEOUT){
				return window[index].seqnum;
			}
		}
	}
	return -1;
}


/* 
 * Manages all the sending procedure : reading the file, placing the packets in
 * the sending buffer, sending packets and processing acknowledgments.
 */
int selectiveRepeat() {
    
    int seq = 0;        // Seq number of the last packet to send
    int unack = 0;      // Numéro de séquence du dernier acquis reçu
    int bufferFill = 0; // Number of packets in the buffer
    int bufferPos = 0;  // Index where to insert next packet in sending buffer
    
    // Initialize ack buffer
    memset(&ackBuffer, 0, sizeof(packetstruct));
    
    // Initialize window to avoid problems when resizing
    int i;
    for(i = 0; i < MAXBUFFSIZE; i++) {
        window[i].seqnum=-1;
        window[i].received=true;
    }
    
    ssize_t size = PAYLOADSIZE;     // PAYLOAD size of last packet inserted
    ssize_t received;               // Number of bytes received on the socket
    int ackedframe, whichisover;    // last seq numb aknowledged and
                                    // idx of the timer that's over
    
    
    //Loops while there is something left to transmit
    while(!isTransmitted(size,bufferFill,bufferPos)) {
        
        // Inserts in the buffer and sends new frames
        while (bufferFill < BUFFSIZE && size == PAYLOADSIZE) {
            size = insert_in_buffer(&seq, &bufferPos, &bufferFill);
            supersend(bufferPos, bufferFill, seq, seq-1);
            
        }
        if(size<PAYLOADSIZE){

            	lastseqsent = true;
        }
        // Acknowledgements reception
        fcntl(sock_id, F_SETFL, O_NONBLOCK);
        received =
        recvfrom(sock_id,(void*)(&ackBuffer),sizeof(ackBuffer),0,NULL,NULL);
        
        if(received != sizeof(packetstruct) && errno != EAGAIN) {
            Die("Reception error");
        }
        
        /* updates 'seq', 'bufferFill' and 'bufferPos'. Returns the seq_num
         contained in the Ack frame */
        if(received==sizeof(packetstruct)) {
            ackedframe = processAck(&seq, &bufferFill, &bufferPos);
            if(ackedframe != -1) {
                
                // Remove acked packets from the buffer
                remv_from_buffer(bufferPos, &bufferFill, seq, &unack, ackedframe);
                if(opts.verbose){
            		printf("Ack received : sequence number %d\n",ackedframe);
            	}
            }
        }
        
        // Timers management
        if((whichisover = timeisover(bufferFill,bufferPos)) != -1) {
            if(opts.verbose){
            	printf("Packet resent : Timer expired for packet with seq %d\n", whichisover);
            }
            supersend(bufferPos,bufferFill, seq, whichisover);
        }
        
    }
    printf("File successfully transmitted !\n");
    return EXIT_SUCCESS;
}



/* Prints the program usage */
void usage() {
    printf("usage : ./sender [--file filename] [--sber x] [--splr x]\n");
    printf("\t\t\t\t\t[--delay d] hostname port\n");
}


/*
 * Checks the arguments passed to the program and updates the adapted variables
 * if needed, or set them to their default values if not specified.
 * 'argv' is unchanged, and 'opt_count' is set to the number of options passed
 * to the program + 1.
 */
void map_options(int argc, char **argv, int *opt_count) {

    // default parameters values
    opts.filename = NULL;
    opts.sber = 0;
    opts.splr = 0;
    opts.delay = 0;
    opts.verbose = 0;

    /* options descriptor */
    static struct option longopts[] = {
        {"file",   required_argument,    NULL,     'f' },
        {"sber",   required_argument,    NULL,     's'},
        {"splr",   required_argument,    NULL,     'S'},
        {"delay",  required_argument,    NULL,     'd'},
        {"verbose", no_argument, NULL, 'v'},
        { NULL,    0,                    NULL,     0 },
    };

    int ch;
    while ((ch = getopt_long(argc, argv, "vf:", longopts, NULL)) != -1) {
        switch (ch) {
        case 'f':
            opts.filename = optarg;
            break;

        case 's':
            opts.sber = atoi(optarg);
            break;

        case 'S':
            opts.splr = atoi(optarg);
            break;

        case 'd':
            opts.delay = atoi(optarg);
            break;
		case 'v':
			opts.verbose = true;
			break;
        default:
            usage();
        }
    }
    *opt_count = optind;
}


// Main function, doing all the job
int main(int argc, char* argv[]) {

    // Check the program arguments
    int opt_count;
    map_options(argc, argv, &opt_count);

    argc -= opt_count;
    argv += opt_count;
    if(argc != 2) {
        usage();
        exit(EXIT_FAILURE);
    }
    
    char *addr_str = argv[0];
    char *port_str = argv[1];
    

    // Open the source file
    if(opts.filename == NULL) {
        fileDescriptor = STDIN_FILENO;
    }
    else {
	    fileDescriptor = open(opts.filename, O_RDONLY);
    }
	if (fileDescriptor < 0) {
		perror("Error opening file");
		exit(EXIT_FAILURE);
	}

	if(opts.verbose){
    	printf("Sending on address %s - port %s\n", addr_str, port_str);
    	printf("Parameters values :\n");
    	printf("\tfd : %d\n", fileDescriptor);
    	printf("\tsber (percent): %d\n", opts.sber);
    	printf("\tsplr (percent): %d\n", opts.splr);
    	printf("\tdelay (ms) : %d\n", opts.delay);
	}

    // Resolve the address passed to the program
	int result;
	if((result = getaddrinfo(addr_str, port_str, &hints, &address)) < 0) {
		printf("Error resolving address %s - code %i\n", addr_str, result);
		freeaddrinfo(address);
		exit(EXIT_FAILURE);
	}

    // initialize the socket used by the sender
	sock_id = init_host(address, sender);
	if(sock_id == -1) {
		perror("Error creating socket");
		freeaddrinfo(address);
		exit(EXIT_FAILURE);
	}

    // call selective repeat to send the file
	selectiveRepeat();
    
    
    // Program closing
	if (close(fileDescriptor)) {
        Die("Error closing source file");
	}
	freeaddrinfo(address);
	close(sock_id);
	return EXIT_SUCCESS;
}

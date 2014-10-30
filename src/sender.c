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
#define TIMEOUT 500                 // Timer de chaque paquet en msec
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

// TODO: c'est quoi ?
struct options {
    char* filename;
    int sber;
    int splr;
    int delay;
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
 * Insère les données avec le header et le CRC dans le sending_buffer à la
 * bonne position et retourne la taille des données lues dans le fichier.
 * Modifie 'seq' : incremente de 1
 * Modifie 'bufferPos' : incremente de 1 % BUFFSIZE
 * Modifie 'bufferFIll' : incremente de 1
 * La Window est aussi mise a jour (seqnum=seq et received=false)
 *
 * TODO: checker le buffer
 */
int insert_in_buffer(int *seq, int *bufferPos, int *bufferFill) {
    
    if (*bufferFill >= BUFFSIZE) {
        Die("Da buffer is-a full");
    }
    //TODO: c'est quoi ce cas ?? Risque de bug pour moi !
    if (window[*bufferPos].seqnum == *seq) {
        return send_buffer[*bufferPos].length;
    }
    
    void *payloadaddress = &(send_buffer[*bufferPos].payload);
    
    ssize_t size = read(fileDescriptor, payloadaddress, PAYLOADSIZE);
    
    // Remplissage du buffer
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
    
    // Mise à jour des infos de la window
    window[*bufferPos].seqnum = *seq;
    window[*bufferPos].received = false;
    
    // Mise à jour des variables de description de la fenêtre et de la séquence
    *bufferPos = (*bufferPos+1)%MAXBUFFSIZE;
    *bufferFill = *bufferFill+1;
    *seq = (*seq+1)%SEQSPAN;
    return size;
}


/*
 * Fonction qui envoie le packet avec 'paquetseq' comme numero de sequence.
 * Ce packet DOIT deja etre dans le buffer !!
 * FIXME: virer -- used : supersend(bufferPos, bufferFill, seq, seq-1);
 */
int supersend(int bufferPos, int bufferFill, int seq, int paquetseq) {
    
    /* TODO: le modulo est vmt necessaire ? Puisque de tte facon, si
     on a paquetseq > seq, ca va dier apres ! */
	int diff = (seq-paquetseq+SEQSPAN)%SEQSPAN;
	if(diff > bufferFill) {
		Die("Yo, you buffer'ss too small fo da shit man");
	}
	int packetbufferindex = (bufferPos-diff+MAXBUFFSIZE)%MAXBUFFSIZE;
	void *bufaddress = &send_buffer[packetbufferindex];

    // Send the word to the server
    // TODO: mettre au propre
	ssize_t lensent = send(sock_id, bufaddress, sizeof(packetstruct), 0);
	window[packetbufferindex].timesent=clock();

    // FIXME Verbose print
	//printf("Packet sent with sequence number %d, %d bytes \n",window[packetbufferindex].seqnum,(int)lensent);
	if(lensent != sizeof(packetstruct)) {
		Die("Mismatch in number of sent bytes");
	}

	// TODO : Wait if receiver unavailable?
	return lensent;
}


/*
 * Function that removes all the packets before ackedframe from the buffer
 * FIXME: la valeur de retour sert a quoi ??
 * FIXME: le noeud du prob a l'air d'etre ici !!
 */
int remv_from_buffer(int bufferPos, int *bufferFill, int seq, int *unack, int ackedframe) {
    
    int diff = (seq-ackedframe+1+SEQSPAN)%SEQSPAN;
    
    if(diff > *bufferFill) {
        printf("Ack is out of window : (diff=%d), (bufFill=%d), (acked=%d), (seq=%d)\n", diff, *bufferFill, ackedframe, seq);
        return 1;
    }
    int i = 0;
    while(i <= *bufferFill-diff) {
        //enlever les paquets acquis du buffer
        window[(bufferPos-*bufferFill+i+MAXBUFFSIZE)%MAXBUFFSIZE].received = true;
        *unack=(*unack+1)%SEQSPAN;
        *bufferFill=*bufferFill-1;
        printf("Unack = %d\n",*unack);
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
    
    // Check the packet validity and convert packet.length with 'ntohs'
    if(!packet_valid(&ackBuffer)) {
        printf("Received an unvalid packet !\n");
        return -1;
    }

    int newWinSize = ackBuffer.window;
    
    if(newWinSize==0){ newWinSize = 1; }
    
    //TODO: je trouve ca plus beau comme ca...
//    if(newWinSize < BUFFSIZE) {
//        int diff = BUFFSIZE-newWinSize;
//        *seq = (*seq-diff+SEQSPAN)%SEQSPAN;
//        *bufferFill = *bufferFill-diff;
//        *bufferPos = (*bufferPos-diff+MAXBUFFSIZE)%MAXBUFFSIZE;
//    }
//    int ackseq = (&ackBuffer)->seqnum;
    
    //TODO: ca reste tres nebuleux tout ca...
    if(newWinSize > BUFFSIZE) {
        return -1;
    }
    int diff = BUFFSIZE-newWinSize;
    *seq = (*seq-diff+SEQSPAN)%SEQSPAN;
    *bufferFill = *bufferFill-diff;     // Ah ouais ??
    *bufferPos = (*bufferPos-diff+MAXBUFFSIZE)%MAXBUFFSIZE;
    
    printf("seq Num : %d\n", ackBuffer.seqnum);
    return ackBuffer.seqnum;
}


/*
 *	Vérifie que le fichier complet a été transmis
 */
int isTransmitted(ssize_t size, int bufferFill, int bufferPos) {
    
    int i =0;
    int allackedwindow = 1;
    while(i < bufferFill && allackedwindow) {
        allackedwindow = (window[(bufferPos-bufferFill+i+MAXBUFFSIZE)%MAXBUFFSIZE].received);
        i++;
    }
    int istransm = (size < PAYLOADSIZE) && allackedwindow;
    return istransm;
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
    int unack = 0;      // FIXME: Numéro de séquence du dernier acquis reçu
    int bufferFill = 0; // Number of packets in the buffer
    int bufferPos = 0;  // Index where to insert next packet in sending buffer
    
    // Initialize ack buffer
    memset(&ackBuffer, 0, sizeof(packetstruct));
    
    // Initialize window to avoid problems when resizing
    int i;
    for(i = 0; i < MAXBUFFSIZE; i++) {
        window[i].seqnum=-1;
    }
    
    ssize_t size = PAYLOADSIZE;     // PAYLOAD size of last packet inserted
    ssize_t received;               // Number of bytes received on the socket
    int ackedframe, whichisover;    // last seq numb aknowledged and
                                    // idx of the timer that's over
    
    
    //La boucle tourne tant qu'il reste du fichier a transmettre
    while(!isTransmitted(size,bufferFill,bufferPos)) {
        
        // Insertion et envoi de nouveaux frames dans le buffer
        while (bufferFill < BUFFSIZE && size == PAYLOADSIZE) {
            
            // FIXME: debug print
            //printf("BufferPos : %d\n BufferFill : %d\n BufferSize : %d\n",bufferPos,bufferFill,BUFFSIZE);
            printf("BufferPos : %d\n BufferFill : %d\n",bufferPos,bufferFill);
            size = insert_in_buffer(&seq, &bufferPos, &bufferFill);
            supersend(bufferPos, bufferFill, seq, seq-1);
        }
        
        // Acknowledgements reception
        fcntl(sock_id, F_SETFL, O_NONBLOCK);    // make receive non-blocking
        received =
            recvfrom(sock_id,(void*)(&ackBuffer),sizeof(ackBuffer),0,NULL,NULL);
        
        if(received != sizeof(packetstruct) && errno != EAGAIN) {
            // The error isn't that nothing was received
            Die("No packet received");
        }
        
        /* updates 'seq', 'bufferFill' and 'bufferPos'. Returns the seq_num
         contained in the Ack frame */
	if(received==sizeof(packetstruct)){
	        ackedframe = processAck(&seq, &bufferFill, &bufferPos);
	        if(ackedframe != -1) {
            
            // Remove acked packets from the buffer
            remv_from_buffer(bufferPos, &bufferFill, seq, &unack, ackedframe);
            
            // Timers management
            while((whichisover = timeisover(bufferFill,bufferPos)) != -1) {
                //printf("Time is over! : %d\n",whichisover);
                supersend(bufferPos,bufferFill, seq, whichisover);
            }
        }
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

    /* options descriptor */
    static struct option longopts[] = {
        {"file",   required_argument,    NULL,     'f' },
        {"sber",   required_argument,    NULL,     's'},
        {"splr",   required_argument,    NULL,     'S'},
        {"delay",  required_argument,    NULL,     'd'},
        { NULL,    0,                    NULL,     0 }
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

    //FIXME: debug print !
    printf("Sending on address %s - port %s\n", addr_str, port_str);
    printf("Parameters values :\n");
    printf("\tfd : %d\n", fileDescriptor);
    printf("\tsber (percent): %d\n", opts.sber);
    printf("\tsplr (percent): %d\n", opts.splr);
    printf("\tdelay (ms) : %d\n", opts.delay);


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

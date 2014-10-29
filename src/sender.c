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
#define DEFAULTBUFFSIZE 31                 // size of the buffer
#define MAXBUFFSIZE 31

#define IP_PROT PF_INET6            // defines the ip protocol used (IPv6)
struct sockaddr_storage src_host;   // source host emitting the packets
socklen_t src_len;      // size of the source address


struct addrinfo *address = NULL;    // address & port we're listening to
struct addrinfo hints = {
    .ai_family = IP_PROT,
    .ai_socktype = SOCK_DGRAM,
    .ai_protocol = IPPROTO_UDP };

//FIXME: definition de window commune a send et receive non ?
typedef struct slot {
	uint8_t seqnum;
	bool received;
	clock_t timesent;
} window_slot;

int BUFFSIZE = DEFAULTBUFFSIZE;
window_slot window[MAXBUFFSIZE];
packetstruct send_buffer[MAXBUFFSIZE];
packetstruct ackBuffer;
int sock_id;
int fileDescriptor;
char *filename;

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
 * Insère les données avec le header et le CRC dans le buffer à la
 * bonne position et retourne la taille des données
 * insérées(sans header et CRC)
 */
int insert_in_buffer(int *seq,int *bufferPos,int *bufferFill) {
	if (*bufferFill>=BUFFSIZE){
		Die("Da buffer is-a full");
	}
	
	// Remplissage du buffer
	void *payloadaddress = &(send_buffer[*bufferPos].payload);
	ssize_t size = read(fileDescriptor, payloadaddress, PAYLOADSIZE);
	send_buffer[*bufferPos].type = PTYPE_ACK;
	send_buffer[*bufferPos].window = 0;
	send_buffer[*bufferPos].seqnum = *seq;
	send_buffer[*bufferPos].length = size;
	send_buffer[*bufferPos].length = htons(send_buffer[*bufferPos].length);

    uint32_t crc;
	if(compute_crc(&send_buffer[*bufferPos], &crc)) {
        Die("Error computing crc");
    }
	send_buffer[*bufferPos].crc = htons(crc);

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
 * Fonction qui envoie un paquet avec l'index paquetseq
*/
int supersend(int bufferPos, int bufferFill, int seq, int paquetseq, int sock_id) {
	int diff = (seq-paquetseq+SEQSPAN)%SEQSPAN;
	if(diff>bufferFill){
		Die("Yo, you buffers too small fo da shit man");
	}
	int packetbufferindex = (bufferPos-diff+MAXBUFFSIZE)%MAXBUFFSIZE;
	void *bufaddress = &send_buffer[packetbufferindex];

    // Send the word to the server
    // TODO: mettre au propre
	ssize_t lensent = send(sock_id, bufaddress, sizeof(packetstruct), 0); // Taille donnée par length +8
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
*/
int remv_from_buffer(int bufferPos, int *bufferFill, int seq, int *unack, int ackedframe){
	int diff = (seq-ackedframe+1+SEQSPAN)%SEQSPAN;
	if(diff>*bufferFill){
		printf("Ack is out of window");
		return 1;
	}
	int i = 0;
	while(i<=*bufferFill-diff){
			//enlever les paquets acquis du buffer
			window[(bufferPos-*bufferFill+i+MAXBUFFSIZE)%MAXBUFFSIZE].received = true;
			*unack=(*unack+1)%SEQSPAN;
			*bufferFill=*bufferFill-1;
			printf("Unack = %d\n",*unack);
	}
	return 1;
}
/*
* Vérifie la validité de l'acquis et retourne le numéro de séquence
*/
int processAck(int *seq, int *bufferFill,int *bufferPos){
	// TODO : Verifier le CRC
	int  newWinSize = ntohs((&ackBuffer)->length);
	if(newWinSize==0){newWinSize = 1;}
	if(newWinSize<BUFFSIZE){
		int diff = BUFFSIZE-newWinSize;
		*seq = (*seq-diff+SEQSPAN)%SEQSPAN;
		*bufferFill = *bufferFill-diff;
		*bufferPos = (*bufferPos-diff+MAXBUFFSIZE)%MAXBUFFSIZE;
	}
	int ackseq = (&ackBuffer)->seqnum;
	return ackseq;

}
/*
*	Vérifie que le fichier complet a été transmis
*/
int isTransmitted(ssize_t size,int bufferFill, int bufferPos)
{
	int i =0;
	int allackedwindow = 1;
	while(i<bufferFill && allackedwindow){
		allackedwindow = (window[(bufferPos-bufferFill+i+MAXBUFFSIZE)%MAXBUFFSIZE].received);
		i++;
	}
	int istransm = (size!=PAYLOADSIZE) && allackedwindow;
return istransm;
}
/*
* Returns the first sequence number for which the time is over or -1 if no timer is over
*/
int timeisover(int bufferFill, int bufferPos){
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
//Fonction qui gère l'envoi des paquets
int selectiveRepeat() {

	int seq = 0;//Numéro de séquence du prochain frame à envoyer
	int unack = 0;//Numéro de séquence du dernier acquis reçu
	int bufferFill = 0;//Nombres de packet dans le buffer
	int bufferPos = 0; // La position dans le buffer du prochain paquet à remplacer

	// TODO : mettre des bonnes valeurs


	//fd_set readfs; //Set de filedescriptors utilisé par select

	ssize_t size = PAYLOADSIZE;
	//La boucle tourne tant qu'il reste du fichier a transmettre
	while(!isTransmitted(size,bufferFill,bufferPos)) {
        // Insertion et envoie de nouveaux frames dans le buffer
		while (bufferFill<BUFFSIZE && size==PAYLOADSIZE) {
			printf("BufferPos : %d\n BufferFill : %d\n BufferSize : %d\n",bufferPos,bufferFill,BUFFSIZE);
			size = insert_in_buffer(&seq, &bufferPos, &bufferFill);
			supersend(bufferPos, bufferFill, seq, seq-1, sock_id);
		}

	    //Réception des acquis
		fcntl(sock_id,F_SETFL,O_NONBLOCK);
        // TODO : Essai d'addresse non définie(ne marche pas avec ai_addr)
		int received =
            recvfrom(sock_id,(void*)(&ackBuffer),sizeof(ackBuffer),0,NULL,NULL);

        if((received) < 0) {
			if (errno !=EAGAIN) {
                //Packets received
				Die("No packet received");
			}
		}
		else {
			int ackedframe = processAck(&seq,&bufferFill,&bufferPos);
			remv_from_buffer(bufferPos, &bufferFill, seq, &unack, ackedframe);
		}

        // Gestion des timers
		int whichisover;
		while((whichisover=timeisover(bufferFill,bufferPos))!=-1) {
			//printf("Time is over! : %d\n",whichisover);
			supersend(bufferPos,bufferFill,seq,whichisover,sock_id);
		}

	}
	printf("\nFile successfully transmitted\n");
	return EXIT_SUCCESS;
}



/* Print l'usage de la fonction send */
void usage() {
    printf("usage : ./sender [--file filename] [--sber x] [--splr x]\n");
    printf("\t\t\t\t\t[--delay d] hostname port\n");
}


/*
 * Formate les options passees au programme
 */
void map_options(int argc, char **argv, int *opt_count) {

    // default values
    //TODO: replace "loremipsum by NULL !"
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


int main(int argc, char* argv[]) {

    int opt_count;
    map_options(argc, argv, &opt_count);

    argc -= opt_count;
    argv += opt_count;

    if(argc != 2) {
        usage();
        exit(EXIT_FAILURE);
    }

    //On ouvre le fichier
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

	char *addr_str = argv[0];
	char *port_str = argv[1];

    //FIXME: debug print !
    printf("Sending on address %s - port %s\n", addr_str, port_str);
    printf("Parameters values :\n");
    printf("\tfd : %d\n", fileDescriptor);
    printf("\tsber (percent): %d\n", opts.sber);
    printf("\tsplr (percent): %d\n", opts.splr);
    printf("\tdelay (ms) : %d\n", opts.delay);


	/* Resolve the address passed to the program */
	//FIXME: faire dependre de argv !
	int result;
	if((result = getaddrinfo(addr_str, port_str, &hints, &address)) < 0) {
		printf("Error resolving address %s - code %i\n", addr_str, result);
		freeaddrinfo(address);
		exit(EXIT_FAILURE);
	}


	/* initialize the socket used by the sender */
	sock_id = init_host(address, sender);
	if(sock_id == -1) {
		perror("Error creating socket");
		freeaddrinfo(address);
		exit(EXIT_FAILURE);
	}

	// FIXME: verbose print
    printf("Socket initialization passed !\n");

	selectiveRepeat();
	int err = close(fileDescriptor);
	if (err <0){
		perror("Error closing file");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(address);
	close(sock_id);
	return EXIT_SUCCESS;
}

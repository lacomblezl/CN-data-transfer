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
#define PAYLOADSIZE 512
#define HEADERSIZE 4
#define CRCSIZE 4
#define MAXSEQ 20
#define TIMEOUT 5
#define BUFFSIZE    10              // size of the buffer

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
} window_slot;

window_slot window[BUFFSIZE];                        
packetstruct send_buffer[BUFFSIZE];
packetstruct ackBuffer;
int sock_id;
int fileDescriptor;
char *filename;

/*
* Cleanly exits the application in case of error
*/
void Die(char* error_msg) {
    perror(error_msg);
    freeaddrinfo(address);
    close(sock_id);
    //TODO: close(fd);
    exit(EXIT_FAILURE);
}
/*
* Insère les données avec le header et le CRC dans le buffer à la   bonne position
* et retourne la taille des données insérées(sans header et CRC)
*/
int insert_in_buffer(int *seq,int *bufferPos,int *bufferFill){
	if (*bufferFill>=BUFFSIZE){
		Die("Da buffer is-a full");
	}
	// Remplissage du buffer
	void *payloadaddress = &(send_buffer[*bufferPos].payload);
	ssize_t size = read(fileDescriptor, payloadaddress, PAYLOADSIZE);
	send_buffer[*bufferPos].type = PTYPE_ACK;
	send_buffer[*bufferPos].window = BUFFSIZE;
	send_buffer[*bufferPos].seqnum = *seq;
	send_buffer[*bufferPos].length = size;
	send_buffer[*bufferPos].length = htons(send_buffer[*bufferPos].length);
    	send_buffer[*bufferPos].crc = htonl(0);
	//FIXME changer le CRC
	// Mise à jour des infos de la window
	window[*bufferPos].seqnum = *seq;
	window[*bufferPos].received = false;
	// Mise à jour des variables de description de la fenêtre et de la séquence
	*bufferPos = (*bufferPos+1)%BUFFSIZE;
	*bufferFill = *bufferFill+1;
	*seq = (*seq+1)%MAXSEQ;
	// TODO changer le contenu de window (Nécessaire? On utilise pas les numéros de séquence)
	return size;
}
/*
 * Fonction qui envoie un paquet avec l'index paquetseq
*/
int supersend(int bufferPos, int bufferFill, int seq, int paquetseq, int sock_id){
	int diff = (seq-paquetseq+MAXSEQ)%MAXSEQ;
	if(diff>bufferFill){
		Die("Yo, you buffers too small fo da shit man");
	}
	int packetbufferindex = (bufferPos-diff+BUFFSIZE)%BUFFSIZE;
	void *bufaddress = &send_buffer[packetbufferindex];
	// Send the word to the server
	int lentosend = ntohs(((packetstruct *)bufaddress)->length)+8;
	ssize_t lensent = send(sock_id, bufaddress, lentosend, 0); // Taille donnée par length +8
	// FIXME Verbose print
	printf("Paquet sent with sequence number %d, %d bytes \n",window[packetbufferindex].seqnum,(int)lensent);
	if(lensent != (lentosend)) {
		Die("Mismatch in number of sent bytes");
	}
	// TODO : Wait if receiver unavailable?
	return lensent;
}
/*
* Function that removes a packet from the buffer
*/
int remv_from_buffer(int bufferPos, int *bufferFill, int seq, int *unack, int ackedframe){
	int diff = (seq-ackedframe+MAXSEQ)%MAXSEQ;
	if(diff>*bufferFill){
		printf("Ack is out of window");
		return 1;
	}
	int packetbufferindex = (bufferPos-diff+BUFFSIZE)%BUFFSIZE;
	window[packetbufferindex].received = true;
	int i = 0;
	while((window[(bufferPos-*bufferFill+i+BUFFSIZE)%BUFFSIZE].received) && i<*bufferFill){
			//enlever les paquets acquis du buffer
			*unack=(*unack+1)%MAXSEQ;
			*bufferFill=*bufferFill-1;
	}
			/*if (unack==seq){
				cancel_timer();
			}
			else{restart_timer();}
			}*/
	// TODO : Bloody timers
	return 1;
}
/*
* Vérifie la validité de l'acquis et retourne le numéro de séquence
*/
int processAck(){
	// TODO : Verifier le CRC
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
		allackedwindow = (window[(bufferPos-bufferFill+i+BUFFSIZE)%BUFFSIZE].received);
		i++;
	}
	int istransm = (size!=PAYLOADSIZE) && allackedwindow;
return istransm;
}
//Fonction qui gère l'envoi des paquets
int selectiveRepeat(){
	int seq = 0;//Numéro de séquence du prochain frame à envoyer
	int unack = 0;//Numéro de séquence du dernier acquis reçu
	int bufferFill = 0;//Nombres de frames dans le buffer
	int bufferPos = 0; // La position dans le buffer du prochain paquet à remplacer

	// TODO : mettre des bonnes valeurs

	fd_set readfs; //Set de filedescriptors utilisé par select
	
	ssize_t size = PAYLOADSIZE;
	//La boucle tourne tant qu'il reste du fichier a transmettre
	while(!isTransmitted(size,bufferFill,bufferPos)){
		struct timeval timeOut;
		timeOut.tv_sec = TIMEOUT;
		timeOut.tv_usec = 0.0;// TODO Valeurs du timeout
		while (bufferFill<BUFFSIZE && size==PAYLOADSIZE){// FIXME : timer pour chaque paquet
			if (seq==unack){//Start timer
			}
			size = insert_in_buffer(&seq,&bufferPos,&bufferFill);
			supersend(bufferPos,bufferFill,seq,seq-1,sock_id);
			//FIXME : verbose print
			// TODO : Vérifier que le timeout n'a pas été dépassé
		}
		int nfd = -1; // number of file descriptor that are set in readfs
		//Vider readfs et mettre sock dedans
		while (nfd==-1){
			FD_ZERO(&readfs);
			FD_SET(sock_id, &readfs);
			nfd = select(sock_id+1,&readfs, NULL,NULL,&timeOut);
		}// FIXME : Avec un if?
		//printf("Value of nfd : %d\n",nfd);
		/*while (nfd==-1){
			perror("select");
			exit(EXIT_FAILURE);
		}*/
		//Acquis reçu
		if (FD_ISSET(sock_id,&readfs)){
			//printf("Sockaddress : %s\n",(address->ai_addr)->sa_data);
			// TODO : Essai d'addresse non définie(ne marche pas avec ai_addr)
			
			//int received = recvfrom(sock_id,(void *)(ackBuffer),sizeof(packetstruct),0,NULL,0); Je ne comprends pas pq cette ligne ne marche pas
			int received = recvfrom(sock_id,(void *)(&ackBuffer),sizeof(ackBuffer),0,NULL,NULL);
			//printf("Value of received : %d\n",received);
			if((received)<0){
				Die("No packet received");
			}
			int ackedframe = processAck();
			remv_from_buffer(bufferPos, &bufferFill, seq, &unack, ackedframe);
		}
		else{
		//Timeout
		//if (nfd == 0){
			printf("Timeout reached\n");
			int i;
			for (i=0;i<bufferFill;i++){
				supersend(bufferPos,bufferFill,seq,seq-bufferFill+i,sock_id);
				// Restart timer ???
			}
		}

		
	}
			printf("File successfully transmitted\n");
	return EXIT_SUCCESS;
}





int main(int argc, const char** argv) {
	//On ouvre le fichier
	filename = "loremipsum.txt";
	fileDescriptor = open(filename, O_RDONLY);
	if (fileDescriptor<0){
		perror("Error opening file");
		exit(EXIT_FAILURE);
	}
	//TODO: getOpt !
	    /* Provisoire... */
	//TODO : si l'addresse n'est pas valide
	const char *addr_str = argv[1];
	const char *port = argv[2];
	
	
	/* Resolve the address passed to the program */
	//FIXME: faire dependre de argv !
	int result;
	if((result = getaddrinfo(addr_str, port, &hints, &address)) < 0) {
		printf("Error resolving address %s - code %i\n", argv[0], result);
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



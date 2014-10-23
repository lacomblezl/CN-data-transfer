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
#define PAYLOADSIZE 512
#define HEADERSIZE 4
#define CRCSIZE 4
#define MAXSEQ 7
#define TIMEOUT 5

#define IP_PROT PF_INET6            // defines the ip protocol used (IPv6)
#define BUFFSIZE 10                 // size of the receiving buffer


struct addrinfo *address = NULL;    // address & port we're listening to
struct addrinfo hints = {
    .ai_family = IP_PROT,
    .ai_socktype = SOCK_DGRAM,
    .ai_protocol = IPPROTO_UDP };

int sock_id;                        // The socket used by the program


// Cleanly exits the application in case of error
void Die(char* error_msg) {
    perror(error_msg);
    freeaddrinfo(address);
    close(sock_id);
    //TODO: close(fd);
    exit(EXIT_FAILURE);
}
/* Insère les données avec le header et le CRC dans le buffer à la bonne position
 et retourne la taille des données insérées(sans header et CRC
*/
int insert_in_buffer(int fileDescriptor,void *bufPointer,int seq,int *bufferPos,int *bufferFill){
	void *address = bufPointer + (*bufferPos)*(HEADERSIZE	+PAYLOADSIZE+CRCSIZE);
	ssize_t size = read(fileDescriptor, address, PAYLOADSIZE);
	if(size<PAYLOADSIZE){
		int i;
		for(i = size;i<PAYLOADSIZE;i++){
			*((char*)(address+i)) = 0;
		}
	}
	strcpy(bufPointer,"aaaa");
	strcpy(address+PAYLOADSIZE,"bbb");
	return size;
}
/*int connect(){
printf("You are connected");
}*/

//Fonction qui envoie un paquet
int supersend(void *bufPointer,int bufferPos, int sock_id,struct addrinfo *destaddr){
	void *bufaddress = bufPointer + (bufferPos)*(HEADERSIZE+PAYLOADSIZE+CRCSIZE);
	/* Send the word to the server */
	int echolen = HEADERSIZE+PAYLOADSIZE+CRCSIZE;
	ssize_t lensent = send(sock_id, bufaddress, echolen-1, 0);
	printf("%d\n",(int)lensent);
	if(lensent != echolen) {
		Die("Mismatch in number of sent bytes");
	}
	return lensent;
}
/*
int* crc(){

	}
int disconnect(){
	printf("You are disconnected");
}*/

//Fonction qui gère l'envoi des paquets
int selectiveRepeat(int fileDescriptor){
	// Paramètres
	int windowSize = 3; //Taille de la fenêtre
	//int maxseq = 7;// Taille de la sequence de nombre à suivre
	
	int seq = 0;//Numéro de séquence du prochain frame à envoyer
	int unack = 0;//Numéro de séquence du dernier acquis reçu
	int bufferFill = 0;//Nombres de frames dans le buffer
	int bufferPos = 0; // La position dans le buffer du prochain paquet à remplacer

	//Allocation de mémoire pour le buffer
	void *bufPointer = malloc(sizeof(char)*(HEADERSIZE+PAYLOADSIZE+CRCSIZE)*windowSize);
	if (bufPointer == NULL){
		perror("Error allocating memory");
		exit(EXIT_FAILURE);
	}
	// déclaration de l'ensemble de lecture et du socket
	// TODO : mettre des bonnes valeurs
	fd_set readfs;
	
	ssize_t size = PAYLOADSIZE;
	//La boucle tourne tant qu'il reste du fichier a transmettre
	while(size==PAYLOADSIZE){
		//struct timerStart, timeAfter;
		//gettimeofday(&timerStart,NULL);
		struct timeval timeOut;
		timeOut.tv_sec = TIMEOUT;
		timeOut.tv_usec = 0.0;// TODO Valeurs du timeout
		while (bufferFill<windowSize && size==(PAYLOADSIZE+HEADERSIZE+CRCSIZE)){// FIXME : timer pour chaque paquet
			if (seq==unack){//Start timer
}
			size = insert_in_buffer(fileDescriptor,bufPointer,seq,&bufferPos,&bufferFill);
			supersend(bufPointer,bufferPos,sock_id,address);
			seq = (seq+1)%MAXSEQ;
			bufferFill++;
			bufferPos = (bufferPos+1)%windowSize; // Vérifier que le timeout n'a pas été dépassé
		}
		int nfd = 0; // number of file descriptor that are set in readfs
		//Vider readfs et mettre sock dedans
		FD_ZERO(&readfs);
		FD_SET(sock_id, &readfs);
		//Erreur de select
		if ((nfd == select(sock_id+1,&readfs, NULL,NULL,&timeOut))<0){
			perror("select");
			exit(EXIT_FAILURE);
		}
		//Timeout
		if (nfd == 0){
			int i;
			for (i=0;i<bufferFill;i++){
				supersend(bufPointer,(bufferPos+i)%MAXSEQ,sock_id,address);
				// Restart timer ???
			}
		}
		//Acquis reçu
		if (FD_ISSET(sock_id,&readfs)){
			char ackBuffer[HEADERSIZE+PAYLOADSIZE+CRCSIZE];
			int received;
			if((received = recv(sock_id,ackBuffer,HEADERSIZE+PAYLOADSIZE+CRCSIZE,0))<0){
				Die("No packet received");
			}
			
			/*int ackedframe;
			if(ackedframe==unack){
			//enlever les paquets acquis du buffer
			unack=(ackedframe+1)%MAXSEQ;
			bufferPos = (bufferPos+1)%windowSize;
			bufferFill--;
			if (unack==seq){
				cancel_timer();
			}
			else{restart_timer();}
			}
			else if(notinsequence){
				bufTable[i]=1;
			}*/
		}
	}
	free(bufPointer);
	return EXIT_SUCCESS;
}





int main(int argc, const char** argv) {
	//On ouvre le fichier
	char* filename = "loremipsum.txt";
	int fileDescriptor = open(filename, O_RDONLY);
	if (fileDescriptor<0){
		perror("Error opening file");
		exit(EXIT_FAILURE);
	}
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
	sock_id = init_host(address, sender);
	if(sock_id == -1) {
		perror("Error creating socket");
		freeaddrinfo(address);
		exit(EXIT_FAILURE);
	}

	// FIXME: verbose print
    printf("Socket initialization passed !\n");

	selectiveRepeat(fileDescriptor);
	int err = close(fileDescriptor);
	if (err <0){
		perror("Error closing file");
		exit(EXIT_FAILURE);
	}
	close(sock_id);
	return EXIT_SUCCESS;
}



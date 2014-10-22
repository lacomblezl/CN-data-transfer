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
#define PAYLOADSIZE 512
#define HEADERSIZE 4
#define CRCSIZE 4
#define MAXSEQ 7

int main(int argc, const char** argv) {
connect();
//On ouvre le fichier
char* filename = "loremipsum.txt";
int fileDescriptor = open(filename, O_RDONLY);
if (fileDescriptor<0){
	perror("Error opening file");
	exit(EXIT_FAILURE);
}
selectiveRepeat(fileDescriptor);
int err = close(fileDescriptor);
if (err <0){
	perror("Error closing file");
	exit(EXIT_FAILURE);
}
disconnect();
return EXIT_SUCCESS;
}

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
	
	ssize_t size = PAYLOADSIZE;
	//La boucle tourne tant qu'il reste du fichier a transmettre
	while(size==PAYLOADSIZE){
		if (bufferFill<windowSize){
			//if (seq==unack){start_timer;}
			size = insert_in_buffer(fileDescriptor,bufPointer,seq,&bufferPos,&bufferFill);
			send(bufPointer,bufferPos);
			seq = (seq+1)%MAXSEQ;
			bufferFill++;
			bufferPos = (bufferPos+1)%windowSize;
		}
		
	/*	if (timerexpires){
			resend(allbuffer);
			restart_timer;
		}
		*/
		//"Simulation" de la réception d'un acquis
		printf("Un acquis a-t-il été reçu?");
		int recvd = 0;
		scanf("%d",&recvd);
		//if recvd(acknowledgement){
		if(recvd==1){
			//Pour le test
			printf("Quel est le numéro de séquence de l'acquis? ");
			int ackedframe;
			scanf("%d",&ackedframe);

			if(ackedframe==unack){
			//enlever les paquets acquis du buffer
			unack=(ackedframe+1)%MAXSEQ;
			bufferPos = (bufferPos+1)%windowSize;
			bufferFill--;
			/*if (unack==seq){
				cancel_timer();
			}
			else{restart_timer();}*/
			}
			/*else if(notinsequence){
				bufTable[i]=1;
			}*/
		}
		else{sleep(1);}
	}
	free(bufPointer);
	return EXIT_SUCCESS;
}

/* Insère les données avec le header et le CRC dans le buffer à la bonne position et retourne la taille des données insérées(sans header et CRC
*/
int insert_in_buffer(int fileDescriptor,void *bufPointer,int seq,int *bufferPos,int *bufferFill){
int bufferPlace = (*bufferPos + *bufferFill)%MAXSEQ;
printf("%d",*bufferPos);
void *address = bufPointer + (*bufferPos)*(HEADERSIZE+PAYLOADSIZE+CRCSIZE);
ssize_t size = read(fileDescriptor, address, PAYLOADSIZE);
if(size<PAYLOADSIZE){
	int i;
	for(i = size;i<PAYLOADSIZE;i++){
	*((char*)(address+i)) = 0;
	}
}
*((char*)(bufPointer)) = "aaaa";
*((char*)(address+PAYLOADSIZE))= "bbb\0";
return size;
}

int connect(){
printf("You are connected");
}
//Fonction qui envoie un paquet
int send(void *bufPointer,int bufferPos){
void *address = bufPointer + (bufferPos)*(HEADERSIZE+PAYLOADSIZE+CRCSIZE);
printf("%s\n",(char*)address);
}
int* crc(){

	}
int disconnect(){
	printf("You are disconnected");
}

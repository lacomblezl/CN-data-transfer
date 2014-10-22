#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>

#define BUFFSIZE 255
#define IP_PROT PF_INET6

void Die(char *mess) { perror(mess); exit(1); }

struct addrinfo hints;
struct addrinfo *server_addr = NULL;
struct addrinfo *client_addr = NULL;

int main(int argc, char *argv[]) {

  int sock;
  // TODO: virer !
  //struct sockaddr_in echoserver;
  //struct sockaddr_in echoclient;
  char buffer[BUFFSIZE];
  //TODO:virerunsigned int clientlen;  , serverlen;
  int received = 0;

  if (argc != 2) {
    fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
    exit(1);
  }

  // Qques parametres passe a notre socket
  hints.ai_family = IP_PROT;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;

  int result;
  char listening_addr[8];
  if (IP_PROT == PF_INET6) {
      strcpy(listening_addr, "::");
  }
  else {
      strcpy(listening_addr, "0.0.0.0");
  }

  if((result = getaddrinfo(listening_addr, argv[1], &hints,
        &server_addr)) < 0) {
    printf("Error resolving address %s - code %i",
                                    argv[1], result);
    freeaddrinfo(server_addr);
    exit(EXIT_FAILURE);
  }
  //FIXME: Memory leak possible par getaddrinfo !

  //TODO: verbose print
  printf("after addrinfo\n");

  /* Create the UDP socket */
  if ((sock = socket(server_addr->ai_family, server_addr->ai_socktype,
    server_addr->ai_protocol)) < 0) {
    Die("Failed to create socket");
  }
  /* TODO:virer
  Construct the server sockaddr_in structure
  memset(&echoserver, 0, sizeof(echoserver));       // Clear struct
  echoserver.sin_family = AF_INET;                  // Internet/IP
  echoserver.sin_addr.s_addr = htonl(INADDR_ANY);   // Any IP address
  echoserver.sin_port = htons(atoi(argv[1]));       // server port
  */

  //TODO: verbose print
  printf("before bind\n");

  /* Bind the socket */
  /* TODO: virer
  serverlen = sizeof(echoserver);
  if (bind(sock, (struct sockaddr *) &echoserver, serverlen) < 0) {
    Die("Failed to bind server socket");
  }*/
  if (bind(sock, server_addr->ai_addr, server_addr->ai_addrlen) < 0) {
    Die("Failed to bind server socket");
  }

  // Initialize to structure to know who to respond to...
  client_addr = (struct addrinfo *) calloc(1, sizeof(struct addrinfo));
  if(client_addr == NULL) {
    //TODO: meilleur message que ca !
    printf("Error malloc client_addr\n");
    exit(EXIT_FAILURE);
  }

  struct sockaddr *clean_addr = (struct sockaddr*) calloc(1,
                                                sizeof(struct sockaddr));
  if(clean_addr == NULL) {
    //TODO: faire ca bien !
    printf("Error allocating memory\n");
  }

  // We specify some hints about the server addrinfo...
  client_addr->ai_family = IP_PROT;
  client_addr->ai_socktype = SOCK_DGRAM;
  client_addr->ai_protocol = IPPROTO_UDP;
  client_addr->ai_addr = clean_addr;

  //TODO: verbose print
  printf("before receive\n");


  //TODO: remplacer le addrinfo client par ca !
  struct sockaddr_storage client_addr_bis;
  socklen_t clientlen = sizeof(client_addr_bis);

  /* Run until cancelled */
  while (1) {
    /* Receive a message from the client */
    /*TODO: virer...
    clientlen = sizeof(echoclient);
    if ((received = recvfrom(sock, buffer, BUFFSIZE, 0,
      (struct sockaddr *) &echoclient,
      &clientlen)) < 0) {
        Die("Failed to receive message");
      }*/
    if ((received = recvfrom(sock, buffer, BUFFSIZE, 0,
        (struct sockaddr*) &client_addr_bis, &(clientlen))) < 0) {
          free(client_addr);
          Die("Mismatch in number of received bytes");
    }


    /* FIXME: c'est la merde ! */
    // We determine the string representation of the address resolved
    char hostname[NI_MAXHOST+1];

    if(getnameinfo((struct sockaddr*) &client_addr_bis, clientlen, hostname,
         sizeof(hostname), NULL, 0, 0)) {
           printf("Error resolving source address\n");
           exit(-1);
         }


      fprintf(stderr,
      "Client connected: %s\n", hostname);//TODO:virer... inet_ntoa(echoclient.sin_addr));
      /* Send the message back to client */
      if (sendto(sock, buffer, received, 0,
        (struct sockaddr*) &client_addr_bis,
        clientlen) != received) {
          Die("Mismatch in number of echo'd bytes");
        }
      }
    }

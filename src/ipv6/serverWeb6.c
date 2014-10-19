#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>

#define BUFFSIZE 255
void Die(char *mess) { perror(mess); exit(1); }


//FIXME: why here ?
struct addrinfo hints;
struct addrinfo *server_addr = NULL;


int main(int argc, char *argv[]) {

  int sock;
  struct sockaddr_in6 echoclient;
  char buffer[BUFFSIZE];

  // FIXME: remove that
  unsigned int clientlen; //, serverlen;
  int received = 0;

  if (argc != 2) {
    fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
    exit(1);
  }



  //FIXME: big verbose test a virer !
  //char print[INET6_ADDRSTRLEN];
  //inet_ntop(AF_INET6, &in6addr_any,print,INET6_ADDRSTRLEN);
  //printf("any:%s\n", print);



  // FIXME: modif here with echoserver...
  //struct sockaddr_in echoserver;

  // Qques parametres passe a notre socket
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;

  int result;
  if((result = getaddrinfo("::", argv[1], &hints,
        &server_addr)) < 0) {
    printf("Error resolving address %s - code %i",
                                    argv[1], result);
    freeaddrinfo(server_addr);
    exit(EXIT_FAILURE);
  }

  //FIXME: Memory leak possible par getaddrinfo !












  /* Create the UDP socket */
  //TODO: use IPv6
  if ((sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    Die("Failed to create socket");
  }


  // FIXME: remove all that shit !
  /* Construct the server sockaddr_in structure
  struct sockaddr_in echoserver;
  memset(&echoserver, 0, sizeof(echoserver));       // Clear struct
  echoserver.sin_family = AF_INET;                  // Internet/IP
  echoserver.sin_addr.s_addr = htonl(INADDR_ANY);   // Any IP address
  echoserver.sin_port = htons(atoi(argv[1]));       // server port
  */

  /* Bind the socket */
  //FIXME: remove that
  //serverlen = sizeof(echoserver);
  if (bind(sock, server_addr->ai_addr, server_addr->ai_addrlen) < 0) {
    Die("Failed to bind server socket");
  }

  //FIXME: verbose print
  printf("Socket bound... Nice !\n");



  /* Run until cancelled */
  while (1) {
    /* Receive a message from the client */
    clientlen = sizeof(echoclient);
    if ((received = recvfrom(sock, buffer, BUFFSIZE, 0,
      (struct sockaddr *) &echoclient,
      &clientlen)) < 0) {
        Die("Failed to receive message");
      }

      //FIXME: plus propre ?
      char dst[INET6_ADDRSTRLEN];
      inet_ntop(AF_INET6, &echoclient.sin6_addr,dst,INET6_ADDRSTRLEN);
      printf("Client connected: %s\n", dst);




      /* Send the message back to client */
      if (sendto(sock, buffer, received, 0,
        (struct sockaddr *) &echoclient,
        sizeof(echoclient)) != received) {
          Die("Mismatch in number of echo'd bytes");
        }
      }
    }

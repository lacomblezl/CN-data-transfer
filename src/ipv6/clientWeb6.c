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


    if (argc != 4) {
      fprintf(stderr, "USAGE: %s <server_ip> <word> <port>\n",
                                argv[0]);
      exit(1);
    }

  int sock_id;










  // FIXME: modif here with echoserver...
  //struct sockaddr_in echoserver;

  // Qques parametres passe a notre socket
  hints.ai_family = PF_INET6;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;

  int result;
  if((result = getaddrinfo(argv[1], argv[3], &hints,
        &server_addr)) < 0) {
    printf("Error resolving address %s - code %i",
                                    argv[1], result);
    freeaddrinfo(server_addr);
    exit(EXIT_FAILURE);
  }

  //FIXME: Memory leak possible par getaddrinfo !











  struct sockaddr_in6 echoclient;
  char buffer[BUFFSIZE];
  unsigned int echolen, clientlen;
  int received = 0;


  /* Create the UDP socket
   * TODO: use IPv6
   */
  if ((sock_id = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    Die("Failed to create socket");
  }


  /* TODO: virer ca !
   * Construct the server sockaddr_in structure
   */
  //echoserver.sin_family = AF_INET;                  /* Internet/IP */
  //echoserver.sin_addr.s_addr = inet_addr(argv[1]);  /* IP address */
  //echoserver.sin_port = htons(atoi(argv[3]));       /* server port */



  // FIXME: virer ca !
  // modif here (utilise getaddrinfo)
  // Send the word to the server
  /*echolen = strlen(argv[2]);
  if (sendto(sock_id, argv[2], echolen, 0,
    (struct sockaddr *) &echoserver,
    sizeof(echoserver)) != echolen) {
      Die("Mismatch in number of sent bytes");
    }*/

  echolen = strlen(argv[2]);

  if (sendto(sock_id, argv[2], echolen, 0, server_addr->ai_addr,
        server_addr->ai_addrlen) != echolen) {
      Die("Mismatch in number of sent bytes");
    }


    //FIXME: verbose print
    printf("Send succes\n");


    /* Receive the word back from the server */
    fprintf(stdout, "Received: ");
    clientlen = sizeof(echoclient);
    if ((received = recvfrom(sock_id, buffer, BUFFSIZE, 0,
      (struct sockaddr *) &echoclient,
      &clientlen)) != echolen) {
        Die("Mismatch in number of received bytes");
      }
      /* FIXME: et en IPv6 ? Ets-ce utile dans min cas ?
        Check that client and server are using same socket
      if (echoserver.sin_addr.s_addr != echoclient.sin_addr.s_addr) {
        Die("Received a packet from an unexpected server");
      }*/
      buffer[received] = '\0';        /* Assure null terminated string */
      printf("%s", buffer);
      printf("\n");
      close(sock_id);
      exit(0);
    }

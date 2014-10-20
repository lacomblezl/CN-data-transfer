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

struct addrinfo hints;
struct addrinfo *server_addr = NULL;
//TODO: struct addrinfo *client_addr = NULL;


int main(int argc, char *argv[]) {

  int sock;

  //TODO: remove that !
  struct sockaddr_in echoclient;
  char buffer[BUFFSIZE];
  unsigned int echolen, clientlen;
  int received = 0;

  if (argc != 4) {
    fprintf(stderr, "USAGE: %s <server_ip> <word> <port>\n", argv[0]);
    exit(1);
  }

  // Qques parametres passe a notre socket
  hints.ai_family = PF_INET;
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


  /* Create the UDP socket */
  if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    Die("Failed to create socket");
  }


  /* Send the word to the server */
  echolen = strlen(argv[2]);
  if (sendto(sock, argv[2], echolen, 0, server_addr->ai_addr,
    server_addr->ai_addrlen) != echolen) {
      Die("Mismatch in number of sent bytes");
  }


    /* Receive the word back from the server */
    fprintf(stdout, "Received: ");
    clientlen = sizeof(echoclient);
    if ((received = recvfrom(sock, buffer, BUFFSIZE, 0,
      (struct sockaddr *) &echoclient,
      &clientlen)) != echolen) {
        Die("Mismatch in number of received bytes");
      }
      /* Check that client and server are using same socket
      if (echoserver.sin_addr.s_addr != echoclient.sin_addr.s_addr) {
        Die("Received a packet from an unexpected server");
      }*/
      buffer[received] = '\0';        /* Assure null terminated string */
      printf("%s", buffer);
      printf("\n");
      close(sock);
      exit(0);
    }

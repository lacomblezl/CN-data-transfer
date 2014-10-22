#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>

#define BUFFSIZE 255
#define IP_PROT PF_INET6

struct addrinfo hints;
struct addrinfo *server_addr = NULL;
struct addrinfo *client_addr = NULL;

void Die(char *mess) {
    perror(mess);
    freeaddrinfo(server_addr);
    exit(1);
}

int main(int argc, char *argv[]) {

  int sock;

  char buffer[BUFFSIZE];
  unsigned int echolen;
  int received = 0;

  if (argc != 4) {
    fprintf(stderr, "USAGE: %s <server_ip> <word> <port>\n", argv[0]);
    exit(1);
  }

  // Qques parametres passe a notre socket
  hints.ai_family = IP_PROT;
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
  if ((sock = socket(server_addr->ai_family, server_addr->ai_socktype,
    server_addr->ai_protocol)) < 0) {
    Die("Failed to create socket");
  }

  /* Set the destination address used by the socket */
  if(connect(sock, server_addr->ai_addr, server_addr->ai_addrlen)) {
      Die("Failed to connect socket");
  }


  /* Send the word to the server */
  echolen = strlen(argv[2]);
  if(send(sock, argv[2], echolen, 0) != echolen) {
      Die("Mismatch in number of sent bytes");
  }

  client_addr = (struct addrinfo *) calloc(1, sizeof(struct addrinfo));
  if(client_addr == NULL) {
    //TODO: meilleur message que ca !
    printf("Error malloc client_addr\n");
    exit(EXIT_FAILURE);
  }

  // We specify some hints about the server addrinfo...
  client_addr->ai_family = IP_PROT;
  client_addr->ai_socktype = SOCK_DGRAM;
  client_addr->ai_protocol = IPPROTO_UDP;

  /* Receive the word back from the server */
  fprintf(stdout, "Received: ");
  if ((received = recvfrom(sock, buffer, BUFFSIZE, 0,
    client_addr->ai_addr, &(client_addr->ai_addrlen))) != echolen) {
      free(client_addr);
      Die("Mismatch in number of received bytes");
  }

  buffer[received] = '\0';        /* Assure null terminated string */
  printf("%s", buffer);
  printf("\n");

  free(client_addr);

  close(sock);
  exit(0);
}

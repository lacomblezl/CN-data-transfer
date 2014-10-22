
/*
 *  rtp.h (Reliable Transfer Protocol)
 *
 *  INGI2141 - Computer Networks
 *
 *  AUTHORS :   Lacomblez Loic (2756-11-00)
 *              Laurent Quentin (...)
 *
 *  DATE : october 2014
 * =============================================================================
 * L'idee de ce protocole est que le concept de base est la connection, et pas
 * le socket. De ce fait, chaque connection ouvre implicitement un socket et
 * retourne le descripteur du socket en le considerant conceptuellement comme le
 * descripteur de la connection.
 */
/*
*	Structure d'un paquet
*/
#include <stdint.h>
typedef struct __attribute__((packed)){
	uint8_t type : 3;
	uint8_t window : 5;
	uint8_t seqnum : 8;
	uint16_t length : 16;
	uint8_t payload[512];
	uint32_t crc;
}packetstruct;

/*
 * Tente d'etablir une connection avec l'host specifie, et retourne -1 en
 * cas d'erreur.
 *
 */
 int connect_up(char *address, char *port, struct **addrinfo addr);


/*
 * Tente d'etablir une connection avec l'host specifie.
 */
 int listen(char *address, char *port, struct **addrinfo addr);


/*
 * Envoie un segment a l'host connecte par la connection
 */
 int send(int connect_id, void *bytes);


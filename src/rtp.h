
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

#include <stdint.h>

/*
 * Structure decrivant le frame utilise par le protocole
 */
typedef struct __attribute__((packed)){
	uint8_t type : 3;
	uint8_t window : 5;
	uint8_t seqnum : 8;
	uint16_t length : 16;
	uint8_t payload[512];
	uint32_t crc;
} packetstruct;

/* === TODO: DONE BY LOIC ===
 * Initializes the socket needed to communicate on top on UDP.
 * One of the address MUST BE NULL, the other is the one used
 * to bind/connect the socket.
 *
 * PARAMETERS :
 *	- dest_addr : adrrinfo concerning the destination address and
 *				ports. Used to instanciate a Sender socket.
 *  - src_addr : addrinfo defining the address and ports we are
 *				listenning to. Used to instanciate a Receiver socket.
 *
 * RETURN :
 *	The identifier of the created socket, or -1 in case of error.
 */
int init_host(struct addrinfo *dest_addr, struct addrinfo *src_addr);

/* //TODO: corriger les options et specifier
 * Tente d'etablir une connection fiable avec l'host specifie, et retourne
* -1 en cas d'erreur.
 */
int connect_up(int sock_id);

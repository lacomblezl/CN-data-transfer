
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
#include <netdb.h>
#include <zlib.h>
#include <stdbool.h>

#define PAYLOADSIZE 512
#define PTYPE_DATA 1
#define PTYPE_ACK 2

/*
 * Structure decrivant le frame utilise par le protocole
 */
typedef struct blah {
	uint8_t type : 3;
	uint8_t window : 5;
	uint8_t seqnum : 8;
	uint16_t length : 16;
	uint8_t payload[PAYLOADSIZE];
	uint32_t crc;
} __attribute__((packed)) packetstruct;


/*
 * Enum qui permet de definir l'host comme sender ou receiver
 */
enum host_type {
	sender = 0,
	receiver = 1 };


/*
 * Initializes the socket needed to communicate on top on UDP.
 * The type is used to specify if the socket should be bound or connected
 * to address.
 *
 * PARAMETERS :
 *	- address : adrrinfo structure properly initialized for a given address.
 *  - type : host_type defining if the socket will be used by a sender or a
 *		receiver. If it's a sender, the socket is connected to 'address'.
 *		Else, the socket is bound to 'address'.
 *
 * RETURN :
 *	The identifier of the created socket. In case of error, -1 is returned and
 *	errno is set appropriatly.
 */
int init_host(struct addrinfo *address, enum host_type type);


/* //TODO: corriger les options, specifier et implementer !
 * Tente d'etablir une connection fiable avec l'host specifie, et retourne
* -1 en cas d'erreur.
 */
int connect_up(int sock_id);


/*
 * Computes the CRC given the following data and binds it to result.
 * Returns 0, or -1 if an error was encountered
 */
int compute_crc(packetstruct* packet, uint32_t *result);


/*
 * Determine si le packet recu est valide en verifiant son CRC...
 * Retourne true si le packet est valide, false sinon.
 */
bool packet_valid(packetstruct* packet);

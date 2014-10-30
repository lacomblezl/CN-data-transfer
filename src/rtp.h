
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
typedef struct __attribute__((__packed__)) {
	uint8_t window : 5;	// necessary order to respect the specifications !
	uint8_t type : 3;	// maybe we first define the least significant bits ??
	uint8_t seqnum;
	uint16_t length;
	uint8_t payload[PAYLOADSIZE];
	uint32_t crc;
} packetstruct;

/*
* sender's window slot definition
*/
typedef struct slot {
	uint8_t seqnum;
    bool received;      
	clock_t timesent;
} window_slot;

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

/*
 * Computes the CRC given the following data and binds it to result.
 * Returns 0, or -1 if an error was encountered. Packet is NOT modified.
 */
int compute_crc(packetstruct* packet, uint32_t *result);


/*
 * Converts the endiannes of the concerned fields and determine if
 * 'packet' has a valid CRC...
 * RETURN :
 *	1 if the packet is valid, 0 if not and -1 in case of an error. 'packet' is
 *	modified by calling ntohs on the crc and length fields.
 */
int packet_valid(packetstruct* packet);

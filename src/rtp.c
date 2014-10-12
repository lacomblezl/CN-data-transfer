/*
 *  rtp.c (Reliable Transfer Protocol)
 *
 *  INGI2141 - Computer Networks
 *
 *  AUTHORS :   Lacomblez Loic (2756-11-00)
 *              Laurent Quentin (...)
 *
 *  DATE : october 2014
 */

#include "rtp.h"


/*
 * Structure decrivant le frame utilise par le protocole
 */
typedef struct frame {
  char header [4];        // 4 bytes = 32 bits
  char payload [512];
  char crc [4];
} rtp_frame;



/*
 * Computes the CRC given the following data
 */
int compute_crc(char* header, char* payload)
{
  //TODO implement crc...
  return 0;
}

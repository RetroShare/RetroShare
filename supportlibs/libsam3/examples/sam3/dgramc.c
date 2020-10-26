/* This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 * I2P-Bote:
 * 5m77dFKGEq6~7jgtrfw56q3t~SmfwZubmGdyOLQOPoPp8MYwsZ~pfUCwud6LB1EmFxkm4C3CGlzq-hVs9WnhUV
 * we are the Borg. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../libsam3/libsam3.h"

// comment the following if you don't want to stress UDP with 'big' datagram
// seems that up to 32000 bytes can be used for localhost
// note that we need 516+6+? bytes for header; lets reserve 1024 bytes for it
#define BIG (32000 - 1024)

#define KEYFILE "dgrams.key"

int main(int argc, char *argv[]) {
  Sam3Session ses;
  char buf[1024];
  char destkey[517] = {0}; // 516 chars + \0
  int sz;
  //
  libsam3_debug = 1;
  //
  if (argc < 2) {
    FILE *fl = fopen(KEYFILE, "rb");
    //
    if (fl != NULL) {
      if (fread(destkey, 516, 1, fl) == 1) {
        fclose(fl);
        goto ok;
      }
      fclose(fl);
    }
    printf("usage: dgramc PUBKEY\n");
    return 1;
  } else {
    if (strlen(argv[1]) != 516) {
      fprintf(stderr, "FATAL: invalid key length!\n");
      return 1;
    }
    strcpy(destkey, argv[1]);
  }
  //
ok:
  printf("creating session...\n");
  /* create TRANSIENT session with temporary disposible destination */
  if (sam3CreateSession(&ses, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT,
                        SAM3_DESTINATION_TRANSIENT, SAM3_SESSION_DGRAM, 4,
                        NULL) < 0) {
    fprintf(stderr, "FATAL: can't create session\n");
    return 1;
  }
  /* send datagram */
  printf("sending test datagram...\n");
  if (sam3DatagramSend(&ses, destkey, "test", 4) < 0) {
    fprintf(stderr, "ERROR: %s\n", ses.error);
    goto error;
  }
  /** receive reply */
  if ((sz = sam3DatagramReceive(&ses, buf, sizeof(buf) - 1)) < 0) {
    fprintf(stderr, "ERROR: %s\n", ses.error);
    goto error;
  }
  /** null terminated string */
  buf[sz] = 0;
  printf("received: [%s]\n", buf);
  //
#ifdef BIG
  {
    char *big = calloc(BIG + 1024, sizeof(char));
    /** generate random string */
    sam3GenChannelName(big, BIG + 1023, BIG + 1023);
    printf("sending BIG datagram...\n");
    if (sam3DatagramSend(&ses, destkey, big, BIG) < 0) {
      free(big);
      fprintf(stderr, "ERROR: %s\n", ses.error);
      goto error;
    }
    if ((sz = sam3DatagramReceive(&ses, big, BIG + 512)) < 0) {
      free(big);
      fprintf(stderr, "ERROR: %s\n", ses.error);
      goto error;
    }
    big[sz] = 0;
    printf("received (%d): [%s]\n", sz, big);
    free(big);
  }
#endif
  //
  printf("sending quit datagram...\n");
  if (sam3DatagramSend(&ses, destkey, "quit", 4) < 0) {
    fprintf(stderr, "ERROR: %s\n", ses.error);
    goto error;
  }
  if ((sz = sam3DatagramReceive(&ses, buf, sizeof(buf) - 1)) < 0) {
    fprintf(stderr, "ERROR: %s\n", ses.error);
    goto error;
  }
  buf[sz] = 0;
  printf("received: [%s]\n", buf);
  //
  sam3CloseSession(&ses);
  return 0;
error:
  sam3CloseSession(&ses);
  return 1;
}

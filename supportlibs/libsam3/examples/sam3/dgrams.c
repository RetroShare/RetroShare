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

#define KEYFILE "dgrams.key"

int main(int argc, char *argv[]) {
  Sam3Session ses;
  char privkey[1024], pubkey[1024], buf[33 * 1024];

  /** quit command */
  const char *quitstr = "quit";
  const size_t quitlen = strlen(quitstr);

  /** reply response */
  const char *replystr = "reply: ";
  const size_t replylen = strlen(replystr);

  FILE *fl;
  //
  libsam3_debug = 1;
  //

  /** generate new destination keypair */
  printf("generating keys...\n");
  if (sam3GenerateKeys(&ses, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT, 4) < 0) {
    fprintf(stderr, "FATAL: can't generate keys\n");
    return 1;
  }
  /** copy keypair into local buffer */
  strncpy(pubkey, ses.pubkey, sizeof(pubkey));
  strncpy(privkey, ses.privkey, sizeof(privkey));
  /** create sam session */
  printf("creating session...\n");
  if (sam3CreateSession(&ses, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT, privkey,
                        SAM3_SESSION_DGRAM, 5, NULL) < 0) {
    fprintf(stderr, "FATAL: can't create session\n");
    return 1;
  }
  /** make sure we have the right destination */
  // FIXME: probably not needed
  if (strcmp(pubkey, ses.pubkey) != 0) {
    fprintf(stderr, "FATAL: destination keys don't match\n");
    sam3CloseSession(&ses);
    return 1;
  }
  /** print destination to stdout */
  printf("PUB KEY\n=======\n%s\n=======\n", ses.pubkey);
  if ((fl = fopen(KEYFILE, "wb")) != NULL) {
    /** write public key to keyfile */
    fwrite(pubkey, strlen(pubkey), 1, fl);
    fclose(fl);
  }

  /* now listen for UDP packets */
  printf("starting main loop...\n");
  for (;;) {
    /** save replylen bytes for out reply at begining */
    char *datagramBuf = buf + replylen;
    const size_t datagramMaxLen = sizeof(buf) - replylen;
    int sz, isquit;
    printf("waiting for datagram...\n");
    /** blocks until we get a UDP packet */
    if ((sz = sam3DatagramReceive(&ses, datagramBuf, datagramMaxLen) < 0)) {
      fprintf(stderr, "ERROR: %s\n", ses.error);
      goto error;
    }
    /** ensure null terminated string */
    datagramBuf[sz] = 0;
    /** print out datagram payload to user */
    printf("FROM\n====\n%s\n====\n", ses.destkey);
    printf("SIZE=%d\n", sz);
    printf("data: [%s]\n", datagramBuf);
    /** check for "quit" */
    isquit = (sz == quitlen && memcmp(datagramBuf, quitstr, quitlen) == 0);
    /** echo datagram back to sender with "reply: " at the beginning */
    memcpy(buf, replystr, replylen);

    if (sam3DatagramSend(&ses, ses.destkey, buf, sz + replylen) < 0) {
      fprintf(stderr, "ERROR: %s\n", ses.error);
      goto error;
    }
    /** if we got a quit command wait for 10 seconds and break out of the
     * mainloop */
    if (isquit) {
      printf("shutting down...\n");
      sleep(10); /* let dgram reach it's destination */
      break;
    }
  }
  /** close session and delete keyfile */
  sam3CloseSession(&ses);
  unlink(KEYFILE);
  return 0;
error:
  /** error case, close session, delete keyfile and return exit code 1 */
  sam3CloseSession(&ses);
  unlink(KEYFILE);
  return 1;
}

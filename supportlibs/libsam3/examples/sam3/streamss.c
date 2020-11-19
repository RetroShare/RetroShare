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

#define KEYFILE "streams.key"

int main(int argc, char *argv[]) {
  Sam3Session ses;
  Sam3Connection *conn;
  FILE *fl;
  //
  libsam3_debug = 1;
  //
  printf("creating session...\n");
  // create TRANSIENT session
  if (sam3CreateSilentSession(&ses, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT,
                              SAM3_DESTINATION_TRANSIENT, SAM3_SESSION_STREAM,
                              4, NULL) < 0) {
    fprintf(stderr, "FATAL: can't create session\n");
    return 1;
  }
  //
  printf("PUB KEY\n=======\n%s\n=======\n", ses.pubkey);
  if ((fl = fopen(KEYFILE, "wb")) != NULL) {
    fwrite(ses.pubkey, strlen(ses.pubkey), 1, fl);
    fclose(fl);
  }
  //
  printf("starting stream acceptor...\n");
  if ((conn = sam3StreamAccept(&ses)) == NULL) {
    fprintf(stderr, "FATAL: can't accept: %s\n", ses.error);
    sam3CloseSession(&ses);
    return 1;
  }
  printf("FROM\n====\n%s\n====\n", conn->destkey);
  //
  printf("starting main loop...\n");
  for (;;) {
    char cmd[256];
    //
    if (sam3tcpReceiveStr(conn->fd, cmd, sizeof(cmd)) < 0)
      goto error;
    printf("cmd: [%s]\n", cmd);
    if (strcmp(cmd, "quit") == 0)
      break;
    // echo command
    if (sam3tcpPrintf(conn->fd, "re: %s\n", cmd) < 0)
      goto error;
  }
  //
  sam3CloseSession(&ses);
  unlink(KEYFILE);
  return 0;
error:
  fprintf(stderr, "FATAL: some error occured!\n");
  sam3CloseSession(&ses);
  unlink(KEYFILE);
  return 1;
}

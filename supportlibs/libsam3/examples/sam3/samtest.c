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

int main(int argc, char *argv[]) {
  int fd;
  SAMFieldList *rep = NULL;
  const char *v;
  //
  libsam3_debug = 1;
  //
  //
  if ((fd = sam3Handshake(NULL, 0, NULL)) < 0)
    return 1;
  //
  if (sam3tcpPrintf(fd, "DEST GENERATE\n") < 0)
    goto error;
  rep = sam3ReadReply(fd);
  // sam3DumpFieldList(rep);
  if (!sam3IsGoodReply(rep, "DEST", "REPLY", "PUB", NULL))
    goto error;
  if (!sam3IsGoodReply(rep, "DEST", "REPLY", "PRIV", NULL))
    goto error;
  v = sam3FindField(rep, "PUB");
  printf("PUB KEY\n=======\n%s\n", v);
  v = sam3FindField(rep, "PRIV");
  printf("PRIV KEY\n========\n%s\n", v);
  sam3FreeFieldList(rep);
  rep = NULL;
  //
  sam3FreeFieldList(rep);
  sam3tcpDisconnect(fd);
  return 0;
error:
  sam3FreeFieldList(rep);
  sam3tcpDisconnect(fd);
  return 1;
}

/* This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 * I2P-Bote:
 * 5m77dFKGEq6~7jgtrfw56q3t~SmfwZubmGdyOLQOPoPp8MYwsZ~pfUCwud6LB1EmFxkm4C3CGlzq-hVs9WnhUV
 * we are the Borg. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../libsam3/libsam3.h"

int main(int argc, char *argv[]) {
  Sam3Session ses;
  //
  //
  libsam3_debug = 1;
  //
  if (argc < 2) {
    printf("usage: %s name [name...]\n", argv[0]);
    return 1;
  }
  /** for each name in arguments ... */
  for (int n = 1; n < argc; ++n) {
    if (!getenv("I2P_LOOKUP_QUIET")) {
      fprintf(stdout, "%s ... ", argv[n]);
      fflush(stdout);
    }
    /** do oneshot name lookup */
    if (sam3NameLookup(&ses, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT, argv[n]) >=
        0) {
      fprintf(stdout, "%s\n\n", ses.destkey);
    } else {
      fprintf(stdout, "FAILED [%s]\n", ses.error);
    }
  }
  //
  return 0;
}

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

#include "../../src/ext/tinytest.h"
#include "../../src/ext/tinytest_macros.h"
#include "../../src/libsam3/libsam3.h"

static int testb32(const char *src, const char *res) {
  size_t dlen = sam3Base32EncodedLength(strlen(src)), len;
  char dest[128];
  //
  len = sam3Base32Encode(dest, sizeof(dest), src, strlen(src));
  tt_int_op(len, ==, dlen);
  tt_int_op(len, ==, strlen(res));
  tt_str_op(res, ==, dest);
  return 1;

end:
  return 0;
}

void test_b32_encode(void *data) {
  (void)data; /* This testcase takes no data. */

  tt_assert(testb32("", ""));
  tt_assert(testb32("f", "my======"));
  tt_assert(testb32("fo", "mzxq===="));
  tt_assert(testb32("foo", "mzxw6==="));
  tt_assert(testb32("foob", "mzxw6yq="));
  tt_assert(testb32("fooba", "mzxw6ytb"));
  tt_assert(testb32("foobar", "mzxw6ytboi======"));

end:;
}

struct testcase_t b32_tests[] = {{
                                     "encode",
                                     test_b32_encode,
                                 },
                                 END_OF_TESTCASES};

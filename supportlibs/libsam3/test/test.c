#include <stddef.h>

#include "../src/ext/tinytest.h"
#include "../src/ext/tinytest_macros.h"

extern struct testcase_t b32_tests[];

struct testgroup_t test_groups[] = {{"b32/", b32_tests}, END_OF_GROUPS};

int main(int argc, const char **argv) {
  return tinytest_main(argc, argv, test_groups);
}

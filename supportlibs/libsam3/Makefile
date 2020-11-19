CFLAGS := -Wall -g -O2 -std=gnu99

SRCS := \
	src/libsam3/libsam3.c \
	src/libsam3a/libsam3a.c

TESTS := \
	src/ext/tinytest.c \
	test/test.c \
	test/libsam3/test_b32.c

LIB_OBJS := ${SRCS:.c=.o}
TEST_OBJS := ${TESTS:.c=.o}

OBJS := ${LIB_OBJS} ${TEST_OBJS}

LIB := libsam3.a

all: build check

check: libsam3-tests
	./libsam3-tests

build: ${LIB}

${LIB}: ${LIB_OBJS}
	${AR} -sr ${LIB} ${LIB_OBJS}

libsam3-tests: ${TEST_OBJS} ${LIB}
	${CC} $^ -o $@

clean:
	rm -f libsam3-tests ${LIB} ${OBJS} examples/sam3/samtest

%.o: %.c Makefile
	${CC} ${CFLAGS} $(LDFLAGS) -c $< -o $@

fmt:
	find . -name '*.c' -exec clang-format -i {} \;
	find . -name '*.h' -exec clang-format -i {} \;

info:
	@echo $(AR)

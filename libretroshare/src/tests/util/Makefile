
RS_TOP_DIR = ../..
DHT_TOP_DIR = ../../../../libbitdht/src
OPS_TOP_DIR = ../../../../openpgpsdk/src
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/tests/scripts/config.mk
###############################################################

TESTOBJ  = dirtest.o compress_test.o sha1_test.o aes_test.o dchat_decrypt.o
TESTS  = dirtest sha1_test aes_test compress_test dchat_decrypt

all: tests

sha1_test: sha1_test.o 
	$(CC) $(CFLAGS) -o sha1_test sha1_test.o $(LIBS)
compress_test: compress_test.o 
	$(CC) $(CFLAGS) -o compress_test compress_test.o $(LIBS)
dirtest: dirtest.o 
	$(CC) $(CFLAGS) -o dirtest dirtest.o $(LIBS)
dirtest: aes_test.o 
	$(CC) $(CFLAGS) -o aes_test aes_test.o $(LIBS)
dchat_decrypt: dchat_decrypt.o 
	$(CC) $(CFLAGS) -o dchat_decrypt dchat_decrypt.o $(LIBS)

###############################################################
include $(RS_TOP_DIR)/tests/scripts/rules.mk
###############################################################


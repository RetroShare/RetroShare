
RS_TOP_DIR = ../..
DHT_TOP_DIR = ../../../../libbitdht/src
OPS_TOP_DIR = ../../../../openpgpsdk/src
OPENPGP_INCLUDE_DIR = ../../../../openpgpsdk/src

##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/tests/scripts/config.mk
###############################################################

TESTOBJ  = test_pgp_handler.o test_pgp_signature_parsing.o test_key_parsing.o test_certificate.o
TESTS    = test_pgp_handler test_pgp_signature_parsing test_key_parsing test_certificate

#rsbaseitem_test

all: tests

test_certificate : test_certificate.o
	$(CC) $(CFLAGS)  -o test_certificate test_certificate.o $(OBJ) $(LIBS) -L../../../../openpgpsdk/src/lib/ -lops -lbz2

test_pgp_handler : test_pgp_handler.o
	$(CC) $(CFLAGS)  -o test_pgp_handler test_pgp_handler.o $(OBJ) $(LIBS) -L../../../../openpgpsdk/src/lib/ -lops -lbz2

test_pgp_signature_parsing : test_pgp_signature_parsing.o
	$(CC) $(CFLAGS)  -o test_pgp_signature_parsing test_pgp_signature_parsing.o $(OBJ) $(LIBS) -L../../../../openpgpsdk/src/lib/ -lops -lbz2

test_key_parsing : test_key_parsing.o
	$(CC) $(CFLAGS)  -o test_key_parsing test_key_parsing.o ../../../../openpgpsdk/src/lib/libops.a -lssl -lcrypto -lbz2

###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################


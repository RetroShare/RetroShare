
RS_TOP_DIR = ../..
DHT_TOP_DIR = ../../../../libbitdht/src
OPENPGP_INCLUDE_DIR = ../../../../openpgpsdk/include

##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/tests/scripts/config.mk
###############################################################

TESTOBJ  = test_pgp_handler.o test_pgp_signature_parsing.o
TESTS    = test_pgp_handler test_pgp_signature_parsing

#rsbaseitem_test

all: tests

test_pgp_handler : test_pgp_handler.o
	$(CC) $(CFLAGS) -o test_pgp_handler test_pgp_handler.o $(OBJ) $(LIBS) -L../../../../openpgpsdk/lib/ -lops -lbz2

test_pgp_signature_parsing : test_pgp_signature_parsing.o
	$(CC) $(CFLAGS) -o test_pgp_signature_parsing test_pgp_signature_parsing.o $(OBJ) $(LIBS) -L../../../../openpgpsdk/lib/ -lops -lbz2

###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################


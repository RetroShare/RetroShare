!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TEMPLATE = lib
macx {
	CONFIG = staticlib 
} else {
	CONFIG += staticlib
}

DEFINES *= OPENSSL_NO_IDEA 

TARGET = ops
DESTDIR = lib
DEPENDPATH += .
INCLUDEPATH += .

#################################### Windows #####################################

win32 {
	DEFINES *= WIN32_LEAN_AND_MEAN _USE_32BIT_TIME_T
	DEPENDPATH += $$INC_DIR
	INCLUDEPATH += $$INC_DIR
}

# Input
HEADERS += openpgpsdk/writer.h \
           openpgpsdk/writer_armoured.h \
           openpgpsdk/version.h \
           openpgpsdk/validate.h \
           openpgpsdk/util.h \
           openpgpsdk/types.h \
           openpgpsdk/streamwriter.h \
           openpgpsdk/std_print.h \
           openpgpsdk/signature.h \
           openpgpsdk/readerwriter.h \
           openpgpsdk/random.h \
           openpgpsdk/partial.h \
           openpgpsdk/packet-show.h \
           openpgpsdk/packet-show-cast.h \
           openpgpsdk/packet-parse.h \
           openpgpsdk/packet.h \
           openpgpsdk/memory.h \
           openpgpsdk/literal.h \
           openpgpsdk/lists.h \
           openpgpsdk/keyring.h \
           openpgpsdk/hash.h \
           openpgpsdk/final.h \
           openpgpsdk/defs.h \
           openpgpsdk/errors.h \
           openpgpsdk/crypto.h \
           openpgpsdk/create.h \
           openpgpsdk/configure.h \
           openpgpsdk/compress.h \
           openpgpsdk/callback.h \
           openpgpsdk/accumulate.h \
           openpgpsdk/armour.h \
           openpgpsdk/parse_local.h \
           openpgpsdk/keyring_local.h \
           openpgpsdk/opsdir.h \
           openpgpsdk/opsstring.h

SOURCES += openpgpsdk/accumulate.c \
           openpgpsdk/compress.c \
           openpgpsdk/create.c \
           openpgpsdk/crypto.c \
           openpgpsdk/errors.c \
           openpgpsdk/fingerprint.c \
           openpgpsdk/hash.c \
           openpgpsdk/keyring.c \
           openpgpsdk/lists.c \
           openpgpsdk/memory.c \
           openpgpsdk/openssl_crypto.c \
           openpgpsdk/packet-parse.c \
           openpgpsdk/packet-print.c \
           openpgpsdk/packet-show.c \
           openpgpsdk/random.c \
           openpgpsdk/reader.c \
           openpgpsdk/reader_armoured.c \
           openpgpsdk/reader_encrypted_se.c \
           openpgpsdk/reader_encrypted_seip.c \
           openpgpsdk/reader_fd.c \
           openpgpsdk/reader_hashed.c \
           openpgpsdk/reader_mem.c \
           openpgpsdk/readerwriter.c \
           openpgpsdk/signature.c \
           openpgpsdk/symmetric.c \
           openpgpsdk/util.c \
           openpgpsdk/validate.c \
           openpgpsdk/writer.c \
           openpgpsdk/writer_armour.c \
           openpgpsdk/writer_partial.c \
           openpgpsdk/writer_literal.c \
           openpgpsdk/writer_encrypt.c \
           openpgpsdk/writer_encrypt_se_ip.c \
           openpgpsdk/writer_fd.c \
           openpgpsdk/writer_memory.c \
           openpgpsdk/writer_skey_checksum.c \
           openpgpsdk/writer_stream_encrypt_se_ip.c \
           openpgpsdk/opsdir.c \
           openpgpsdk/opsstring.c

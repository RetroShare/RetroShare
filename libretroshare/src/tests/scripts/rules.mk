
# defines required / used.
#
# CFLAGS
#
#

librs:  $(RSOBJ)
	$(AR) r $(LIBRS) $(RSOBJ)
	$(RANLIB) $(LIBRS)

.cc.o:
	$(CC) $(CFLAGS) -c $<

clean:
	-/bin/rm $(RSOBJ) $(EXECOBJ) $(TESTOBJ)

clobber: clean retest
	-/bin/rm $(EXEC) $(TESTS)


include $(RS_TOP_DIR)/tests/scripts/regress.mk


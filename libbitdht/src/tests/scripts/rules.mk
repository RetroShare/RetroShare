
# defines required / used.
#
# CFLAGS
#
#

.cc.o:
	$(CC) $(CFLAGS) -c $<

clean:
	-/bin/rm $(EXECOBJ) $(TESTOBJ)

clobber: clean retest
	-/bin/rm $(EXEC) $(TESTS)


include $(TEST_TOP_DIR)/scripts/regress.mk


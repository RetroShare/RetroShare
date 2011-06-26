
testoutputfiles = $(foreach tt,$(1),$(tt).tstout)

%.tstout : %.sh %
	-sh ./$< > $@ 2>&1

%.tstout : %
	-./$< > $@ 2>&1

TESTOUT = $(call testoutputfiles,$(TESTS))

.phony : tests regress retest clobber

tests: $(TESTS)

regress: $(TESTOUT)
	@-echo "--------------- SUCCESS (count):"
	@-grep -c SUCCESS $(TESTOUT)
	@-echo "--------------- FAILURE REPORTS:"
	@-grep FAILURE $(TESTOUT) || echo no failures
	@-echo "--------------- end"

retest:
	-/bin/rm $(TESTOUT)


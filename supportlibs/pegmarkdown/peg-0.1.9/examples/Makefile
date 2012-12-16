EXAMPLES = test rule accept wc dc dcv calc basic localctx

CFLAGS = -g -O3

DIFF = diff
TEE = cat >

all : $(EXAMPLES)

test : .FORCE
	../peg -o test.peg.c test.peg
	$(CC) $(CFLAGS) -o test test.c
	echo 'ab.ac.ad.ae.afg.afh.afg.afh.afi.afj.' | ./$@ | $(TEE) $@.out
	$(DIFF) $@.ref $@.out
	rm -f $@.out
	@echo

rule : .FORCE
	../peg -o rule.peg.c rule.peg
	$(CC) $(CFLAGS) -o rule rule.c
	echo 'abcbcdabcbcdabcbcdabcbcd' | ./$@ | $(TEE) $@.out
	$(DIFF) $@.ref $@.out
	rm -f $@.out
	@echo

accept : .FORCE
	../peg -o accept.peg.c accept.peg
	$(CC) $(CFLAGS) -o accept accept.c
	echo 'abcbcdabcbcdabcbcdabcbcd' | ./$@ | $(TEE) $@.out
	$(DIFF) $@.ref $@.out
	rm -f $@.out
	@echo

wc : .FORCE
	../leg -o wc.leg.c wc.leg
	$(CC) $(CFLAGS) -o wc wc.leg.c
	cat wc.leg | ./$@ | $(TEE) $@.out
	$(DIFF) $@.ref $@.out
	rm -f $@.out
	@echo

dc : .FORCE
	../peg -o dc.peg.c dc.peg
	$(CC) $(CFLAGS) -o dc dc.c
	echo '  2  *3 *(3+ 4) ' | ./dc | $(TEE) $@.out
	$(DIFF) $@.ref $@.out
	rm -f $@.out
	@echo

dcv : .FORCE
	../peg -o dcv.peg.c dcv.peg
	$(CC) $(CFLAGS) -o dcv dcv.c
	echo 'a = 6;  b = 7;  a * b' | ./dcv | $(TEE) $@.out
	$(DIFF) $@.ref $@.out
	rm -f $@.out
	@echo

calc : .FORCE
	../leg -o calc.leg.c calc.leg
	$(CC) $(CFLAGS) -o calc calc.leg.c
	echo 'a = 6;  b = 7;  a * b' | ./calc | $(TEE) $@.out
	$(DIFF) $@.ref $@.out
	rm -f $@.out
	@echo

basic : .FORCE
	../leg -o basic.leg.c basic.leg
	$(CC) $(CFLAGS) -o basic basic.leg.c
	( echo 'load "test"'; echo "run" ) | ./basic | $(TEE) $@.out
	$(DIFF) $@.ref $@.out
	rm -f $@.out
	@echo

localctx : .FORCE
	../peg -o test.peg.c test.peg
	$(CC) $(CFLAGS) -o localctx localctx.c
	echo 'ab.ac.ad.ae.afg.afh.afg.afh.afi.afj.' | ./$@ | $(TEE) $@.out
	$(DIFF) $@.ref $@.out
	rm -f $@.out
	@echo

clean : .FORCE
	rm -f *~ *.o *.[pl]eg.[cd] $(EXAMPLES)
	rm -rf *.dSYM

spotless : clean

.FORCE :

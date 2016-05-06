#
# credits: 01..13.c from the pcc cpp-tests suite
#

TCC = ../../tcc
TESTS = $(patsubst %.c,%.test,$(wildcard *.c))
TESTS += $(patsubst %.S,%.test,$(wildcard *.S))

all test : $(sort $(TESTS))

DIFF_OPTS = -Nu -b -B -I "^\#"

%.test: %.c %.expect
	@echo PPTest $* ...
	-@$(TCC) -E -P $< >$*.output 2>&1 ; \
	    diff $(DIFF_OPTS) $*.expect $*.output \
	    && rm -f $*.output

%.test: %.S %.expect
	@echo PPTest $* ...
	-@$(TCC) -E -P $< >$*.output 2>&1 ; \
	    diff $(DIFF_OPTS) $*.expect $*.output \
	    && rm -f $*.output

# automatically generate .expect files with gcc:
%.expect: %.c
	gcc -E -P $*.c >$*.expect 2>&1

%.expect: %.S
	gcc -E -P $*.S >$*.expect 2>&1

# tell make not to delete
.PRECIOUS: %.expect

clean:
	rm -vf *.output

02.test : DIFF_OPTS += -w
15.test : DIFF_OPTS += -w

# diff options:
# -b ighore space changes
# -w ighore all whitespace
# -B ignore blank lines
# -I <RE> ignore lines matching RE

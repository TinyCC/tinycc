all: test cvt

test: prog.bin 
	cmp -l prog.bin prog.bin.ref

run: tc prog.c
	./tc prog.c

run2: tc tc1.c prog.c
	./tc tc1.c prog.c

run3: tc tc1.c prog.c
	./tc tc1.c tc1.c prog.c

prog.bin: prog.c tct
	./tct prog.c $@
	ndisasm -b 32 $@

p2.bin: p2.c tct
	./tct $< $@
	ndisasm -b 32 $@

tct: tc.c
	gcc -DTEST -O2 -g -o $@ $< -ldl

tc: tc.c Makefile
	gcc -O2 -Wall -g -o $@ $< -ldl

tc1: tc1.c
	gcc -O2 -Wall -g -o $@ $<

cvt: cvt.c
	gcc -O2 -Wall -g -o $@ $<

instr.o: instr.S
	gcc -O2 -Wall -g -c -o $@ $<

tc.i: tc.c Makefile
	gcc -E -P -DTINY -o $@ tc.c

tc1.c: tc.i cvt Makefile
	./cvt $< $@
	@ls -l $@

test2: tct tc1.c
	./tct tc1.c tc2
	ndisasm -b 32 tc2

tc2: tc
	./tct < tc1.c > tc2
	ndisasm -b 32 tc2


all: test cvt

test: prog.bin 
	cmp -l prog.bin prog.bin.ref

run: tcc prog.c
	./tcc prog.c

run2: tcc tcc1.c prog.c
	./tcc tcc1.c prog.c

run3: tcc tcc1.c prog.c
	./tcc tcc1.c tcc1.c prog.c

prog.bin: prog.c tcc
	./tc prog.c $@
	ndisasm -b 32 $@

p2.bin: p2.c tcc
	./tcc $< $@
	ndisasm -b 32 $@

# Tiny C Compiler

tcc: tcc.c
	gcc -O2 -Wall -g -o $@ $< -ldl

tcc1: tcc1.c
	gcc -O2 -Wall -g -o $@ $<

tcc1.i: tcc.c Makefile
	gcc -E -P -o $@ $<

tcc1.c: tcc1.i cvt Makefile
	./cvt -d $< $@
	@ls -l $@

# obfuscated C compiler
otcc: otcc.c
	gcc -O2 -Wall -g -o $@ $< -ldl

otcc.i: otcc.c Makefile
	gcc -E -P -DTINY -o $@ $<

otcc1.c: otcc.i cvt Makefile
	./cvt $< $@
	@ls -l $@

orun: otcc otcc1.c
	./otcc otcc1.c ex1.c

# misc

cvt: cvt.c
	gcc -O2 -Wall -g -o $@ $<

instr.o: instr.S
	gcc -O2 -Wall -g -c -o $@ $<

clean:
	rm -f *~ *.o tcc tcc1 cvt 

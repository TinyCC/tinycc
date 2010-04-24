#!/bin/bash
a=$(mktemp)
b=$(mktemp)
c=$(mktemp)
cp config.h $a
cp config.mak $b
# force 386 build on x86_64
./configure --cpu=x86
# configure doesn't provide a way to set tccdir
tccdir=$(grep TCCDIR $a|awk '{gsub("\"","",$3);print $3}')
grep -v CONFIG_TCCDIR $a > $c
echo "#define CONFIG_TCCDIR \"${tccdir}/win32\"" >> $c
mv $c config.h
make i386-win32-tcc
mv i386-win32-tcc tcc.exe
sync
make CONFIG_WIN32=1 libtcc1.a
cp include/* win32/include
mv libtcc1.a win32/lib
mv $a config.h
mv $b config.mak
rm tcc.exe

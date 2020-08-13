# please refer to win32/tcc-win32.txt for more detail
# this script shows:
# * build tcc (with sys cc) and then tcc itself
# * to build cross i386-win32-tcc and x86_64-win32-tcc at osx/lnx
# * to build hello_win.c 32/64 with tccwin*.sh

R_=$(pwd)
#RTCC=$(cd `dirname $0`; pwd)
RTCC=$(cd `dirname $0`/../; pwd)
cd $RTCC
echo RTCC=$RTCC

make clean
./configure
make tcc
./configure --cc=./tcc
make tcc
#make cross
make cross-i386-win32
make cross-x86_64-win32

cd $R_
pwd

echo RTCC=$RTCC

sh $RTCC/win32/tccwin32.sh $RTCC/win32/examples/hello_win.c -o $RTCC/win32/examples/hello_win32.exe
sh $RTCC/win32/tccwin64.sh $RTCC/win32/examples/hello_win.c -o $RTCC/win32/examples/hello_win64.exe
ls -al $RTCC/win32/examples/hello_win*.exe

sh $RTCC/win32/tccwin32.sh $RTCC/tcc.c -DTCC_LIBTCC1="\"i386-win32-libtcc1.a\"" -o $RTCC/tcc32.exe
sh $RTCC/win32/tccwin64.sh $RTCC/tcc.c -DTCC_LIBTCC1="\"x86_64-win32-libtcc1.a\"" -o $RTCC/tcc64.exe
ls -al $RTCC/tcc*.exe

#sh $RTCC/win32/tccwin32.sh $RTCC/libtcc.c -DTCC_LIBTCC1="\"i386-win32-libtcc1.a\"" -DLIBTCC_AS_DLL -DTCC_TARGET_PE -DTCC_TARGET_I386 -o $RTCC/libtcc32.dll -shared
#sh $RTCC/win32/tccwin64.sh $RTCC/libtcc.c -DTCC_LIBTCC1="\"x86_64-win32-libtcc1.a\"" -DLIBTCC_AS_DLL -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -shared -o $RTCC/libtcc64.dll
sh $RTCC/win32/tccwin32.sh $RTCC/libtcc.c -DLIBTCC_AS_DLL -shared -o $RTCC/libtcc32.dll
sh $RTCC/win32/tccwin64.sh $RTCC/libtcc.c -DLIBTCC_AS_DLL -shared -o $RTCC/libtcc64.dll
ls -al $RTCC/libtcc*.dll

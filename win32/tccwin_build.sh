# please refer to win32/tcc-win32.txt for more detail
# this script shows:
# * build tcc (with sys cc) and then tcc itself
# * to build cross i386-win32-tcc and x86_64-win32-tcc at osx/lnx
# * to build hello_win.c 32/64 with tccwin*.sh

ROLD=$(pwd)
#RTCC=$(cd `dirname $0`; pwd)
RTCC=$(cd `dirname $0`/../; pwd)
cd $RTCC

make clean
./configure
make tcc
./configure --cc=./tcc
make tcc
#make cross
make cross-i386-win32
make cross-x86_64-win32

sh ./win32/tccwin32.sh ./win32/examples/hello_win.c -o ./win32/examples/hello_win32.exe
sh ./win32/tccwin64.sh ./win32/examples/hello_win.c -o ./win32/examples/hello_win64.exe
ls -al ./win32/examples/hello_win*.exe

sh ./win32/tccwin32.sh ./tcc.c -DTCC_LIBTCC1="\"i386-win32-libtcc1.a\"" -o ./tcc32.exe
sh ./win32/tccwin64.sh ./tcc.c -DTCC_LIBTCC1="\"x86_64-win32-libtcc1.a\"" -o ./tcc64.exe
ls -al ./tcc*.exe

sh ./win32/tccwin32.sh ./libtcc.c -DLIBTCC_AS_DLL -shared -o ./libtcc32.dll
sh ./win32/tccwin64.sh ./libtcc.c -DLIBTCC_AS_DLL -shared -o ./libtcc64.dll
ls -al ./libtcc*.dll

cd $ROLD
pwd
echo RTCC=$RTCC

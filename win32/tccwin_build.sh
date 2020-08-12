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


#!/bin/sh

if [ -z "$TESTSUITE_PATH" ]
then
  if [ -d "$HOME/gcc/gcc-3.2/gcc/testsuite/gcc.c-torture" ]
  then
    TESTSUITE_PATH="$HOME/gcc/gcc-3.2/gcc/testsuite/gcc.c-torture"
  fi
fi

if [ -z "$TESTSUITE_PATH" ]
then
  echo "gcc testsuite not found."
  echo "define TESTSUITE_PATH to point to the gcc.c-torture directory"
  exit 1
fi

if [ -z "$TCC_SOURCE_PATH" ]
then
  if [ -f "include/tccdefs.h" ]
  then
    TCC_SOURCE_PATH="."
  elif [ -f "../include/tccdefs.h" ]
  then
    TCC_SOURCE_PATH=".."
  elif [ -f "../tinycc/include/tccdefs.h" ]
  then
    TCC_SOURCE_PATH="../tinycc"
  fi
fi

if [ -z "$RUNTIME_DIR" ]
then
   RUNTIME_DIR=$XDG_RUNTIME_DIR
fi
if [ -z "$RUNTIME_DIR" ]
then
  RUNTIME_DIR="/tmp"
fi

if [ -z "$CC" ]
then
  if [ -z "$TCC_SOURCE_PATH" ]
  then
      echo "tcc not found."
      echo "define TCC_SOURCE_PATH to point to the tcc source path"
      exit 1
  fi

  TCC="./tcc -B. -I$TCC_SOURCE_PATH/ -I$TCC_SOURCE_PATH/include -DNO_TRAMPOLINES"
else
  TCC="$CC -O1 -Wno-implicit-int $CFLAGS"
fi

rm -f tcc.sum tcc.fail
nb_ok="0"
nb_skipped="0"
nb_failed="0"
nb_exe_failed="0"

# skip some failed tests not implemented in tcc
# builtin: gcc "__builtins_*"
# ieee: gcc "__builtins_*" in the ieee directory
# complex: C99 "_Complex" and gcc "__complex__"
# misc: stdc features, other arch, gcc extensions (example: gnu_inline in c89)
#

old_pwd="`pwd`"
cd "$TESTSUITE_PATH"

skip_builtin="`grep "_builtin_"  compile/*.c  execute/*.c  execute/ieee/*.c | cut -d ':' -f1 | cut -d '/' -f2 |  sort -u `"
skip_ieee="`grep "_builtin_"   execute/ieee/*.c | cut -d ':' -f1 | cut -d '/' -f3 |  sort -u `"
skip_complex="`grep -i "_Complex"  compile/*.c  execute/*.c  execute/ieee/*.c | cut -d ':' -f1 | cut -d '/' -f2 |  sort -u `"
skip_misc="20000120-2.c mipscop-1.c mipscop-2.c mipscop-3.c mipscop-4.c
   fp-cmp-4f.c fp-cmp-4l.c fp-cmp-8f.c fp-cmp-8l.c pr38016.c "

cd "$old_pwd"

for src in $TESTSUITE_PATH/compile/*.c ; do
  echo $TCC -o $RUNTIME_DIR/tst.o -c $src
  $TCC -o $RUNTIME_DIR/tst.o -c $src >> tcc.fail 2>&1
  if [ "$?" = "0" ] ; then
    result="PASS"
    nb_ok=$(( $nb_ok + 1 ))
  else
    base=`basename "$src"`
    skip_me="`echo  $skip_builtin $skip_ieee $skip_complex $skip_misc | grep -w $base`"

    if [ -n "$skip_me" ]
    then
      result="SKIP"
      nb_skipped=$(( $nb_skipped + 1 ))
    else
      result="FAIL"
      nb_failed=$(( $nb_failed + 1 ))
    fi
  fi
  echo "$result: $src"  >> tcc.sum
done

if [ -f "$RUNTIME_DIR/tst.o" ]
then
    rm -f "$RUNTIME_DIR/tst.o"
fi

for src in $TESTSUITE_PATH/execute/*.c  $TESTSUITE_PATH/execute/ieee/*.c ; do
  echo $TCC $src -o $RUNTIME_DIR/tst -lm
  $TCC $src -o $RUNTIME_DIR/tst -lm >> tcc.fail 2>&1
  if [ "$?" = "0" ] ; then
    result="PASS"
    if $RUNTIME_DIR/tst >> tcc.fail 2>&1
    then
      result="PASS"
      nb_ok=$(( $nb_ok + 1 ))
    else
      result="FAILEXE"
      nb_exe_failed=$(( $nb_exe_failed + 1 ))
    fi
  else
    base=`basename "$src"`
    skip_me="`echo $skip_builtin $skip_ieee $skip_complex $skip_misc | grep -w $base`"

    if [ -n "$skip_me" ]
    then
      result="SKIP"
      nb_skipped=$(( $nb_skipped + 1 ))
    else
      result="FAIL"
      nb_failed=$(( $nb_failed + 1 ))
    fi
  fi
  echo "$result: $src"  >> tcc.sum
done

if [ -f "$RUNTIME_DIR/tst.o" ]
then
    rm -f "$RUNTIME_DIR/tst.o"
fi
if [ -f "$RUNTIME_DIR/tst" ]
then
    rm -f "$RUNTIME_DIR/tst"
fi

echo "$nb_ok test(s) ok." >> tcc.sum
echo "$nb_ok test(s) ok."
echo "$nb_skipped test(s) skipped." >> tcc.sum
echo "$nb_skipped test(s) skipped."
echo "$nb_failed test(s) failed." >> tcc.sum
echo "$nb_failed test(s) failed."
echo "$nb_exe_failed test(s) exe failed." >> tcc.sum
echo "$nb_exe_failed test(s) exe failed."

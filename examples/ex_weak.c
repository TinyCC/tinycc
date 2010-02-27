#! /usr/local/bin/tcc -run
#include <tcclib.h>

extern void weak_f (void) __attribute__ ((weak));

int main ()
{
    if (weak_f) {
        weak_f();
    }
}

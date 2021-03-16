#pragma once

#include <stdint.h>

#include "atomic-x86.h"
#include "atomic-i386.c"

ATOMIC_X86_STORE(uint64_t, 8)
ATOMIC_X86_LOAD(uint64_t, 8)
ATOMIC_X86_COMPARE_EXCHANGE(uint64_t, 8, "q")

#include "atomic-gen64.c"

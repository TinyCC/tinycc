#pragma once

#include <stdint.h>

#include "atomic-x86.h"

ATOMIC_X86_STORE(uint8_t, 1)
ATOMIC_X86_STORE(uint16_t, 2)
ATOMIC_X86_STORE(uint32_t, 4)

ATOMIC_X86_LOAD(uint8_t, 1)
ATOMIC_X86_LOAD(uint16_t, 2)
ATOMIC_X86_LOAD(uint32_t, 4)

ATOMIC_X86_COMPARE_EXCHANGE(uint8_t, 1, "b")
ATOMIC_X86_COMPARE_EXCHANGE(uint16_t, 2, "w")
ATOMIC_X86_COMPARE_EXCHANGE(uint32_t, 4, "l")

#include "atomic-gen32.c"

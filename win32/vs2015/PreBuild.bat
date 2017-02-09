set /p VERSION= < %~dp0..\..\VERSION

:config.h
echo> .\config.h #define TCC_VERSION "%VERSION%"
echo>> .\config.h #ifdef TCC_TARGET_X86_64
echo>> .\config.h #define CONFIG_TCC_LIBPATHS "{B}/lib/64;{B}/lib"
echo>> .\config.h #else
echo>> .\config.h #define CONFIG_TCC_LIBPATHS "{B}/lib/32;{B}/lib"
echo>> .\config.h #endif

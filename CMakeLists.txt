cmake_minimum_required(VERSION 2.6...3.14)

project(tcc LANGUAGES C)

set(C_STANDARD 90)

include(CMakeDependentOption)
include(CheckCCompilerFlag)
include(GNUInstallDirs)
message("Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")

set(CMAKE_CONFIGURATION_TYPES=Debug;Release)
if(WIN32)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/TCC-BUILT" CACHE PATH "Install path prefix (default)" FORCE)
endif()
message(STATUS "Installing to ${CMAKE_INSTALL_PREFIX}")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_MODULE_PATH})

include(TargetArch)

# Set target architectures
target_architecture(CMAKE_TARGET_ARCHITECTURES)
list(LENGTH CMAKE_TARGET_ARCHITECTURES cmake_target_arch_len)
message("Target architecture type: ${CMAKE_TARGET_ARCHITECTURES}")

set(TCC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/TCC-BUILT")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")

enable_testing()
set( CORE_FILES
    ${TCC_DIR}/tcctools.c
    ${TCC_DIR}/libtcc.c
    ${TCC_DIR}/tccpp.c
    ${TCC_DIR}/tccgen.c
    ${TCC_DIR}/tccdbg.c
    ${TCC_DIR}/tccelf.c
    ${TCC_DIR}/tccasm.c
    ${TCC_DIR}/tccrun.c
    ${TCC_DIR}/tcc.h
    ${TCC_DIR}/config.h
    ${TCC_DIR}/libtcc.h
    ${TCC_DIR}/tcctok.h )

set( I386_FILES ${CORE_FILES}
    ${TCC_DIR}/i386-gen.c
    ${TCC_DIR}/i386-link.c
    ${TCC_DIR}/i386-asm.c
    ${TCC_DIR}/i386-asm.h
    ${TCC_DIR}/i386-tok.h )

set( I386_WIN32_FILES ${I386_FILES} ${TCC_DIR}/tccpe.c )
set( X86_64_FILES ${CORE_FILES}
    ${TCC_DIR}/x86_64-gen.c
    ${TCC_DIR}/x86_64-link.c
    ${TCC_DIR}/i386-asm.c
    ${TCC_DIR}/x86_64-asm.h )

set( X86_64_WIN32_FILES ${X86_64_FILES} ${TCC_DIR}/tccpe.c )
set( X86_64_OSX_FILES ${X86_64_FILES} ${TCC_DIR}/tccmacho.c )
set( ARM_FILES ${CORE_FILES}
    ${TCC_DIR}/arm-gen.c
    ${TCC_DIR}/arm-link.c
    ${TCC_DIR}/arm-asm.c
    ${TCC_DIR}/arm-tok.h )

set( ARM_WINCE_FILES ${ARM_FILES} ${TCC_DIR}/tccpe.c )
set( ARM64_FILES ${CORE_FILES} ${TCC_DIR}/arm64-gen.c ${TCC_DIR}/arm64-link.c ${TCC_DIR}/arm64-asm.c )
set( ARM64_WIN32_FILES ${ARM64_FILES} ${TCC_DIR}/tccpe.c )
set( ARM64_OSX_FILES ${ARM64_FILES} ${TCC_DIR}/tccmacho.c )
set( C67_FILES ${CORE_FILES} ${TCC_DIR}/c67-gen.c ${TCC_DIR}/c67-link.c ${TCC_DIR}/tcccoff.c )
set( RISCV64_FILES ${CORE_FILES} ${TCC_DIR}/riscv64-gen.c ${TCC_DIR}/riscv64-link.c ${TCC_DIR}/riscv64-asm.c )

# Set C compiler flags.
include( CheckCCompilerFlag )
check_c_compiler_flag( -fno-strict-aliasing, HAVE_NOSTRICTALIASING )
if( HAVE_NOSTRICTALIASING )
  add_definitions( -fno-strict-aliasing )
endif( HAVE_NOSTRICTALIASING )

set (TCC_INCLUDE_DIRS -I${TCC_DIR}/include)
if (WIN32)
    list(APPEND TCC_INCLUDE_DIRS
            -I${TCC_DIR}/win32/include -I${TCC_DIR}/win32/include/winapi)
endif()

if(UNIX)
  add_definitions( -Wall -O2 -Wdeclaration-after-statement -Wno-pointer-sign -Wno-sign-compare -Wno-unused-result )
  if(NOT APPLE)
    add_definitions( -Wno-format-truncation -Wno-stringop-truncation )
  elseif(APPLE)
    execute_process (
        COMMAND xcrun --show-sdk-path
        OUTPUT_VARIABLE mac_include
    )
    string(STRIP ${mac_include} mac_include)
    set(USR_INCLUDE ${mac_include}/usr/include )
    message(STATUS "OSX include location: ${USR_INCLUDE}")
    list(APPEND TCC_INCLUDE_DIRS -I${USR_INCLUDE} )
    add_definitions( -DCONFIG_USR_INCLUDE=\"${USR_INCLUDE}\" )
  endif()
endif()

file(READ ${TCC_DIR}/VERSION TCC_UNSTRIPPED_VERSION)
string(STRIP ${TCC_UNSTRIPPED_VERSION} TCC_VERSION)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/config.h)
file(STRINGS "VERSION" TCC_VERSION)
list(GET TCC_VERSION 0 TCC_VERSION)
configure_file(config.h.in ${TCC_DIR}/config.h)
configure_file(config.texi.in config.texi)

include_directories(${CMAKE_BINARY_DIR})

# Set target and sources
if( WIN32 AND NOT CYGWIN )
  if( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL i386 )
    set( NATIVE_TARGET TCC_TARGET_I386 )
    set( CODE_GENERATOR_SRCS ${I386_WIN32_FILES} )
  elseif( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL x86_64 )
    set( NATIVE_TARGET TCC_X86_64 )
    set( CODE_GENERATOR_SRCS ${X86_64_WIN32_FILES} )
  elseif( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm
    OR ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm64 )
        set( NATIVE_TARGET TCC_TARGET_ARM )
        set( CODE_GENERATOR_SRCS ${ARM_WINCE_FILES} )
  endif()
else()
  if( NOT APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL riscv64 )
    set( NATIVE_TARGET TCC_TARGET_RISCV64 )
    set( CODE_GENERATOR_SRCS ${RISCV64_FILES} )
  elseif( NOT APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm64 )
    set( NATIVE_TARGET TCC_TARGET_ARM64 )
    set( CODE_GENERATOR_SRCS ${ARM64_FILES} )
  elseif( NOT APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL i386 )
    set( NATIVE_TARGET TCC_TARGET_I386 )
    set( CODE_GENERATOR_SRCS ${I386_FILES} )
  elseif( NOT APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL x86_64 )
    set( NATIVE_TARGET TCC_X86_64 )
    set( CODE_GENERATOR_SRCS ${X86_64_FILES} )
  elseif( NOT APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm_eabi )
    set( NATIVE_TARGET TCC_ARM_EABI )
    set( CODE_GENERATOR_SRCS ${ARM_FILES} )
  elseif( NOT APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm )
    set( NATIVE_TARGET TCC_TARGET_ARM )
    set( CODE_GENERATOR_SRCS ${ARM_FILES} )
  elseif( NOT APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL armv7 )
    set( NATIVE_TARGET TCC_TARGET_ARM )
    set( CODE_GENERATOR_SRCS ${ARM_FILES} )
  elseif( APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm64 )
    set( NATIVE_TARGET TCC_TARGET_MACHO )
    set( CODE_GENERATOR_SRCS ${ARM64_OSX_FILES} )
  elseif( APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL x86_64 )
    set( NATIVE_TARGET TCC_TARGET_MACHO )
    set( CODE_GENERATOR_SRCS ${X86_64_OSX_FILES} )
  endif()
endif( WIN32 AND NOT CYGWIN )

find_package (Threads)

################################################################################
# TCC compiler as library

add_library(libtcc STATIC ${CODE_GENERATOR_SRCS})
target_compile_definitions(libtcc
    PRIVATE ONE_SOURCE=0
    PUBLIC "${NATIVE_TARGET}="
    PUBLIC TCC_LIBTCC1=\"libtcc1-${CMAKE_TARGET_ARCHITECTURES}.a\"
    PRIVATE CONFIG_TCCDIR=\".\"
    PUBLIC TCC_VERSION=\"${TCC_VERSION}\")

if(WIN32)
    if( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm64 )
        target_compile_definitions(libtcc PUBLIC
            "TCC_TARGET_ARM="
            "TCC_ARM_VFP="
            "TCC_ARM_EABI="
            "TCC_ARM_HARDFLOAT="
            "TCC_TARGET_PE="
            CONFIG_TCC_LIBPATHS=\"{B}\")
    elseif( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL i386 )
        target_compile_definitions(libtcc PUBLIC
            "TCC_TARGET_I386="
            "TCC_TARGET_PE="
            CONFIG_TCC_LIBPATHS=\"{B}\")
    elseif( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL x86_64 )
        target_compile_definitions(libtcc PUBLIC
            "TCC_TARGET_X86_64="
            "TCC_TARGET_PE="
            CONFIG_TCC_LIBPATHS=\"{B}\")
    endif()
elseif (UNIX)
    if(APPLE)
        if( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm64 )
            target_compile_definitions(libtcc PUBLIC
                "TCC_TARGET_ARM64="
                "TCC_TARGET_MACHO=1")
        elseif( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL x86_64 )
            target_compile_definitions(libtcc PUBLIC
                "TCC_TARGET_X86_64="
                "TCC_TARGET_MACHO=1")
        endif()
    else()
        if( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL riscv64 )
            target_compile_definitions(libtcc PUBLIC
                "TCC_TARGET_RISCV64="
                "CONFIG_USE_LIBGCC="
                CONFIG_TRIPLET=\"riscv64-linux-gnu\")
        elseif( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm64 )
            target_compile_definitions(libtcc PUBLIC
                "TCC_TARGET_ARM64="
                "CONFIG_USE_LIBGCC="
                CONFIG_TRIPLET=\"aarch64-linux-gnu\")
        elseif( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL i386 )
            target_compile_definitions(libtcc PUBLIC
                "TCC_TARGET_I386="
                "CONFIG_USE_LIBGCC="
                CONFIG_TRIPLET=\"i386-linux-gnu\")
        elseif( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL x86_64 )
            target_compile_definitions(libtcc PUBLIC
                "TCC_TARGET_X86_64="
                "CONFIG_USE_LIBGCC="
                CONFIG_TRIPLET=\"x86_64-linux-gnu\")
        elseif( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL armv7 )
            target_compile_definitions(libtcc PUBLIC
                "TCC_TARGET_ARM="
                "TCC_ARM_VFP="
                "TCC_ARM_EABI="
                "TCC_ARM_HARDFLOAT="
                "CONFIG_USE_LIBGCC="
                CONFIG_TRIPLET=\"arm-linux-gnueabi\")
        elseif( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm_eabi )
            target_compile_definitions(libtcc PUBLIC
                "TCC_TARGET_ARM="
                "TCC_ARM_VFP="
                "TCC_ARM_EABI="
                "CONFIG_USE_LIBGCC="
                CONFIG_TRIPLET=\"arm-linux-gnueabi\")
        endif()
    endif()
    target_link_libraries(libtcc PRIVATE
        dl ${CMAKE_THREAD_LIBS_INIT})
endif()

target_include_directories(libtcc
    PUBLIC ${TCC_DIR}
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/tcc-config)
set_target_properties(libtcc PROPERTIES
    PREFIX ""
    POSITION_INDEPENDENT_CODE ON
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")

################################################################################
# TCC compiler as executable

add_executable(tcc ${TCC_DIR}/tcc.c)
target_link_libraries(tcc PRIVATE libtcc)

################################################################################
# Runtime Library (libtcc1-xx.a)

set( BCHECK_O ${TCC_DIR}/lib/bcheck.c )
set( B_O ${BCHECK_O} ${TCC_DIR}/lib/bt-exe.c ${TCC_DIR}/lib/bt-log.c ${TCC_DIR}/lib/bt-dll.c )
set( BT_O ${TCC_DIR}/lib/bt-exe.c ${TCC_DIR}/lib/bt-log.c ${TCC_DIR}/lib/tcov.c )

set( I386_O
 ${TCC_DIR}/lib/libtcc1.c ${TCC_DIR}/lib/alloca.S
 ${TCC_DIR}/lib/alloca-bt.S ${TCC_DIR}/lib/stdatomic.c
 ${TCC_DIR}/lib/atomic.S ${TCC_DIR}/lib/builtin.c ${BT_O} )

set( X86_64_O
 ${TCC_DIR}/lib/libtcc1.c ${TCC_DIR}/lib/alloca.S
 ${TCC_DIR}/lib/alloca-bt.S ${TCC_DIR}/lib/stdatomic.c
 ${TCC_DIR}/lib/atomic.S ${TCC_DIR}/lib/builtin.c ${BT_O} )

set( ARM_O
 ${TCC_DIR}/lib/libtcc1.c ${TCC_DIR}/lib/armeabi.c
 ${TCC_DIR}/lib/alloca.S ${TCC_DIR}/lib/armflush.c
 ${TCC_DIR}/lib/stdatomic.c ${TCC_DIR}/lib/atomic.S
 ${TCC_DIR}/lib/builtin.c ${BT_O} )

set( ARM64_O
 ${TCC_DIR}/lib/lib-arm64.c ${TCC_DIR}/lib/stdatomic.c
 ${TCC_DIR}/lib/atomic.S ${TCC_DIR}/lib/builtin.c ${BT_O} )

set( RISCV64_O
 ${TCC_DIR}/lib/lib-arm64.c ${TCC_DIR}/lib/stdatomic.c
 ${TCC_DIR}/lib/atomic.S ${TCC_DIR}/lib/builtin.c ${BT_O} )

set( WIN_O
 ${TCC_DIR}/win32/lib/crt1.c ${TCC_DIR}/win32/lib/crt1w.c ${TCC_DIR}/win32/lib/wincrt1.c
 ${TCC_DIR}/win32/lib/wincrt1w.c ${TCC_DIR}/win32/lib/dllcrt1.c ${TCC_DIR}/win32/lib/dllmain.c )

set( DSO_O ${TCC_DIR}/lib/dsohandle.c )

set( OBJ-i386 ${I386_O} ${BCHECK_O} ${DSO_O} )
set( OBJ-x86_64 ${X86_64_O} ${TCC_DIR}/lib/va_list.c ${BCHECK_O} ${DSO_O} )
set( OBJ-x86_64-osx ${X86_64_O} ${TCC_DIR}/lib/va_list.c ${BCHECK_O} )
set( OBJ-i386-win32 ${I386_O} ${TCC_DIR}/win32/lib/chkstk.S ${B_O} ${WIN_O} )
set( OBJ-x86_64-win32 ${X86_64_O} ${TCC_DIR}/win32/lib/chkstk.S ${B_O} ${WIN_O} )
set( OBJ-arm64 ${ARM64_O} ${BCHECK_O} ${DSO_O} )
set( OBJ-arm64-osx ${ARM64_O} ${BCHECK_O} )
set( OBJ-arm ${ARM_O} ${BCHECK_O} ${DSO_O} )

set( OBJ-arm-fpa ${ARM_O} ${DSO_O} )
set( OBJ-arm-fpa-ld ${ARM_O} ${DSO_O} )
set( OBJ-arm-vfp ${ARM_O} ${DSO_O} )
set( OBJ-arm-eabi ${ARM_O} ${DSO_O} )
set( OBJ-arm-eabihf ${ARM_O} ${DSO_O} )
set( OBJ-arm-wince ${ARM_O} ${WIN_O} )
set( OBJ-riscv64 ${RISCV64_O} ${BCHECK_O} ${DSO_O} )

if (WIN32)
  if( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL i386 )
    set( LIB_SRCFILES ${OBJ-i386-win32} )
  elseif( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL x86_64 )
    set( LIB_SRCFILES ${OBJ-x86_64-win32} )
  elseif( ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm64
    OR ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm )
        set( LIB_SRCFILES ${OBJ-arm-wince} )
  endif()
else()
  if( NOT APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL riscv64 )
    set( LIB_SRCFILES ${OBJ-riscv64} )
  elseif( NOT APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm64 )
    set( LIB_SRCFILES ${OBJ-arm64} )
  elseif( NOT APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL i386 )
    set( LIB_SRCFILES ${OBJ-i386} )
  elseif( NOT APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL x86_64 )
    set( LIB_SRCFILES ${OBJ-x86_64} )
  elseif( NOT APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm_eabi )
    set( LIB_SRCFILES ${OBJ-arm-eabi} )
  elseif( NOT APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm )
    set( LIB_SRCFILES ${OBJ-arm} )
  elseif( NOT APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL armv7 )
    set( LIB_SRCFILES ${OBJ-eabihf} )
  elseif( APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL arm64 )
    set( LIB_SRCFILES ${OBJ-arm64-osx} )
  elseif( APPLE AND ${CMAKE_TARGET_ARCHITECTURES} STREQUAL x86_64 )
    set( LIB_SRCFILES ${OBJ-x86_64-osx} )
  endif()
endif()

# compile C files to object files
file(MAKE_DIRECTORY ${LIB_DIR})
foreach(LIB_SRCFILE ${LIB_SRCFILES})
    get_filename_component(LIB_SRCFILE_NOEXT ${LIB_SRCFILE} NAME_WE)
    add_custom_command(
        OUTPUT ${LIB_DIR}/${LIB_SRCFILE_NOEXT}.o
        COMMAND tcc -s -DCONFIG_TCC_STATIC -fPIC -gdwarf -fno-omit-frame-pointer -Wno-unused-function -Wno-unused-variable ${TCC_INCLUDE_DIRS}
                    -L ${LIB_DIR}
                    -c ${LIB_SRCFILE}
                    -o ${LIB_DIR}/${LIB_SRCFILE_NOEXT}.o
        MAIN_DEPENDENCY ${LIB_SRCFILE}
        VERBATIM
        COMMENT "Compiling Runtime Library (${LIB_SRCFILE})")
    list(APPEND LIB_OBJFILES ${LIB_DIR}/${LIB_SRCFILE_NOEXT}.o)
endforeach()

# link all objects files to archive file
add_custom_target(
    libarchive ALL
    tcc -ar rcs ${LIB_DIR}/libtcc1-${CMAKE_TARGET_ARCHITECTURES}.a ${LIB_OBJFILES}
    DEPENDS ${LIB_OBJFILES}
    BYPRODUCTS ${LIB_DIR}/libtcc1-${CMAKE_TARGET_ARCHITECTURES}.a
    COMMENT "Linking Runtime Library"
    SOURCES ${LIB_SRCFILES})

add_subdirectory(tests)

if(WIN32)
  file(GLOB WIN32_DEFS win32/lib/*.def)
  install(FILES ${WIN32_DEFS} DESTINATION lib)
  install(FILES tcclib.h DESTINATION include)
  install(DIRECTORY include/ DESTINATION include)
  install(DIRECTORY win32/include/ DESTINATION include)
  install(FILES ${LIB_DIR}/libtcc1-${CMAKE_TARGET_ARCHITECTURES}.a DESTINATION lib)
  install(FILES libtcc.h DESTINATION lib)
else()
  install(FILES libtcc.h tcclib.h DESTINATION include)
  install(DIRECTORY include/ DESTINATION lib/tcc/include)
  install(DIRECTORY win32/include/ DESTINATION lib/tcc/win32/include)
  install(DIRECTORY include/ DESTINATION lib/tcc/win32/include)
endif()

project(tcc C)
cmake_minimum_required(VERSION 2.6)
enable_testing()

# Detect native platform
if(WIN32)
  set(BUILD_SHARED_LIBS ON)
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM")
    set(TCC_NATIVE_TARGET "WinCE")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(TCC_NATIVE_TARGET "Win64")
  else()
    set(TCC_NATIVE_TARGET "Win32")
  endif()
else()
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
    set(TCC_NATIVE_TARGET "ARM")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^armv5")
      set(TCC_ARM_VERSION_DEFAULT 5)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^armv6")
      set(TCC_ARM_VERSION_DEFAULT 6)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^armv7")
      set(TCC_ARM_VERSION_DEFAULT 7)
    else()
      set(TCC_ARM_VERSION_DEFAULT 4)
    endif()
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(TCC_NATIVE_TARGET "x86_64")
    set(TCC_ARCH_DIR "x86_64-linux-gnu")
  else()
    set(TCC_NATIVE_TARGET "i386")
    set(TCC_ARCH_DIR "i386-linux-gnu")
  endif()
endif()

if(WIN32)
  set(EXE_PATH ".")
  set(TCC_LIB_PATH lib)
  set(NATIVE_LIB_PATH)
  set(DOC_PATH doc)
else()
  set(EXE_PATH bin)
  set(TCC_LIB_PATH lib/tcc)
  set(NATIVE_LIB_PATH lib)
  set(DOC_PATH share/doc/tcc)
endif()

if(NOT WIN32)
  if(EXISTS /usr/lib/${TCC_ARCH_DIR}/crti.o)
    set(CONFIG_LDDIR lib/${TCC_ARCH_DIR})
    set(CONFIG_MULTIARCHDIR ${TCC_ARCH_DIR})
  elseif(EXISTS /usr/lib64/crti.o)
    set(CONFIG_LDDIR lib64)
  endif()
endif()

# Use two variables to keep CMake configuration variable names consistent
set(TCC_BCHECK OFF CACHE BOOL "Enable bounds checking")
set(CONFIG_TCC_BCHECK ${TCC_BCHECK})
set(TCC_ASSERT OFF CACHE BOOL "Enable assertions")
set(CONFIG_TCC_ASSERT ${TCC_ASSERT})

set(TCC_BUILD_NATIVE ON CACHE BOOL "Build native compiler")
set(TCC_BUILD_I386 OFF CACHE BOOL "Build i386 cross compiler")
set(TCC_BUILD_X64 OFF CACHE BOOL "Build x86-64 cross compiler")
set(TCC_BUILD_WIN32 OFF CACHE BOOL "Build Windows i386 cross compiler")
set(TCC_BUILD_WIN64 OFF CACHE BOOL "Build Windows x86-64 cross compiler")
set(TCC_BUILD_WINCE OFF CACHE BOOL "Build Windows CE cross compiler")
set(TCC_BUILD_ARM_FPA OFF CACHE BOOL "Build arm-fpa cross compiler")
set(TCC_BUILD_ARM_FPA_LD OFF CACHE BOOL "Build arm-fpa-ld cross compiler")
set(TCC_BUILD_ARM_VFP OFF CACHE BOOL "Build arm-vfp cross compiler")
set(TCC_BUILD_ARM_EABI OFF CACHE BOOL "Build ARM EABI cross compiler")
set(TCC_BUILD_ARM_EABIHF OFF CACHE BOOL "Build ARM EABI hardware float cross compiler")
set(TCC_BUILD_ARM OFF CACHE BOOL "Build ARM cross compiler")
set(TCC_BUILD_C67 OFF CACHE BOOL "Build C67 cross compiler")

set(TCC_ARM_VERSION ${TCC_ARM_VERSION_DEFAULT} CACHE STRING "ARM target CPU version")
set_property(CACHE TCC_ARM_VERSION PROPERTY STRINGS 4 5 6 7)

if(WIN32)
  set(CONFIG_TCCDIR ${CMAKE_INSTALL_PREFIX})
else()
  set(CONFIG_TCCDIR ${CMAKE_INSTALL_PREFIX}/lib/tcc)
endif()

file(STRINGS "VERSION" TCC_VERSION)
list(GET TCC_VERSION 0 TCC_VERSION)
include_directories(${CMAKE_BINARY_DIR})
configure_file(config.h.in config.h)
configure_file(config.texi.in config.texi)

# Utility variables
set(I386_SOURCES i386-gen.c i386-asm.c i386-asm.h i386-tok.h)
set(X86_64_SOURCES x86_64-gen.c i386-asm.c x86_64-asm.h)
set(ARM_SOURCES arm_gen.c)

set(LIBTCC1_I386_SOURCES lib/alloca86.S lib/alloca86-bt.S)
set(LIBTCC1_WIN_SOURCES win32/lib/crt1.c win32/lib/wincrt1.c win32/lib/dllcrt1.c win32/lib/dllmain.c win32/lib/chkstk.S)
if(NOT WIN32)
  set(LIBTCC1_I386_SOURCES ${LIBTCC1_I386_SOURCES} lib/bcheck.c)
endif()

if(WIN32)
  add_executable(tiny_impdef win32/tools/tiny_impdef.c)
endif()
add_executable(tiny_libmaker_32 win32/tools/tiny_libmaker.c)
set_target_properties(tiny_libmaker_32 PROPERTIES COMPILE_DEFINITIONS TCC_TARGET_I386)
add_executable(tiny_libmaker_64 win32/tools/tiny_libmaker.c)
set_target_properties(tiny_libmaker_64 PROPERTIES COMPILE_DEFINITIONS TCC_TARGET_X86_64)

macro(make_libtcc1 prefix xcc xar defs includes sources)
  set(libtcc1_prefix)
  if("${prefix}" STRGREATER "")
    set(libtcc1_prefix ${prefix}-)
  endif()
  set(libtcc1_flags -I${CMAKE_SOURCE_DIR}/include)
  foreach(i ${defs})
    set(libtcc1_flags ${libtcc1_flags} -D${i})
  endforeach()
  foreach(i ${includes})
    set(libtcc1_flags ${libtcc1_flags} -I${CMAKE_SOURCE_DIR}/${i})
  endforeach()
  set(libtcc1_objects)
  foreach(libtcc1_c lib/libtcc1.c ${sources})
    string(REGEX MATCH "[^/]+$" libtcc1_o ${libtcc1_c})
    string(REGEX MATCH "^[^.]+" libtcc1_o ${libtcc1_o})
    set(libtcc1_o ${libtcc1_prefix}${libtcc1_o}.o)
    add_custom_command(OUTPUT ${libtcc1_o}
      COMMAND ${xcc} ${libtcc1_flags} -c ${CMAKE_SOURCE_DIR}/${libtcc1_c} -o ${libtcc1_o}
      DEPENDS ${xcc} ${CMAKE_SOURCE_DIR}/${libtcc1_c}
    )
    list(APPEND libtcc1_objects ${libtcc1_o})
  endforeach()
  add_custom_command(OUTPUT ${libtcc1_prefix}libtcc1.a
    COMMAND ${xar} ${libtcc1_prefix}libtcc1.a ${libtcc1_objects}
    DEPENDS ${xar} ${libtcc1_objects}
  )
  add_custom_target(${libtcc1_prefix}libtcc1 ALL DEPENDS ${libtcc1_prefix}libtcc1.a)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${libtcc1_prefix}libtcc1.a
    DESTINATION ${TCC_LIB_PATH}/${prefix} RENAME libtcc1.a)
endmacro()

macro(make_tcc native_name cross_name cross_enabled definitions tcc_sources libtcc_ar libtcc_sources libtcc_includes)
  if (xx${native_name} STREQUAL xx${TCC_NATIVE_TARGET})
    set(TCC_NATIVE_DEFINITIONS ${definitions})
    if("${CONFIG_MULTIARCHDIR}" STRGREATER "")
      set(TCC_NATIVE_DEFINITIONS ${TCC_NATIVE_DEFINITIONS} CONFIG_MULTIARCHDIR="${CONFIG_MULTIARCHDIR}")
    endif()
    if("${CONFIG_LDDIR}" STRGREATER "")
      set(TCC_NATIVE_DEFINITIONS ${TCC_NATIVE_DEFINITIONS} CONFIG_LDDIR="${CONFIG_LDDIR}")
    endif()

    set(TCC_NATIVE_FLAGS)
    foreach(make_tcc_def ${TCC_NATIVE_DEFINITIONS})
      set(TCC_NATIVE_FLAGS ${TCC_NATIVE_FLAGS} -D${make_tcc_def})
    endforeach()
    
    if (TCC_BUILD_NATIVE)
      add_library(libtcc
        libtcc.c
        tccpp.c
        tccgen.c
        tccelf.c
        tccasm.c
        tccrun.c
        tcc.h
        libtcc.h
        tcctok.h
        ${tcc_sources}
      )
      set_target_properties(libtcc PROPERTIES OUTPUT_NAME tcc PREFIX lib)
      if(WIN32)
        set_target_properties(libtcc PROPERTIES LINK_FLAGS "-Wl,--output-def,libtcc.def")
      endif()
      add_executable(tcc tcc.c)
      target_link_libraries(tcc libtcc)
      if(NOT WIN32)
        target_link_libraries(tcc dl)
      endif()
      install(TARGETS tcc libtcc RUNTIME DESTINATION ${EXE_PATH} LIBRARY DESTINATION ${NATIVE_LIB_PATH} ARCHIVE DESTINATION ${NATIVE_LIB_PATH})
      set_target_properties(tcc libtcc PROPERTIES COMPILE_DEFINITIONS "${TCC_NATIVE_DEFINITIONS}")
      
      if("${libtcc_sources}" STRGREATER "")
        make_libtcc1("" tcc "${libtcc_ar}" "${TCC_NATIVE_DEFINITIONS}" "${libtcc_includes}" "${libtcc_sources}")
      endif()
    endif()
  elseif(${cross_enabled})
    add_executable(${cross_name}-tcc tcc.c)
    set_target_properties(${cross_name}-tcc PROPERTIES COMPILE_DEFINITIONS "ONE_SOURCE;${definitions}")
    install(TARGETS ${cross_name}-tcc RUNTIME DESTINATION ${EXE_PATH})
    
    if("${libtcc_sources}" STRGREATER "")
      make_libtcc1(${cross_name} "${cross_name}-tcc" "${libtcc_ar}" "${definitions}" "${libtcc_includes}" "${libtcc_sources}")
    endif()
  endif()
endmacro()

make_tcc("Win32" i386-w64-mingw32 TCC_BUILD_WIN32
  "TCC_TARGET_I386;TCC_TARGET_PE"
  "${I386_SOURCES};tccpe.c"
  tiny_libmaker_32 "${LIBTCC1_I386_SOURCES};${LIBTCC1_WIN_SOURCES}" "win32/include;win32/include/winapi"
)
make_tcc("Win64" x86_64-w64-mingw32 TCC_BUILD_WIN64
  "TCC_TARGET_X86_64;TCC_TARGET_PE"
  "${X86_64_SOURCES};tccpe.c"
  tiny_libmaker_64 "lib/alloca86_64.S;${LIBTCC1_WIN_SOURCES}" "win32/include;win32/include/winapi"
)
make_tcc("WinCE" arm-wince-mingw32ce TCC_BUILD_WINCE
  "TCC_TARGET_ARM;TCC_ARM_VERSION=${TCC_ARM_VERSION};TCC_TARGET_PE"
  "${ARM_SOURCES};tccpe.c"
  "" "" ""
)
make_tcc("i386" i386-linux-gnu TCC_BUILD_I386
  TCC_TARGET_I386
  "${I386_SOURCES}"
  tiny_libmaker_32 "${LIBTCC1_I386_SOURCES}" ""
)
make_tcc("x86_64" x86_64-linux-gnu TCC_BUILD_X64
  TCC_TARGET_X86_64
  "${X86_64_SOURCES}"
  tiny_libmaker_64 "lib/alloca86_64.S" ""
)
set(ARM_DEFINITIONS TCC_TARGET_ARM TCC_ARM_VERSION=${TCC_ARM_VERSION})
make_tcc("" arm-linux-gnueabihf TCC_BUILD_ARM_EABIHF
  "${ARM_DEFINITIONS};TCC_ARM_EABI;TCC_ARM_HARDFLOAT"
  "${ARM_SOURCES}"
  "" "" ""
)
make_tcc("" arm-linux-gnueabi TCC_BUILD_ARM_EABI
  "${ARM_DEFINITIONS};TCC_ARM_EABI"
  "${ARM_SOURCES}"
  "" "" ""
)
make_tcc("" arm-linux-fpa TCC_BUILD_ARM_FPA
  "${ARM_DEFINITIONS}"
  "${ARM_SOURCES}"
  "" "" ""
)
make_tcc("" arm-linux-fpa-ld TCC_BUILD_ARM_FPA_LD
  "${ARM_DEFINITIONS};LDOUBLE_SIZE=12"
  "${ARM_SOURCES}"
  "" "" ""
)
make_tcc("" arm-linux-gnu TCC_BUILD_ARM_VFP
  "${ARM_DEFINITIONS};TCC_ARM_VFP"
  "${ARM_SOURCES}"
  "" "" ""
)
make_tcc("" c67 TCC_BUILD_C67
  TCC_TARGET_C67
  "c67-gen.c;tcccoff.c"
  "" "" ""
)

add_subdirectory(tests)

find_program(MAKEINFO NAMES makeinfo PATHS C:/MinGW/MSYS/1.0/bin)
if(MAKEINFO)
  add_custom_command(OUTPUT tcc-doc.html
    COMMAND ${MAKEINFO} --no-split --html -o tcc-doc.html ${CMAKE_CURRENT_SOURCE_DIR}/tcc-doc.texi
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tcc-doc.texi
  )
  set(TCC_DOC_FILES tcc-doc.html)
  if(NOT WIN32)
    add_custom_command(OUTPUT tcc-doc.info
      COMMAND ${MAKEINFO} -o tcc-doc.info ${CMAKE_CURRENT_SOURCE_DIR}/tcc-doc.texi
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tcc-doc.texi
    )
    set(TCC_DOC_FILES ${TCC_DOC_FILES} tcc-doc.info)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tcc-doc.info DESTINATION share/info)
  endif()
  add_custom_target(tcc-doc ALL DEPENDS ${TCC_DOC_FILES})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tcc-doc.html DESTINATION ${DOC_PATH})
endif()

if(WIN32)
  file(GLOB WIN32_DEFS win32/lib/*.def)
  install(FILES ${WIN32_DEFS} DESTINATION lib)
  install(FILES tcclib.h DESTINATION include)
  install(DIRECTORY include/ DESTINATION include)
  install(DIRECTORY win32/include/ DESTINATION include)
  install(DIRECTORY win32/examples/ DESTINATION examples)
  install(FILES win32/tcc-win32.txt DESTINATION doc)
  install(FILES ${CMAKE_BINARY_DIR}/libtcc.def DESTINATION libtcc)
  install(FILES ${CMAKE_BINARY_DIR}/libtcc.dll.a DESTINATION libtcc RENAME libtcc.a)
  install(FILES libtcc.h DESTINATION libtcc)
else()
  install(FILES libtcc.h tcclib.h DESTINATION include)
  install(DIRECTORY include/ DESTINATION lib/tcc/include)
  install(DIRECTORY win32/include/ DESTINATION lib/tcc/win32/include)
  install(DIRECTORY include/ DESTINATION lib/tcc/win32/include)
endif()


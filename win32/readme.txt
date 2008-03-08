
    TinyCC
    ======

    This file contains some additional information for usage of TinyCC
    under MS-Windows:


    Overview:
    --------- 
    TinyCC (aka TCC) is a small but hyperfast C compiler, written by
    Fabrice Bellard.

    TinyCC for MS-Windows can produce console applications, native 
    windows GUI programs and DLL's.

    The package with under 300kb includes a complete C-compiler with
    header files and basic system library support.

    With the -run switch you can run C-sources without any linking
    directly from the command line.

    TinyCC can be used as dynamic code generator library in your own
    program.

    TinyCC can of course compile itself.


    Compilation:  (omit that if you use the binary ZIP package)
    ------------
    You can use the MinGW and MSYS tools available at
    http://www.mingw.org to compile TCC for Windows. Untar the TCC
    archive and type in the MSYS shell:
    
       ./configure
       make
       make install 

       TCC is installed in c:\Program Files\tcc

    Alternatively you can use win32\build-tcc.bat to compile TCC
    with just gcc and ar from MINGW. To install, copy the entire
    contents of the win32 directory to where you want.


    Installation: (from the binary ZIP package)
    -------------
    Just unzip the package to a directory anywhere on your computer.
    
    The binary package does not include libtcc. If you want tcc as
    dynamic code generator, please use the source code distribution.


    Examples:
    ---------
    For the 'Fibonacci' console example type from the command line:

        tcc examples\fib.c

    For the 'Hello Windows' GUI example:

        tcc examples\hello_win.c

    For the 'Hello DLL' example:

        tcc -shared examples\dll.c
        tcc examples\hello_dll.c examples\dll.def


    Import Definition Files:
    ------------------------
    To link with Windows system DLLs, TinyCC uses import definition
    files (.def) instead of libraries.

    The included 'tiny_impdef' program may be used to make additional 
    .def files for any DLL. For example:

        tiny_impdef.exe opengl32.dll

    To use it, put the opengl32.def file into the tcc/lib directory,
    and specify -lopengl32 at the tcc commandline.


    Resource Files:
    ---------------
    TinyCC-PE can now link windows resources in coff format as generated
    by MINGW's windres.exe. For example:

        windres -O coff app.rc -o appres.o
        tcc app.c appres.o -o app.exe


    Tiny Libmaker:
    --------------
    The included tiny_libmaker tool by Timovj Lahde can be used as
    'ar' replacement to make a library from several object files.


    Header Files:
    -------------
    The system header files (except _mingw.h) are from the mingw
    distribution (http://www.mingw.org/).


    Documentation and License:
    --------------------------
    TCC is distributed under the GNU Lesser General Public License
    (see COPYING file).

    Please read tcc-doc.html to have all the features of TCC. Also 
    visit: http://fabrice.bellard.free.fr/tcc/

    
    -- grischka@users.sourceforge.net

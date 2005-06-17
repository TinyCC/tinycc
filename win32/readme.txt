
	TinyCC-PE
	---------

	TinyCC (aka TCC) is a small but hyperfast C compiler,
	written by Fabrice Bellard,


	TinyCC-PE is the TinyCC compiler with an extension to
	write PE executables for MS-Windows.


	Features:
	---------

	TinyCC-PE can produce console applications, native windows
	GUI programs and DLL's.

	Most of the features pointed out by Fabrice Bellard for the
	original version are still valid, i.e:

	- SMALL! The package with ~400kb includes a complete C-compiler
	  with header files for console and GUI applications.

	- With the -run switch you can run C-sources without any
	  linking directly from the command line.

	- TCC can of course compile itself.


	Compilation:  (omit that if you use the binary ZIP package)
	------------
        
        You must use the MinGW and MSYS tools available at
        http://www.mingw.org to compile TCC for Windows. Untar the TCC
        archive and type in the MSYS shell:
        
           ./configure
           make
           make install 

           TCC is installed in c:\Program Files\tcc

	Installation: (from the binary ZIP package)
	-------------

	Just unzip the package to a directory anywhere on your computer.


	Examples:
	---------

	For the 'Fibonacci' console example type from the command line:

		tcc examples\fib.c


	For the 'Hello Windows' GUI example:

		tcc examples\hello_win.c


	For the 'Hello DLL' example:

		tcc -shared examples\dll.c
		tcc examples\hello_dll.c examples\dll.def


	Import Definitions:
	-------------------

	TinyCC-PE searches and reads import definition files similar
	to libraries.

	The included 'tiny_impdef' program may be used to make .def files
	for any DLL, e.g for an 'opengl32.def':

		tiny_impdef.exe opengl32.dll

	or to the same effect:

		tcc -lkernel32 -run tiny_impdef.c opengl32.dll


	Header Files:
	-------------

	The system header files, except '_mingw.h', are from the
	2.0 mingw distribution. See also: http://www.mingw.org/


	Compile TCC:
	------------

	With TCC itself just say:

		tcc src\tcc.c -lkernel32 -o tcc.new.exe

	Other compilers like mingw-gcc or msvc work as well.
	To make libtcc1.a, you need 'ar' from the mingw binutils.


	Documentation and License:
	--------------------------

	TCC is distributed under the GNU Lesser General Public License
	(see COPYING file).

	Please read the original tcc-doc.html to have all the features
	of TCC. Also visit: http://fabrice.bellard.free.fr/tcc/


	--------------------------------------------
	09.Apr.2005 - grischka@users.sourceforge.net


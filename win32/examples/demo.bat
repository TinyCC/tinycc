@echo off

@rem Compute 10th Fibonacci number
tcc fib.c
fib 10
del fib.exe

@rem hello_win.c
tcc hello_win.c
hello_win
del hello_win.exe

@rem 'Hello DLL' example
tcc -shared dll.c
tcc hello_dll.c dll.def
hello_dll
del hello_dll.exe dll.def dll.dll

@rem a console threaded program
tcc taxi_simulator.c
taxi_simulator
del taxi_simulator.exe

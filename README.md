# A clone of libobjcryst for Windows and Python3
This is a modified version of https://github.com/diffpy/libobjcryst

## Main differences from the original
- Windows MSVC compilers compatibility
- Python3 compatibility
- No Anaconda related configurations
  - just because i don't use Anaconda environment...

Last one might be a large gap between original one.
This is a main reason this is a clone, not a fork.

## Branches
- main
  - Modified version of the original one.
- upstream
  - Going to update with the original one.

## Major changes
- Add TMP/TEMP external environment variable handlings
  - SCons, a software construction tool written with Python (https://scons.org/),
    does not use environment variables by default.
  - In SCons, "external environment" and "construction environment" variables are clearly separated.
    Environment variables required by the compiler/linker must be explicitly included in construction variables.
  - For Windows MSVC compilers, TMP/TEMP variables are important
    because link.exe uses the temporary directory in the configure script.
    - If the TMP/TEMP variables are not included in construction environment,
    link.exe will fail in the configure script.
    - That kind of linker failures give misleading messages that headers/libraries are not found,
      because the compiler/linker fails to compile/link simple programs to check for header/library existance.
    - This problem can also be solved by https://scons.org/faq.html#Linking_on_Windows_gives_me_an_error
  - NOTE: currently i don't know which is really necessary, TMP or TEMP...
    https://en.wikipedia.org/wiki/Environment_variable

- Add INCLUDE, LIB external environment variable handlings
  - MSVC compiler use these environment variables to search headers/libraries.

- Change MSVC runtime link option from /MT to /MD
  - /MT and /MD links runtime libraries statically and dynamically, respectively.
  - 
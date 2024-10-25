AC_PREREQ([2.71])
AC_INIT([nes-tools], [0.1])
AC_CONFIG_SRCDIR([src/cpu6502.c])

AC_CONFIG_HEADERS([src/config.h])

# Checks for header files.
AC_CHECK_HEADERS([
        stdlib.h stdint.h time.h
        SDL2/SDL.h SDL2/SDL_ttf.h
        stdarg.h stdio.h
        ])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

AC_CONFIG_FILES([
        Makefile
        src/Makefile
        src/audio/Makefile
        ])

AC_OUTPUT

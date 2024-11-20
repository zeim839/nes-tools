AC_PREREQ([2.71])
AC_INIT([nes-tools], [0.1])
AC_CONFIG_SRCDIR([src/cpu6502.c])

AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# User-specified SDL2 path.
AC_ARG_WITH([sdl-prefix],
  [AS_HELP_STRING([--with-sdl-prefix=DIR],
                  [Specify the prefix directory for SDL2])],
  [sdl_prefix=$withval],
  [sdl_prefix=""])

if test -n "$sdl_prefix"; then
  CFLAGS="$CFLAGS -I$sdl_prefix/include"
  LDFLAGS="$LDFLAGS -L$sdl_prefix/lib"
fi

# User-specified SDL2_ttf path.
AC_ARG_WITH([sdl-ttf-prefix],
  [AS_HELP_STRING([--with-sdl-ttf-prefix=DIR],
                  [Specify the prefix directory for SDL2_ttf])],
  [sdl_ttf_prefix=$withval],
  [sdl_ttf_prefix=""])

if test -n "$sdl_ttf_prefix"; then
  CFLAGS="$CFLAGS -I$sdl_ttf_prefix/include/SDL2"
  LDFLAGS="$LDFLAGS -L$sdl_ttf_prefix/lib"
fi

# Validate SDL2 installation.
AC_CHECK_HEADER([SDL2/SDL_ttf.h],
  [AC_MSG_NOTICE([SDL2_ttf headers found])],
  [AC_MSG_ERROR([SDL2_ttf headers not found in $sdl2_ttf_prefix/include/SDL2])])

AC_CHECK_LIB([SDL2_ttf], [TTF_Init],
  [AC_MSG_NOTICE([SDL2_ttf library found])],
  [AC_MSG_ERROR([SDL2_ttf library not found in $sdl2_ttf_prefix/lib])])

AC_CHECK_HEADER([SDL2/SDL.h],
  [AC_MSG_NOTICE([SDL2 headers found])],
  [AC_MSG_ERROR([SDL2 headers not found in $sdl2_prefix/include/SDL2])])

AC_CHECK_LIB([SDL2], [SDL_Init],
  [AC_MSG_NOTICE([SDL2 library found])],
  [AC_MSG_ERROR([SDL2 library not found in $sdl2_prefix/lib])])

# Checks for header files.
AC_CHECK_HEADERS([
        stdlib.h stdint.h time.h
        SDL2/SDL.h SDL2/SDL_ttf.h
        stdarg.h stdio.h
        ])

AC_CONFIG_FILES([
        Makefile
        src/Makefile
        src/audio/Makefile
        ])

AC_OUTPUT
